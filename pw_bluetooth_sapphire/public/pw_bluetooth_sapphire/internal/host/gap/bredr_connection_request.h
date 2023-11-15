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

#ifndef SRC_CONNECTIVITY_BLUETOOTH_CORE_BT_HOST_GAP_BREDR_CONNECTION_REQUEST_H_
#define SRC_CONNECTIVITY_BLUETOOTH_CORE_BT_HOST_GAP_BREDR_CONNECTION_REQUEST_H_

#include <lib/fit/function.h>

#include <list>

#include "pw_bluetooth_sapphire/internal/host/common/identifier.h"
#include "pw_bluetooth_sapphire/internal/host/common/inspectable.h"
#include "pw_bluetooth_sapphire/internal/host/gap/peer.h"
#include "pw_bluetooth_sapphire/internal/host/hci-spec/protocol.h"
#include "pw_bluetooth_sapphire/internal/host/transport/error.h"

namespace bt::gap {

class BrEdrConnection;

// A |BrEdrConnectionRequest| represents a request for the GAP to connect to a
// given |DeviceAddress| by one or more clients. BrEdrConnectionManager is
// responsible for tracking ConnectionRequests and passing them to the Connector
// when ready.
//
// There is at most One BrEdrConnectionRequest per address at any given time; if
// multiple clients wish to connect, they each append a callback to the list in
// the ConnectionRequest for the device they are interested in.
//
// If a remote peer makes an incoming request for a connection, we track that
// here also - whether an incoming request is pending is indicated by
// HasIncoming()
class BrEdrConnectionRequest final {
 public:
  using OnComplete = fit::function<void(hci::Result<>, BrEdrConnection*)>;
  using RefFactory = fit::function<BrEdrConnection*()>;

  // Construct without a callback. Can be used for incoming only requests
  BrEdrConnectionRequest(pw::async::Dispatcher& pw_dispatcher,
                         const DeviceAddress& addr,
                         PeerId peer_id,
                         Peer::InitializingConnectionToken token);

  BrEdrConnectionRequest(pw::async::Dispatcher& pw_dispatcher,
                         const DeviceAddress& addr,
                         PeerId peer_id,
                         Peer::InitializingConnectionToken token,
                         OnComplete&& callback);

  BrEdrConnectionRequest(BrEdrConnectionRequest&&) = default;

  void RecordHciCreateConnectionAttempt();
  bool ShouldRetry(hci::Error failure_mode);

  void AddCallback(OnComplete cb) {
    callbacks_.Mutable()->push_back(std::move(cb));
  }

  // Notifies all elements in |callbacks| with |status| and the result of
  // |generate_ref|. Called by the appropriate manager once a connection request
  // has completed, successfully or otherwise
  void NotifyCallbacks(hci::Result<> status, const RefFactory& generate_ref);

  void BeginIncoming() { has_incoming_.Set(true); }
  void CompleteIncoming() { has_incoming_.Set(false); }
  bool HasIncoming() const { return *has_incoming_; }
  bool AwaitingOutgoing() { return !callbacks_->empty(); }

  // Attach request inspect node as a child of |parent| named |name|.
  void AttachInspect(inspect::Node& parent, std::string name);

  DeviceAddress address() const { return address_; }

  // If a role change occurs while this request is still pending, set it here so
  // that the correct role is used when connection establishment completes.
  void set_role_change(pw::bluetooth::emboss::ConnectionRole role) {
    role_change_ = role;
  }

  // If the default role of the requested connection is changed during
  // connection establishment, the new role will be returned.
  const std::optional<pw::bluetooth::emboss::ConnectionRole>& role_change()
      const {
    return role_change_;
  }

  Peer::InitializingConnectionToken take_peer_init_token() {
    BT_ASSERT(peer_init_conn_token_);
    return std::exchange(peer_init_conn_token_, std::nullopt).value();
  }

 private:
  PeerId peer_id_;
  DeviceAddress address_;
  UintInspectable<std::list<OnComplete>> callbacks_;
  BoolInspectable<bool> has_incoming_{false};
  std::optional<pw::bluetooth::emboss::ConnectionRole> role_change_;
  // Used to determine whether an outbound connection request should be retried.
  // If empty, no HCI Create Connection Requests associated with this object
  // have been made, otherwise stores the time at which the first HCI request
  // associated with this object was made.
  IntInspectable<std::optional<pw::chrono::SystemClock::time_point>>
      first_create_connection_req_made_{
          std::nullopt,
          [](auto& t) { return t ? t->time_since_epoch().count() : -1; }};

  inspect::StringProperty peer_id_property_;
  inspect::Node inspect_node_;

  std::optional<Peer::InitializingConnectionToken> peer_init_conn_token_;

  pw::async::Dispatcher& dispatcher_;

  BT_DISALLOW_COPY_AND_ASSIGN_ALLOW_MOVE(BrEdrConnectionRequest);
};

}  // namespace bt::gap

#endif  // SRC_CONNECTIVITY_BLUETOOTH_CORE_BT_HOST_GAP_BREDR_CONNECTION_REQUEST_H_
