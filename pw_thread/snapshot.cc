// Copyright 2021 The Pigweed Authors
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
#include "pw_thread/snapshot.h"

#include <string_view>

#include "pw_bytes/span.h"
#include "pw_function/function.h"
#include "pw_log/log.h"
#include "pw_protobuf/encoder.h"
#include "pw_status/status.h"
#include "pw_thread_protos/thread.pwpb.h"

namespace pw::thread {

Status SnapshotStack(const StackContext& stack,
                     Thread::StreamEncoder& encoder,
                     ProcessThreadStackCallback& thread_stack_callback) {
  // TODO(pwbug/422): Add support for ascending stacks.
  encoder.WriteStackStartPointer(stack.stack_high_addr);
  encoder.WriteStackEndPointer(stack.stack_low_addr);
  encoder.WriteStackPointer(stack.stack_pointer);
  PW_LOG_INFO("Active stack: 0x%08x-0x%08x (%ld bytes)",
              stack.stack_high_addr,
              stack.stack_pointer,
              static_cast<long>(stack.stack_high_addr) -
                  static_cast<long>(stack.stack_pointer));
  PW_LOG_INFO("Stack Limits: 0x%08x-0x%08x (%ld bytes)",
              stack.stack_low_addr,
              stack.stack_high_addr,
              static_cast<long>(stack.stack_high_addr) -
                  static_cast<long>(stack.stack_low_addr));

  if (stack.stack_pointer > stack.stack_high_addr) {
    PW_LOG_ERROR("%s's stack underflowed by %lu bytes",
                 stack.thread_name.data(),
                 static_cast<long unsigned>(stack.stack_pointer -
                                            stack.stack_high_addr));
    return Status::OutOfRange();
  }

  // Log an error, but don't prevent the capture.
  if (stack.stack_pointer < stack.stack_low_addr) {
    PW_LOG_ERROR(
        "%s's stack overflowed by %lu bytes",
        stack.thread_name.data(),
        static_cast<long unsigned>(stack.stack_low_addr - stack.stack_pointer));
  }

  return thread_stack_callback(
      encoder,
      ConstByteSpan(reinterpret_cast<const std::byte*>(stack.stack_pointer),
                    stack.stack_high_addr - stack.stack_pointer));
}

}  // namespace pw::thread
