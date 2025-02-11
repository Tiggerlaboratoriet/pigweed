// Copyright 2023 The Pigweed Authors
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

#ifndef SRC_CONNECTIVITY_BLUETOOTH_CORE_BT_HOST_COMMON_TRACE_H_
#define SRC_CONNECTIVITY_BLUETOOTH_CORE_BT_HOST_COMMON_TRACE_H_

#include <cstdint>

#include "pw_bluetooth_sapphire/config.h"

#ifndef NTRACE
#include <lib/trace/event.h>
#else
typedef uint64_t trace_flow_id_t;
#define TRACE_NONCE() (0u)
#define TRACE_ENABLED() (false)
#define TRACE_FLOW_BEGIN(...)
#define TRACE_FLOW_END(...)
#define TRACE_DURATION(...)
#endif  // NTRACE

#endif  // SRC_CONNECTIVITY_BLUETOOTH_CORE_BT_HOST_COMMON_TRACE_H_
