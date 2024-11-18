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

#include "pw_allocator/best_fit_block_allocator.h"

#include "pw_allocator/block_allocator_testing.h"
#include "pw_unit_test/framework.h"

namespace {

// Test fixtures.

using ::pw::allocator::Layout;
using ::pw::allocator::test::Preallocation;
using BestFitBlockAllocator = ::pw::allocator::BestFitBlockAllocator<uint16_t>;
using BlockAllocatorTest =
    ::pw::allocator::test::BlockAllocatorTest<BestFitBlockAllocator>;

class BestFitBlockAllocatorTest : public BlockAllocatorTest {
 public:
  BestFitBlockAllocatorTest() : BlockAllocatorTest(allocator_) {}

 private:
  BestFitBlockAllocator allocator_;
};

// Unit tests.

TEST_F(BestFitBlockAllocatorTest, CanAutomaticallyInit) {
  BestFitBlockAllocator allocator(GetBytes());
  CanAutomaticallyInit(allocator);
}

TEST_F(BestFitBlockAllocatorTest, CanExplicitlyInit) {
  BestFitBlockAllocator allocator;
  CanExplicitlyInit(allocator);
}

TEST_F(BestFitBlockAllocatorTest, GetCapacity) { GetCapacity(); }

TEST_F(BestFitBlockAllocatorTest, AllocateLarge) { AllocateLarge(); }

TEST_F(BestFitBlockAllocatorTest, AllocateSmall) { AllocateSmall(); }

TEST_F(BestFitBlockAllocatorTest, AllocateLargeAlignment) {
  AllocateLargeAlignment();
}

TEST_F(BestFitBlockAllocatorTest, AllocateAlignmentFailure) {
  AllocateAlignmentFailure();
}

TEST_F(BestFitBlockAllocatorTest, AllocatesBestCompatible) {
  auto& allocator = GetAllocator({
      {kLargeOuterSize, Preallocation::kFree},
      {kSmallerOuterSize, Preallocation::kUsed},
      {kSmallOuterSize, Preallocation::kFree},
      {kSmallerOuterSize, Preallocation::kUsed},
      {kLargerOuterSize, Preallocation::kFree},
      {Preallocation::kSizeRemaining, Preallocation::kUsed},
  });

  void* ptr1 = allocator.Allocate(Layout(kSmallInnerSize, 1));
  EXPECT_LT(Fetch(1), ptr1);
  EXPECT_LT(ptr1, Fetch(3));

  void* ptr2 = allocator.Allocate(Layout(kSmallInnerSize, 1));
  EXPECT_LT(ptr2, Fetch(1));

  // A second small block fits in the leftovers of the first "Large" block.
  void* ptr3 = allocator.Allocate(Layout(kSmallInnerSize, 1));
  EXPECT_LT(ptr3, Fetch(1));

  allocator.Deallocate(ptr1);
  allocator.Deallocate(ptr2);
  allocator.Deallocate(ptr3);
}

TEST_F(BestFitBlockAllocatorTest, DeallocateNull) { DeallocateNull(); }

TEST_F(BestFitBlockAllocatorTest, DeallocateShuffled) { DeallocateShuffled(); }

TEST_F(BestFitBlockAllocatorTest, IterateOverBlocks) { IterateOverBlocks(); }

TEST_F(BestFitBlockAllocatorTest, ResizeNull) { ResizeNull(); }

TEST_F(BestFitBlockAllocatorTest, ResizeLargeSame) { ResizeLargeSame(); }

TEST_F(BestFitBlockAllocatorTest, ResizeLargeSmaller) { ResizeLargeSmaller(); }

TEST_F(BestFitBlockAllocatorTest, ResizeLargeLarger) { ResizeLargeLarger(); }

TEST_F(BestFitBlockAllocatorTest, ResizeLargeLargerFailure) {
  ResizeLargeLargerFailure();
}

TEST_F(BestFitBlockAllocatorTest, ResizeSmallSame) { ResizeSmallSame(); }

TEST_F(BestFitBlockAllocatorTest, ResizeSmallSmaller) { ResizeSmallSmaller(); }

TEST_F(BestFitBlockAllocatorTest, ResizeSmallLarger) { ResizeSmallLarger(); }

TEST_F(BestFitBlockAllocatorTest, ResizeSmallLargerFailure) {
  ResizeSmallLargerFailure();
}

TEST_F(BestFitBlockAllocatorTest, CanMeasureFragmentation) {
  CanMeasureFragmentation();
}

TEST_F(BestFitBlockAllocatorTest, PoisonPeriodically) { PoisonPeriodically(); }

}  // namespace
