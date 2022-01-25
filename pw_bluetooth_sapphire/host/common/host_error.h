// Copyright 2022 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_CONNECTIVITY_BLUETOOTH_CORE_BT_HOST_COMMON_HOST_ERROR_H_
#define SRC_CONNECTIVITY_BLUETOOTH_CORE_BT_HOST_COMMON_HOST_ERROR_H_

#include <string>

namespace bt {

// Status types used for internal errors generated by the host
enum class HostError : uint8_t {
  // TODO(fxbug.dev/86900): Remove this enum value alongside bt::Status
  kNoError = 0u,

  // Not found.
  kNotFound,

  // Not ready.
  kNotReady,

  // The time limit for the operation has expired.
  kTimedOut,

  // The operation was initiated with invalid parameters.
  kInvalidParameters,

  // The parameters were rejected by the controller or peer.
  kParametersRejected,

  // An advertising data blob is too large due to controller constraints.
  kAdvertisingDataTooLong,

  // A scan response data blob is too large due to controller constraints.
  kScanResponseTooLong,

  // The operation was canceled.
  kCanceled,

  // Operation is already in progress.
  kInProgress,

  // Operation is not supported by the host.
  kNotSupported,

  // Received an invalid packet from the controller.
  kPacketMalformed,

  // Link was disconnected during operation.
  kLinkDisconnected,

  // Ran out of resources.
  kOutOfMemory,

  // Operation security requirements were not met.
  kInsufficientSecurity,

  // A transaction did not meet reliability requirements (e.g. an ATT Reliable Write)
  kNotReliable,

  // Error code for protocol errors. The actual error code is specified by a
  // protocol error code type.
  //
  // TODO(fxbug.dev/86900): Remove this enum value alongside bt::Status
  kProtocolError,

  // Generic error code. Use this only if another error code does not accurately
  // capture the failure condition.
  kFailed,
};

// Returns a string representation of HostError.
std::string HostErrorToString(HostError error);

}  // namespace bt

#endif  // SRC_CONNECTIVITY_BLUETOOTH_CORE_BT_HOST_COMMON_HOST_ERROR_H_
