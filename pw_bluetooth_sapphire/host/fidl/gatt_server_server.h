// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_CONNECTIVITY_BLUETOOTH_CORE_BT_HOST_FIDL_GATT_SERVER_SERVER_H_
#define SRC_CONNECTIVITY_BLUETOOTH_CORE_BT_HOST_FIDL_GATT_SERVER_SERVER_H_

#include <fuchsia/bluetooth/gatt/cpp/fidl.h>

#include "lib/fidl/cpp/binding.h"
#include "lib/fxl/macros.h"
#include "lib/fxl/memory/weak_ptr.h"

#include "src/connectivity/bluetooth/core/bt-host/fidl/server_base.h"
#include "src/connectivity/bluetooth/core/bt-host/gatt/local_service_manager.h"
#include "src/connectivity/bluetooth/core/bt-host/gatt/types.h"

namespace bthost {

// Implements the gatt::Server FIDL interface.
class GattServerServer
    : public GattServerBase<fuchsia::bluetooth::gatt::Server> {
 public:
  // |adapter_manager| is used to lazily request a handle to the corresponding
  // adapter. It MUST out-live this GattServerServer instance.
  GattServerServer(
      fbl::RefPtr<btlib::gatt::GATT> gatt,
      fidl::InterfaceRequest<fuchsia::bluetooth::gatt::Server> request);

  ~GattServerServer() override;

  // Removes the service with the given |id| if it is known.
  // This can be called as a result of FIDL connection errors (such as handle
  // closure) or as a result of gatt.Service.RemoveService().
  void RemoveService(uint64_t id);

 private:
  class LocalServiceImpl;

  // ::fuchsia::bluetooth::gatt::Server overrides:
  void PublishService(
      fuchsia::bluetooth::gatt::ServiceInfo service_info,
      fidl::InterfaceHandle<fuchsia::bluetooth::gatt::LocalServiceDelegate>
          delegate,
      fidl::InterfaceRequest<fuchsia::bluetooth::gatt::LocalService>
          service_iface,
      PublishServiceCallback callback) override;

  // Called when a remote device issues a read request to one of our services.
  void OnReadRequest(::btlib::gatt::IdType service_id, ::btlib::gatt::IdType id,
                     uint16_t offset, ::btlib::gatt::ReadResponder responder);

  // Called when a remote device issues a write request to one of our services.
  void OnWriteRequest(::btlib::gatt::IdType service_id,
                      ::btlib::gatt::IdType id, uint16_t offset,
                      const ::btlib::common::ByteBuffer& value,
                      ::btlib::gatt::WriteResponder responder);

  // Called when a remote device has configured notifications or indications on
  // a local characteristic.
  void OnCharacteristicConfig(::btlib::gatt::IdType service_id,
                              ::btlib::gatt::IdType chrc_id,
                              ::btlib::gatt::DeviceId peer_id, bool notify,
                              bool indicate);

  // The mapping between service identifiers and FIDL Service implementations.
  // TODO(armansito): Consider using fbl::HashTable.
  std::unordered_map<uint64_t, std::unique_ptr<LocalServiceImpl>> services_;

  // Keep this as the last member to make sure that all weak pointers are
  // invalidated before other members get destroyed.
  fxl::WeakPtrFactory<GattServerServer> weak_ptr_factory_;

  FXL_DISALLOW_COPY_AND_ASSIGN(GattServerServer);
};

}  // namespace bthost

#endif  // SRC_CONNECTIVITY_BLUETOOTH_CORE_BT_HOST_FIDL_GATT_SERVER_SERVER_H_
