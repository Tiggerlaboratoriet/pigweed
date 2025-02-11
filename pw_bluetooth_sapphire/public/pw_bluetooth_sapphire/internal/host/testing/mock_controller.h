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

#ifndef SRC_CONNECTIVITY_BLUETOOTH_CORE_BT_HOST_TESTING_MOCK_CONTROLLER_H_
#define SRC_CONNECTIVITY_BLUETOOTH_CORE_BT_HOST_TESTING_MOCK_CONTROLLER_H_

#include <lib/fit/function.h>

#include <queue>
#include <vector>

#include "pw_bluetooth_sapphire/internal/host/common/byte_buffer.h"
#include "pw_bluetooth_sapphire/internal/host/common/macros.h"
#include "pw_bluetooth_sapphire/internal/host/common/weak_self.h"
#include "pw_bluetooth_sapphire/internal/host/hci-spec/protocol.h"
#include "pw_bluetooth_sapphire/internal/host/testing/controller_test_double_base.h"

namespace bt::testing {

struct ExpectationMetadata {
  ExpectationMetadata(const char* file, int line, const char* expectation)
      : file(file), line(line), expectation(expectation) {}
  const char* file;
  int line;
  // String inside of the expectation expression:
  // EXPECT_ACL_PACKET_OUT(expectation, ...).
  const char* expectation;
};

struct PacketExpectation {
  DynamicByteBuffer data;
  ExpectationMetadata meta;
};

class Transaction {
 public:
  // The |expected| buffer and the buffers in |replies| will be copied, so their
  // lifetime does not need to extend past Transaction construction.
  Transaction(const ByteBuffer& expected,
              const std::vector<const ByteBuffer*>& replies,
              ExpectationMetadata meta);
  virtual ~Transaction() = default;
  Transaction(Transaction&& other) = default;
  Transaction& operator=(Transaction&& other) = default;

  // Returns true if the transaction matches the given HCI packet.
  virtual bool Match(const ByteBuffer& packet);

  const PacketExpectation& expected() { return expected_; }
  void set_expected(const PacketExpectation& expected) {
    expected_ = PacketExpectation{.data = DynamicByteBuffer(expected.data),
                                  .meta = expected.meta};
  }

  std::queue<DynamicByteBuffer>& replies() { return replies_; }

 private:
  PacketExpectation expected_;
  std::queue<DynamicByteBuffer> replies_;

  BT_DISALLOW_COPY_AND_ASSIGN_ALLOW_MOVE(Transaction);
};

// A CommandTransaction is used to set up an expectation for a command channel
// packet and the events that should be sent back in response to it.
class CommandTransaction final : public Transaction {
 public:
  CommandTransaction(const ByteBuffer& expected,
                     const std::vector<const ByteBuffer*>& replies,
                     ExpectationMetadata meta)
      : Transaction(expected, replies, meta), prefix_(false) {}

  // Match by opcode only.
  CommandTransaction(hci_spec::OpCode expected_opcode,
                     const std::vector<const ByteBuffer*>& replies,
                     ExpectationMetadata meta);

  // Move constructor and assignment operator.
  CommandTransaction(CommandTransaction&& other) = default;
  CommandTransaction& operator=(CommandTransaction&& other) = default;

  // Returns true if the transaction matches the given HCI command packet.
  bool Match(const ByteBuffer& cmd) override;

