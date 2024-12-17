// Copyright 2024 The Pigweed Authors
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.
#pragma once

#include <optional>

#include "pw_async2/dispatcher.h"
#include "pw_containers/intrusive_forward_list.h"
#include "pw_multibuf/multibuf.h"
#include "pw_result/result.h"
#include "pw_sync/interrupt_spin_lock.h"

namespace pw::multibuf {

class MultiBufAllocationFuture;

enum class ContiguityRequirement {
  kAllowDiscontiguous,
  kNeedsContiguous,
};

inline constexpr ContiguityRequirement kAllowDiscontiguous =
    ContiguityRequirement::kAllowDiscontiguous;
inline constexpr ContiguityRequirement kNeedsContiguous =
    ContiguityRequirement::kNeedsContiguous;

/// Interface for allocating ``MultiBuf`` objects.
///
/// A ``MultiBufAllocator`` differs from a regular ``pw::allocator::Allocator``
/// in that they may provide support for:
/// - Asynchronous allocation.
/// - Non-contiguous buffer allocation.
/// - Internal header/footer reservation.
/// - Size-range allocation.
///
/// In order to accomplish this, they return ``MultiBuf`` objects rather than
/// arbitrary pieces of memory.
///
/// Additionally, ``MultiBufAllocator`` implementations may choose to store
/// their allocation metadata separately from the data itself. This allows for
/// things like allocation headers to be kept out of restricted DMA-capable or
/// shared-memory regions.
///
/// NOTE: ``MultiBufAllocator``s *must* outlive any futures created from them.
class MultiBufAllocator {
 public:
  MultiBufAllocator() = default;

  /// ```MultiBufAllocator`` is not copyable or movable.
  MultiBufAllocator(MultiBufAllocator&) = delete;
  MultiBufAllocator& operator=(MultiBufAllocator&) = delete;
  MultiBufAllocator(MultiBufAllocator&&) = delete;
  MultiBufAllocator& operator=(MultiBufAllocator&&) = delete;

  virtual ~MultiBufAllocator() {}

  /// Attempts to allocate a ``MultiBuf`` of exactly ``size`` bytes.
  ///
  /// Memory allocated by an arbitrary ``MultiBufAllocator`` does not provide
  /// any alignment requirements, preferring instead to allow the allocator
  /// maximum flexibility for placing regions (especially discontiguous
  /// regions).
  ///
  /// @retval ``MultiBuf`` if the allocation was successful.
  /// @retval ``nullopt_t`` if the memory is not currently available.
  std::optional<MultiBuf> Allocate(size_t size);

  /// Attempts to allocate a ``MultiBuf`` of at least ``min_size`` bytes and at
  /// most ``desired_size`` bytes.
  ///
  /// Memory allocated by an arbitrary ``MultiBufAllocator`` does not provide
  /// any alignment requirements, preferring instead to allow the allocator
  /// maximum flexibility for placing regions (especially discontiguous
  /// regions).
  ///
  /// @retval ``MultiBuf`` if the allocation was successful.
  /// @retval ``nullopt_t`` if the memory is not currently available.
  std::optional<MultiBuf> Allocate(size_t min_size, size_t desired_size);

  /// Attempts to allocate a contiguous ``MultiBuf`` of exactly ``size``
  /// bytes.
  ///
  /// Memory allocated by an arbitrary ``MultiBufAllocator`` does not provide
  /// any alignment requirements, preferring instead to allow the allocator
  /// maximum flexibility for placing regions (especially discontiguous
  /// regions).
  ///
  /// @retval ``MultiBuf`` with a single ``Chunk`` if the allocation was
  /// successful.
  /// @retval ``nullopt_t`` if the memory is not currently available.
  std::optional<MultiBuf> AllocateContiguous(size_t size);

  /// Attempts to allocate a contiguous ``MultiBuf`` of at least ``min_size``
  /// bytes and at most ``desired_size`` bytes.
  ///
  /// Memory allocated by an arbitrary ``MultiBufAllocator`` does not provide
  /// any alignment requirements, preferring instead to allow the allocator
  /// maximum flexibility for placing regions (especially discontiguous
  /// regions).
  ///
  /// @retval ``MultiBuf`` with a single ``Chunk`` if the allocation was
  /// successful.
  /// @retval ``nullopt_t`` if the memory is not currently available.
  std::optional<MultiBuf> AllocateContiguous(size_t min_size,
                                             size_t desired_size);

