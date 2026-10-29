#include <gtest/gtest.h>

#include <cstring>

#include "prc_intranet/const.hpp"
#include "prc_intranet/dispatch.hpp"
#include "prc_intranet/payload.hpp"
#include "prc_intranet/transmit.hpp"

using prc_intranet::constants::MessageId;
using prc_intranet::constants::dlc_for;

TEST(Heartbeat, PayloadIsEightBytes) {
    EXPECT_EQ(sizeof(prc_intranet::payload::heartbeat), 8u);
}

TEST(Heartbeat, IdsSitAtSystemPriority) {
    // (prio << 8) | (node << 4) | idx, System = 5.
    EXPECT_EQ(static_cast<uint16_t>(MessageId::heartbeat_fc),         0x504);
    EXPECT_EQ(static_cast<uint16_t>(MessageId::heartbeat_prc_engine), 0x511);
    EXPECT_EQ(static_cast<uint16_t>(MessageId::heartbeat_dpr_eth),    0x521);
    EXPECT_EQ(static_cast<uint16_t>(MessageId::heartbeat_dpr_lox),    0x531);
}

TEST(Heartbeat, DlcIsEight) {
    EXPECT_EQ(dlc_for(MessageId::heartbeat_fc), 8u);
    EXPECT_EQ(dlc_for(MessageId::heartbeat_prc_engine), 8u);
    EXPECT_EQ(dlc_for(MessageId::heartbeat_dpr_eth), 8u);
    EXPECT_EQ(dlc_for(MessageId::heartbeat_dpr_lox), 8u);
}

namespace {

struct Frame {
    uint16_t can_id = 0;
    uint8_t  data[8] = {};
    uint32_t dlc = 0;
    int      sent = 0;
};

struct Received {
    prc_intranet::payload::heartbeat hb{};
    int calls = 0;
};

void CaptureSend(void* ptr, uint16_t can_id, const uint8_t* buffer, uint32_t dlc) noexcept {
    auto* f = static_cast<Frame*>(ptr);
    f->can_id = can_id;
    f->dlc = dlc;
    std::memcpy(f->data, buffer, dlc);
    f->sent++;
}

void RecordHeartbeatFc(void* ptr, prc_intranet::payload::heartbeat hb) noexcept {
    auto* r = static_cast<Received*>(ptr);
    r->hb = hb;
    r->calls++;
}

} // namespace

TEST(Heartbeat, FcFrameRoundTripsThroughDispatch) {
    Frame frame;
    prc_intranet::prc_driver tx{};
    tx.driver_ptr = &frame;
    tx.send = CaptureSend;
    prc_intranet::context tx_ctx = prc_intranet::create_context(tx);

    prc_intranet::payload::heartbeat out{};
    out.counter   = 0xFFFFFFFEu;
    out.uptime_ms = 123456u;
    prc_intranet::send_heartbeat_fc(&tx_ctx, out);

    ASSERT_EQ(frame.sent, 1);
    EXPECT_EQ(frame.can_id, 0x504);
    EXPECT_EQ(frame.dlc, 8u);

    Received rx_state;
    prc_intranet::prc_driver rx{};
    rx.driver_ptr = &rx_state;
    rx.on_heartbeat_fc = RecordHeartbeatFc;
    prc_intranet::context rx_ctx = prc_intranet::create_context(rx);

    prc_intranet::dispatch_frame(&rx_ctx, frame.can_id, frame.data, frame.dlc);
    ASSERT_EQ(rx_state.calls, 1);
    EXPECT_EQ(rx_state.hb.counter, 0xFFFFFFFEu);
    EXPECT_EQ(rx_state.hb.uptime_ms, 123456u);

    // A board heartbeat id does not reach the FC callback.
    prc_intranet::dispatch_frame(&rx_ctx, 0x511, frame.data, frame.dlc);
    EXPECT_EQ(rx_state.calls, 1);
}
