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

#include "pw_allocator/block/detailed_block.h"
#include "pw_allocator/block_allocator.h"

namespace pw::allocator {

/// Alias for a default block type that is compatible with
/// `DualFirstFitBlockAllocator`.
template <typename OffsetType>
using DualFirstFitBlock = DetailedBlock<OffsetType>;

/// Block allocator that uses a "dual first-fit" allocation strategy split
/// between large and small allocations.
///
/// In this strategy, the strategy includes a threshold value. Requests for
/// more than this threshold are handled similarly to `FirstFit`, while requests
/// for less than this threshold are handled similarly to `LastFit`.
///
/// This algorithm approaches the performance of `FirstFit` and `LastFit` while
/// improving on those algorithms fragmentation.
template <typename OffsetType = uintptr_t>
class DualFirstFitBlockAllocator
    : public BlockAllocator<DualFirstFitBlock<OffsetType>> {
 public:
  using BlockType = DualFirstFitBlock<OffsetType>;

 private:
  using Base = BlockAllocator<BlockType>;

 public:
  /// Constexpr constructor. Callers must explicitly call `Init`.
  constexpr DualFirstFitBlockAllocator() = default;

  /// Non-constexpr constructor that automatically calls `Init`.
  ///
  /// @param[in]  region      Region of memory to use when satisfying allocation
  ///                         requests. The region MUST be valid as an argument
  ///                         to `BlockType::Init`.
  /// @param[in]  threshold   Value for which requests are considered "large".
  DualFirstFitBlockAllocator(ByteSpan region, size_t threshold)
      : threshold_(threshold) {
    Base::Init(region);
  }

  /// Sets the threshold value for which requests are considered "large".
  void set_threshold(size_t threshold) { threshold_ = threshold; }

 private:
  /// @copydoc BlockAllocator::ChooseBlock
  BlockResult<BlockType> ChooseBlock(Layout layout) override {
    if (layout.size() < threshold_) {
      // Search backwards for the last block that can hold this allocation.
      for (auto* block : Base::rblocks()) {
        auto result = BlockType::AllocLast(std::move(block), layout);
        if (result.ok()) {
          return result;
        }
      }
    } else {
      // Search forwards for the first block that can hold this allocation.
      for (auto* block : Base::blocks()) {
        auto result = BlockType::AllocFirst(std::move(block), layout);
        if (result.ok()) {
          return result;
        }
      }
    }
    // No valid block found.
    return BlockResult<BlockType>(nullptr, Status::NotFound());
  }

  size_t threshold_ = 0;
};

}  // namespace pw::allocator
