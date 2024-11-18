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

#include "pw_allocator/worst_fit_block_allocator.h"

#include "pw_malloc/config.h"
#include "pw_malloc/malloc.h"

namespace pw::malloc {

using WorstFitBlockAllocator =
    ::pw::allocator::WorstFitBlockAllocator<PW_MALLOC_BLOCK_OFFSET_TYPE,
                                            PW_MALLOC_BLOCK_POISON_INTERVAL>;
void InitSystemAllocator(ByteSpan heap) {
  InitSystemAllocator<WorstFitBlockAllocator>(heap);
}

Allocator* GetSystemAllocator() {
  static WorstFitBlockAllocator allocator;
  return &allocator;
}

}  // namespace pw::malloc
