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
#include "pw_bluetooth_sapphire/internal/host/transport/emboss_packet.h"

#include <gtest/gtest.h>
#include <pw_bluetooth/hci_commands.emb.h>
#include <pw_bluetooth/hci_test.emb.h>
#include <pw_bluetooth/hci_vendor.emb.h>

#include "pw_bluetooth_sapphire/internal/host/common/byte_buffer.h"
#include "pw_bluetooth_sapphire/internal/host/hci-spec/vendor_protocol.h"
#include "pw_bluetooth_sapphire/internal/host/transport/emboss_control_packets.h"

namespace bt::hci {
namespace {

TEST(StaticPacketTest, StaticPacketBasic) {
  StaticPacket<pw::bluetooth::emboss::TestCommandPacketWriter> packet;
  packet.view().header().opcode().BackingStorage().WriteUInt(1234);
  packet.view().header().parameter_total_size().Write(1);
  packet.view().payload().Write(13);

  EXPECT_EQ(packet.data(), BufferView({0xD2, 0x04, 0x01, 0x0D}));

  packet.SetToZeros();
  EXPECT_EQ(packet.data(), BufferView({0, 0, 0, 0}));
}

TEST(EmbossCommandPacketTest, EmbossCommandPacketBasic) {
  auto packet =
      EmbossCommandPacket::New<pw::bluetooth::emboss::TestCommandPacketWriter>(
          1234);
  packet.view_t().payload().Write(13);

  EXPECT_EQ(packet.size(), 4u);
  EXPECT_EQ(packet.data(), BufferView({0xD2, 0x04, 0x01, 0x0D}));
  EXPECT_EQ(packet.mutable_data(), packet.data());
  EXPECT_EQ(packet.opcode(), 1234);
  EXPECT_EQ(packet.ocf(), 1234 & 0x3FF);
  EXPECT_EQ(packet.ogf(), 1234 >> 10);
  EXPECT_EQ(packet.view_t().payload().Read(), 13);
}

TEST(EmbossCommandPacketTest, EmbossCommandPacketDeathTest) {
  EmbossCommandPacket packet =
      EmbossCommandPacket::New<pw::bluetooth::emboss::TestCommandPacketView>(
          1234);

  // Try and fail to request view for struct larger than TestCommandPacket.
  EXPECT_DEATH_IF_SUPPORTED(
      packet.view<pw::bluetooth::emboss::InquiryCommandView>(),
      "emboss packet buffer not large enough");
  // Try and fail to allocate 0 length packet (needs at least 3 bytes for the
  // header).
  EXPECT_DEATH_IF_SUPPORTED(
      EmbossCommandPacket::New<pw::bluetooth::emboss::CommandHeaderView>(1234,
                                                                         0),
      "command packet size must be at least 3 bytes");
}

TEST(EmbossEventPacketTest, EmbossEventPacketBasic) {
  auto packet =
      EmbossEventPacket::New<pw::bluetooth::emboss::TestEventPacketWriter>(123);
  packet.view_t().payload().Write(13);

  EXPECT_EQ(packet.size(), 3u);
  EXPECT_EQ(packet.data(), BufferView({0x7B, 0x01, 0x0D}));
  EXPECT_EQ(packet.mutable_data(), packet.data());
  EXPECT_EQ(packet.event_code(), 123);
  EXPECT_EQ(packet.view_t().payload().Read(), 13);
}

TEST(EmbossEventPacketTest, EmbossEventPacketDeathTest) {
  EmbossEventPacket packet =
      EmbossEventPacket::New<pw::bluetooth::emboss::TestEventPacketView>(123);

  // Try and fail to allocate 0 length packet (needs at least 2 bytes for the
  // header).
  EXPECT_DEATH_IF_SUPPORTED(EmbossEventPacket::New(0),
                            "event packet size must be at least 2 bytes");
}

TEST(EmbossEventPacketTest, StatusCode) {
  // Confirm status can be read from vendor subevent.
  auto packet = EmbossEventPacket::New<
      pw::bluetooth::vendor::android_hci::LEMultiAdvtStateChangeSubeventWriter>(
      hci_spec::kVendorDebugEventCode);
  auto view = packet.view_t();
  view.status().Write(hci_spec::StatusCode::OPERATION_CANCELLED_BY_HOST);
  view.vendor_event().subevent_code().Write(
      hci_spec::vendor::android::kLEMultiAdvtStateChangeSubeventCode);

  ASSERT_TRUE(packet.StatusCode().has_value());
  EXPECT_EQ(packet.StatusCode().value(),
            hci_spec::StatusCode::OPERATION_CANCELLED_BY_HOST);
  EXPECT_EQ(packet.ToResult(),
            ToResult(hci_spec::StatusCode::OPERATION_CANCELLED_BY_HOST));
}

}  // namespace
}  // namespace bt::hci
