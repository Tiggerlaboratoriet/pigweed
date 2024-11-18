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
/// `LastFitBlockAllocator`.
template <typename OffsetType>
using LastFitBlock = DetailedBlock<OffsetType>;

/// Block allocator that uses a "last-fit" allocation strategy.
///
/// In this strategy, the allocator handles an allocation request by starting at
/// the end of the range of blocks and looking for the last one which can
/// satisfy the request.
///
/// This strategy may result in slightly better fragmentation than the
/// corresponding "first-fit" strategy, since even with alignment it will result
/// in at most one unused fragment before the allocated block.
template <typename OffsetType = uintptr_t>
class LastFitBlockAllocator : public BlockAllocator<LastFitBlock<OffsetType>> {
 public:
  using BlockType = LastFitBlock<OffsetType>;

 private:
  using Base = BlockAllocator<BlockType>;

 public:
  /// Constexpr constructor. Callers must explicitly call `Init`.
  constexpr LastFitBlockAllocator() = default;

  /// Non-constexpr constructor that automatically calls `Init`.
  ///
  /// @param[in]  region  Region of memory to use when satisfying allocation
  ///                     requests. The region MUST be valid as an argument to
  ///                     `BlockType::Init`.
  explicit LastFitBlockAllocator(ByteSpan region) { Base::Init(region); }

 private:
  /// @copydoc Allocator::Allocate
  BlockResult<BlockType> ChooseBlock(Layout layout) override {
    // Search backwards for the last block that can hold this allocation.
    for (auto* block : Base::rblocks()) {
      auto result = BlockType::AllocLast(std::move(block), layout);
      if (result.ok()) {
        return result;
      }
    }
    return BlockResult<BlockType>(nullptr, Status::NotFound());
  }
};

}  // namespace pw::allocator