 protected:
  /// Awakens callers asynchronously waiting for allocations of at most
  /// ``size_available`` bytes or at most ``contiguous_size_available``
  /// contiguous bytes.
  ///
  /// This function should be invoked by implementations of
  /// ``MultiBufAllocator`` when more memory becomes available to allocate.
  void MoreMemoryAvailable(size_t size_available,
                           size_t contiguous_size_available);

 private:
  friend class MultiBufAllocationFuture;

  // Instances of this class are informed when more memory becomes available.
  class MemoryAvailableDelegate
      : public IntrusiveForwardList<MemoryAvailableDelegate>::Item {
   public:
    explicit MemoryAvailableDelegate() = default;
    MemoryAvailableDelegate(MemoryAvailableDelegate&) = delete;
    MemoryAvailableDelegate& operator=(MemoryAvailableDelegate&) = delete;
    MemoryAvailableDelegate(MemoryAvailableDelegate&&) = delete;
    MemoryAvailableDelegate& operator=(MemoryAvailableDelegate&&) = delete;
    virtual ~MemoryAvailableDelegate() = default;

    // Callback from allocator when new memory being available. Function should
    // return true if object's need has been met which also indicates the object
    // can be released by the allocator.
    virtual bool HandleMemoryAvailable(
        MultiBufAllocator& alloc,
        size_t size_available,
        size_t contiguous_size_available) const = 0;
  };

  void AddMemoryAvailableDelegate(MemoryAvailableDelegate& delegate)
      PW_EXCLUSIVE_LOCKS_REQUIRED(lock_) {
    mem_delegates_.push_front(delegate);
  }

  void RemoveMemoryAvailableDelegate(MemoryAvailableDelegate& delegate)
      PW_EXCLUSIVE_LOCKS_REQUIRED(lock_) {
    mem_delegates_.remove(delegate);
  }

  /// Attempts to allocate a ``MultiBuf`` of at least ``min_size`` bytes and at
  /// most ``desired_size`` bytes.
  ///
  /// @returns @rst
  ///
  /// .. pw-status-codes::
  ///
  ///    OK: Returns the buffer if the allocation was successful.
  ///
  ///    RESOURCE_EXHAUSTED: Insufficient memory is available currently.
  ///
  ///    OUT_OF_RANGE: This amount of memory will not become possible to
  ///    allocate in the future, or this allocator is unable to signal via
  ///    ``MoreMemoryAvailable`` (this will result in asynchronous allocations
  ///    failing immediately on OOM).
  ///
  /// @endrst
  virtual pw::Result<MultiBuf> DoAllocate(
      size_t min_size,
      size_t desired_size,
      ContiguityRequirement contiguity_requirement) = 0;

  sync::InterruptSpinLock lock_;
  IntrusiveForwardList<MemoryAvailableDelegate> mem_delegates_
      PW_GUARDED_BY(lock_);
};

class MultiBufAllocatorAsync {
  // : public MultiBufAllocator::MoreMemoryDelegate {
 public:
  MultiBufAllocatorAsync(MultiBufAllocator& mbuf_allocator)
      : mbuf_allocator_(mbuf_allocator) {}

  /// ```MultiBufAllocatorAsync`` is not copyable or movable.
  MultiBufAllocatorAsync(MultiBufAllocatorAsync&) = delete;
  MultiBufAllocatorAsync& operator=(MultiBufAllocatorAsync&) = delete;
  MultiBufAllocatorAsync(MultiBufAllocatorAsync&&) = delete;
  MultiBufAllocatorAsync& operator=(MultiBufAllocatorAsync&&) = delete;

  /// Asynchronously allocates a ``MultiBuf`` of exactly ``size`` bytes.
  ///
  /// Memory allocated by an arbitrary ``MultiBufAllocator`` does not provide
  /// any alignment requirements, preferring instead to allow the allocator
  /// maximum flexibility for placing regions (especially discontiguous
  /// regions).
  ///
  /// @retval A ``MultiBufAllocationFuture`` which will yield a ``MultiBuf``
  /// when one is available.
  MultiBufAllocationFuture AllocateAsync(size_t size);