 private:
  bool prefix_ = false;
};

// A DataTransaction is used to set up an expectation for an acl data channel
class DataTransaction final : public Transaction {
 public:
  DataTransaction(const ByteBuffer& expected,
                  const std::vector<const ByteBuffer*>& replies,
                  ExpectationMetadata meta)
      : Transaction(expected, replies, meta) {}
  DataTransaction(DataTransaction&& other) = default;
  DataTransaction& operator=(DataTransaction&& other) = default;
};

// A ScoTransaction is used to set up an expectation for a SCO data channel
// SCO packets don't have a concept of "replies".
class ScoTransaction final : public Transaction {
 public:
  ScoTransaction(const ByteBuffer& expected, ExpectationMetadata meta)
      : Transaction(expected, /*replies=*/{}, meta) {}
  ScoTransaction(ScoTransaction&& other) = default;
  ScoTransaction& operator=(ScoTransaction&& other) = default;
};

// Helper macro for expecting a data packet and specifying a variable number of
// responses that the MockController should send in response to the expected
// packet.
#define EXPECT_ACL_PACKET_OUT(device, expected, ...) \
  (device)->QueueDataTransaction(                    \
      (expected),                                    \
      {__VA_ARGS__},                                 \
      bt::testing::ExpectationMetadata(__FILE__, __LINE__, #expected))

// Helper macro for expecting a SCO packet.
#define EXPECT_SCO_PACKET_OUT(device, expected) \
  (device)->QueueScoTransaction(                \
      (expected),                               \
      bt::testing::ExpectationMetadata(__FILE__, __LINE__, #expected))

// Helper macro for expecting a command packet and receiving a variable number
// of responses.
#define EXPECT_CMD_PACKET_OUT(device, expected, ...) \
  (device)->QueueCommandTransaction(                 \
      (expected),                                    \
      {__VA_ARGS__},                                 \
      bt::testing::ExpectationMetadata(__FILE__, __LINE__, #expected))

// MockController allows unit tests to set up an expected sequence of HCI
// command packets and ACL data packets and any packets that should be sent back
// in response. The code internally verifies each received packet using gtest
// ASSERT_* macros.
class MockController final : public ControllerTestDoubleBase,
                             public WeakSelf<MockController> {
 public:
  explicit MockController(pw::async::Dispatcher& pw_dispatcher);
  ~MockController() override;

  // Queues a transaction into the MockController's expected command queue. Each
  // packet received through the command channel endpoint will be verified
  // against the next expected transaction in the queue. A mismatch will cause a
  // fatal assertion. On a match, MockController will send back the replies
  // provided in the transaction.
  void QueueCommandTransaction(CommandTransaction transaction);
  void QueueCommandTransaction(const ByteBuffer& expected,
                               const std::vector<const ByteBuffer*>& replies,
                               ExpectationMetadata);
  void QueueCommandTransaction(hci_spec::OpCode expected_opcode,
                               const std::vector<const ByteBuffer*>& replies,
                               ExpectationMetadata meta);

  // Queues a transaction into the MockController's expected ACL data queue.
  // Each packet received through the ACL data channel endpoint will be verified
  // against the next expected transaction in the queue. A mismatch will cause a
  // fatal assertion. On a match, MockController will send back the replies
  // provided in the transaction.
  void QueueDataTransaction(DataTransaction transaction);
  void QueueDataTransaction(const ByteBuffer& expected,
                            const std::vector<const ByteBuffer*>& replies,
                            ExpectationMetadata meta);

  // Queues a transaction into the MockController's expected SCO packet queue.
  // Each packet received through the SCO data channel endpoint will be verified
  // against the next expected transaction in the queue. A mismatch will cause a
  // fatal assertion.
  void QueueScoTransaction(const ByteBuffer& expected,
                           ExpectationMetadata meta);

  // Returns true iff all transactions queued with QueueScoTransaction() have
  // been received.
  bool AllExpectedScoPacketsSent() const;

  // Returns true iff all transactions queued with QueueDataTransaction() have
  // been received.
  bool AllExpectedDataPacketsSent() const;

  // Returns true iff all transactions queued with QueueCommandTransaction()
  // have been received.
  bool AllExpectedCommandPacketsSent() const;

  // Callback to invoke when a packet is received over the data channel. Care
  // should be taken to ensure that a callback with a reference to test case
  // variables is not invoked when tearing down.
  using DataCallback = fit::function<void(const ByteBuffer& packet)>;
  void SetDataCallback(DataCallback callback);
  void ClearDataCallback();

  // Callback invoked when a transaction completes. Care should be taken to
  // ensure that a callback with a reference to test case variables is not
  // invoked when tearing down.
  using TransactionCallback = fit::function<void(const ByteBuffer& rx)>;
  void SetTransactionCallback(TransactionCallback callback);
  void SetTransactionCallback(fit::closure callback);
  void ClearTransactionCallback();

 private:
  void OnCommandReceived(const ByteBuffer& data);
  void OnACLDataPacketReceived(const ByteBuffer& acl_data_packet);
  void OnScoDataPacketReceived(const ByteBuffer& sco_data_packet);

  // Controller overrides:
  void SendCommand(pw::span<const std::byte> data) override;
  void SendAclData(pw::span<const std::byte> data) override;
  void SendScoData(pw::span<const std::byte> data) override;

  std::queue<CommandTransaction> cmd_transactions_;
  std::queue<DataTransaction> data_transactions_;
  std::queue<ScoTransaction> sco_transactions_;
  DataCallback data_callback_;
  TransactionCallback transaction_callback_;

  BT_DISALLOW_COPY_AND_ASSIGN_ALLOW_MOVE(MockController);
};

}  // namespace bt::testing

#endif  // SRC_CONNECTIVITY_BLUETOOTH_CORE_BT_HOST_TESTING_MOCK_CONTROLLER_H_
