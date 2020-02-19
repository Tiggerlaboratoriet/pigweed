#include "test_packets.h"

#include "src/connectivity/bluetooth/core/bt-host/common/test_helpers.h"

namespace bt::l2cap::testing {

DynamicByteBuffer AclCommandRejectNotUnderstoodRsp(l2cap::CommandId id,
                                                   hci::ConnectionHandle handle) {
  return DynamicByteBuffer(StaticByteBuffer(
      // ACL data header (handle: |link_handle|, length: 10 bytes)
      LowerBits(handle), UpperBits(handle), 0x0a, 0x00,
      // L2CAP B-frame header (length: 6 bytes, channel-id: 0x0001 (ACL sig))
      0x06, 0x00, 0x01, 0x00,
      // Information Response (type, ID, length: 2)
      l2cap::kCommandRejectCode, id, 0x02, 0x00,
      // Reason = Not Understood
      LowerBits(static_cast<uint16_t>(RejectReason::kNotUnderstood)),
      UpperBits(static_cast<uint16_t>(RejectReason::kNotUnderstood))));
}

DynamicByteBuffer AclExtFeaturesInfoRsp(l2cap::CommandId id, hci::ConnectionHandle handle,
                                        l2cap::ExtendedFeatures features) {
  const auto features_bytes = ToBytes(features);
  return DynamicByteBuffer(StaticByteBuffer(
      // ACL data header (handle: |link_handle|, length: 16 bytes)
      LowerBits(handle), UpperBits(handle), 0x10, 0x00,
      // L2CAP B-frame header (length: 12 bytes, channel-id: 0x0001 (ACL sig))
      0x0c, 0x00, 0x01, 0x00,
      // Information Response (type, ID, length: 8)
      l2cap::kInformationResponse, id, 0x08, 0x00,
      // Type = Features Mask
      0x02, 0x00,
      // Result = Success
      0x00, 0x00,
      // Data (Mask)
      features_bytes[0], features_bytes[1], features_bytes[2], features_bytes[3]));
}

DynamicByteBuffer AclFixedChannelsSupportedInfoReq(l2cap::CommandId id,
                                                   hci::ConnectionHandle handle) {
  return DynamicByteBuffer(StaticByteBuffer(
      // ACL data header (handle, length: 10)
      LowerBits(handle), UpperBits(handle), 0x0a, 0x00,

      // L2CAP B-frame header (length: 6, chanel-id: 0x0001 (ACL sig))
      0x06, 0x00, 0x01, 0x00,

      // Fixed Channels Supported Information Request
      // (ID, length: 2, info type)
      l2cap::kInformationRequest, id, 0x02, 0x00,
      LowerBits(static_cast<uint16_t>(InformationType::kFixedChannelsSupported)),
      UpperBits(static_cast<uint16_t>(InformationType::kFixedChannelsSupported))));
}

DynamicByteBuffer AclFixedChannelsSupportedInfoRsp(l2cap::CommandId id,
                                                   hci::ConnectionHandle handle,
                                                   l2cap::FixedChannelsSupported chan_mask) {
  const auto chan_bytes = ToBytes(chan_mask);
  return DynamicByteBuffer(StaticByteBuffer(
      // ACL data header (handle: |link_handle|, length: 20 bytes)
      LowerBits(handle), UpperBits(handle), 0x14, 0x00,
      // L2CAP B-frame header (length: 16 bytes, channel-id: 0x0001 (ACL sig))
      0x10, 0x00, 0x01, 0x00,
      // Information Response (type, ID, length: 12)
      l2cap::kInformationResponse, id, 0x0c, 0x00,
      // Type = Fixed Channels Supported
      0x03, 0x00,
      // Result = Success
      0x00, 0x00,
      // Data (Mask)
      chan_bytes[0], chan_bytes[1], chan_bytes[2], chan_bytes[3], chan_bytes[4], chan_bytes[5],
      chan_bytes[6], chan_bytes[7]));
}

DynamicByteBuffer AclNotSupportedInformationResponse(l2cap::CommandId id,
                                                     hci::ConnectionHandle handle) {
  return DynamicByteBuffer(StaticByteBuffer(
      // ACL data header (handle: |link_handle|, length: 12 bytes)
      LowerBits(handle), UpperBits(handle), 0x0c, 0x00,
      // L2CAP B-frame header (length: 8 bytes, channel-id: 0x0001 (ACL sig))
      0x08, 0x00, 0x01, 0x00,
      // Information Response (type, ID, length: 4)
      l2cap::kInformationResponse, id, 0x04, 0x00,
      // Type = invalid type
      0xFF, 0xFF,
      // Result
      LowerBits(static_cast<uint16_t>(l2cap::InformationResult::kNotSupported)),
      UpperBits(static_cast<uint16_t>(l2cap::InformationResult::kNotSupported))));
}

DynamicByteBuffer AclConfigReq(l2cap::CommandId id, hci::ConnectionHandle handle,
                               l2cap::ChannelId dst_id, uint16_t mtu, l2cap::ChannelMode mode) {
  return DynamicByteBuffer(StaticByteBuffer(
      // ACL data header (handle, length: 27 bytes)
      LowerBits(handle), UpperBits(handle), 0x1b, 0x00,
      // L2CAP B-frame header (length: 23, channel-id: 0x0001 (ACL sig))
      0x17, 0x00, 0x01, 0x00,
      // Configuration Request (ID, length: 19, |dst_id|, flags: 0,
      0x04, id, 0x13, 0x00, LowerBits(dst_id), UpperBits(dst_id), 0x00, 0x00,
      // option: [type: MTU, length: 2, MTU: 1024])
      0x01, 0x02, LowerBits(mtu), UpperBits(mtu),
      // option: [type: Retransmission and Flow Control, length: 9, mode, parameters]
      0x04, 0x09, static_cast<uint8_t>(mode), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00));
}

DynamicByteBuffer AclConfigRsp(l2cap::CommandId id, hci::ConnectionHandle link_handle,
                               l2cap::ChannelId src_id) {
  return DynamicByteBuffer(StaticByteBuffer(
      // ACL data header (handle: |link_handle|, length: 14 bytes)
      LowerBits(link_handle), UpperBits(link_handle), 0x0e, 0x00,

      // L2CAP B-frame header (length: 10 bytes, channel-id: 0x0001 (ACL sig))
      0x0a, 0x00, 0x01, 0x00,

      // Configuration Response (ID, length: 6, src cid, flags: 0,
      // result: success)
      0x05, id, 0x06, 0x00, LowerBits(src_id), UpperBits(src_id), 0x00, 0x00, 0x00, 0x00));
}

DynamicByteBuffer AclConnectionReq(l2cap::CommandId id, hci::ConnectionHandle link_handle,
                                   l2cap::ChannelId src_id, l2cap::PSM psm) {
  return DynamicByteBuffer(StaticByteBuffer(
      // ACL data header (handle: |link_handle|, length: 12 bytes)
      LowerBits(link_handle), UpperBits(link_handle), 0x0c, 0x00,

      // L2CAP B-frame header (length: 8 bytes, channel-id: 0x0001 (ACL sig))
      0x08, 0x00, 0x01, 0x00,

      // Connection Request (ID, length: 4, |psm|, |src_id|)
      0x02, id, 0x04, 0x00, LowerBits(psm), UpperBits(psm), LowerBits(src_id), UpperBits(src_id)));
}

DynamicByteBuffer AclConnectionRsp(l2cap::CommandId id, hci::ConnectionHandle link_handle,
                                   l2cap::ChannelId src_id, l2cap::ChannelId dst_id) {
  return DynamicByteBuffer(StaticByteBuffer(
      // ACL data header (handle: |link handle|, length: 16 bytes)
      LowerBits(link_handle), UpperBits(link_handle), 0x10, 0x00,
      // L2CAP B-frame header: length 12, channel-id 1 (signaling)
      0x0c, 0x00, 0x01, 0x00,
      // Connection Response (0x03), id, length 8
      l2cap::kConnectionResponse, id, 0x08, 0x00,
      // destination cid
      LowerBits(dst_id), UpperBits(dst_id),
      // source cid
      LowerBits(src_id), UpperBits(src_id),
      // Result (success), status
      0x00, 0x00, 0x00, 0x00));
}

}  // namespace bt::l2cap::testing