  /// Asynchronously allocates a ``MultiBuf`` of at least
  /// ``min_size`` bytes and at most ``desired_size` bytes.
  ///
  /// Memory allocated by an arbitrary ``MultiBufAllocator`` does not provide
  /// any alignment requirements, preferring instead to allow the allocator
  /// maximum flexibility for placing regions (especially discontiguous
  /// regions).
  ///
  /// @retval A ``MultiBufAllocationFuture`` which will yield a ``MultiBuf``
  /// when one is available.
  MultiBufAllocationFuture AllocateAsync(size_t min_size, size_t desired_size);

  /// Asynchronously allocates a contiguous ``MultiBuf`` of exactly ``size``
  /// bytes.
  ///
  /// Memory allocated by an arbitrary ``MultiBufAllocator`` does not provide
  /// any alignment requirements, preferring instead to allow the allocator
  /// maximum flexibility for placing regions (especially discontiguous
  /// regions).
  ///
  /// @retval A ``MultiBufAllocationFuture`` which will yield an ``MultiBuf``
  /// consisting of a single ``Chunk`` when one is available.
  MultiBufAllocationFuture AllocateContiguousAsync(size_t size);

  /// Asynchronously allocates an ``OwnedChunk`` of at least
  /// ``min_size`` bytes and at most ``desired_size`` bytes.
  ///
  /// @retval A ``MultiBufAllocationFuture`` which will yield an ``MultiBuf``
  /// consisting of a single ``Chunk`` when one is available.
  MultiBufAllocationFuture AllocateContiguousAsync(size_t min_size,
                                                   size_t desired_size);

 private:
  MultiBufAllocator& mbuf_allocator_;
};

/// An object that asynchronously yields a ``MultiBuf`` when ``Pend``ed.
///
/// See ``pw::async2`` for details on ``Pend`` and how it is used to build
/// asynchronous tasks.
class MultiBufAllocationFuture
    : public MultiBufAllocator::MemoryAvailableDelegate {
 public:
  constexpr explicit MultiBufAllocationFuture(MultiBufAllocator& allocator)
      : allocator_(&allocator),
        min_size_(0),
        desired_size_(0),
        contiguity_requirement_(kAllowDiscontiguous) {}
  MultiBufAllocationFuture(MultiBufAllocator& allocator,
                           size_t min_size,
                           size_t desired_size,
                           ContiguityRequirement contiguity_requirement)
      : allocator_(&allocator),
        min_size_(min_size),
        desired_size_(desired_size),
        contiguity_requirement_(contiguity_requirement) {}

  MultiBufAllocationFuture(MultiBufAllocationFuture&&);
  MultiBufAllocationFuture& operator=(MultiBufAllocationFuture&&);
  ~MultiBufAllocationFuture() override;

  void SetDesiredSize(size_t min_size) {
    SetDesiredSizes(min_size, min_size, kAllowDiscontiguous);
  }
  void SetDesiredSizes(size_t min_size,
                       size_t desired_size,
                       ContiguityRequirement contiguity_requirement);
  async2::Poll<std::optional<MultiBuf>> Pend(async2::Context& cx);

  /// Returns the ``allocator`` associated with this future.
  MultiBufAllocator& allocator() { return *allocator_; }
  size_t min_size() const { return min_size_; }
  size_t desired_size() const { return min_size_; }
  bool needs_contiguous() const {
    return contiguity_requirement_ == kNeedsContiguous;
  }

 private:
  friend class MultiBufAllocator;

  bool HandleMemoryAvailable(MultiBufAllocator& alloc,
                             size_t size_available,
                             size_t contiguous_size_available) const override;

  /// Attempts to allocate with the stored parameters.
  async2::Poll<std::optional<MultiBuf>> TryAllocate();

  // The allocator this future is tied to.
  MultiBufAllocator* allocator_;

  // The waker to wake when a suitably-sized allocation becomes available.
  async2::Waker waker_;

  // The properties of the kind of allocation being waited for.
  //
  // These properties can only be mutated while holding the allocator's lock,
  // however the MultiBufAllocationFuture owner can freely read these values
  // without needing to acquire the lock.
  //
  // The allocator may read these values so long as this value is listed and
  // the allocator holds the lock.
  // The properties of the kind of allocation being waited for.
  size_t min_size_;
  size_t desired_size_;
  ContiguityRequirement contiguity_requirement_;
};

}  // namespace pw::multibuf
