#include <gtest/gtest.h>

#include <cstring>

#include "prc_intranet/const.hpp"
#include "prc_intranet/dispatch.hpp"
#include "prc_intranet/payload.hpp"
#include "prc_intranet/transmit.hpp"

using prc_intranet::constants::MessageId;
using prc_intranet::constants::dlc_for;
using prc_intranet::payload::heartbeat;

TEST(Heartbeat, PayloadIsEightBytes) {
    EXPECT_EQ(sizeof(heartbeat), 8u);
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

TEST(Heartbeat, LinkBitsMatchTheMatrixLayout) {
    using namespace prc_intranet::constants;
    EXPECT_EQ(LINK_BIT_FC, 0);
    EXPECT_EQ(LINK_BIT_ENGINE, 1);
    EXPECT_EQ(LINK_BIT_ETH, 2);
    EXPECT_EQ(LINK_BIT_LOX, 3);
    EXPECT_EQ(LINK_BOARD_COUNT, 4);
}

namespace {

struct Frame {
    uint16_t can_id = 0;
    uint8_t  data[8] = {};
    uint32_t dlc = 0;
    int      sent = 0;
};

struct Received {
    heartbeat hb{};
    int calls = 0;
};

void CaptureSend(void* ptr, uint16_t can_id, const uint8_t* buffer, uint32_t dlc) noexcept {
    auto* f = static_cast<Frame*>(ptr);
    f->can_id = can_id;
    f->dlc = dlc;
    std::memcpy(f->data, buffer, dlc);
    f->sent++;
}

void Record(void* ptr, heartbeat hb) noexcept {
    auto* r = static_cast<Received*>(ptr);
    r->hb = hb;
    r->calls++;
}

heartbeat Sample() {
    heartbeat hb{};
    hb.uptime_ms  = 0x12345678u;
    hb.counter    = 0xFFFEu;
    hb.alive_mask = 0x0Bu;   // hears FC, Engine, LOX
    hb.reserved   = 0;
    return hb;
}

Frame Send(void (*send)(prc_intranet::context*, heartbeat), heartbeat hb) {
    Frame frame;
    prc_intranet::prc_driver tx{};
    tx.driver_ptr = &frame;
    tx.send = CaptureSend;
    prc_intranet::context ctx = prc_intranet::create_context(tx);
    send(&ctx, hb);
    return frame;
}

} // namespace

// Wire layout, little endian on both STM32 and the host.
TEST(Heartbeat, WireLayout) {
    const Frame f = Send(prc_intranet::send_heartbeat_fc, Sample());
    ASSERT_EQ(f.sent, 1);
    ASSERT_EQ(f.dlc, 8u);
    const uint8_t expected[8] = {0x78, 0x56, 0x34, 0x12,  // uptime_ms
                                 0xFE, 0xFF,              // counter
                                 0x0B,                    // alive_mask
                                 0x00};                   // reserved
    for (int i = 0; i < 8; i++) EXPECT_EQ(f.data[i], expected[i]) << "byte " << i;
}

TEST(Heartbeat, FcFrameRoundTripsThroughDispatch) {
    const Frame frame = Send(prc_intranet::send_heartbeat_fc, Sample());
    EXPECT_EQ(frame.can_id, 0x504);

    Received rx_state;
    prc_intranet::prc_driver rx{};
    rx.driver_ptr = &rx_state;
    rx.on_heartbeat_fc = Record;
    prc_intranet::context rx_ctx = prc_intranet::create_context(rx);

    prc_intranet::dispatch_frame(&rx_ctx, frame.can_id, frame.data, frame.dlc);
    ASSERT_EQ(rx_state.calls, 1);
    EXPECT_EQ(rx_state.hb.uptime_ms, 0x12345678u);
    EXPECT_EQ(rx_state.hb.counter, 0xFFFEu);
    EXPECT_EQ(rx_state.hb.alive_mask, 0x0Bu);
    EXPECT_EQ(rx_state.hb.reserved, 0u);

    // A board heartbeat id does not reach the FC callback.
    prc_intranet::dispatch_frame(&rx_ctx, 0x511, frame.data, frame.dlc);
    EXPECT_EQ(rx_state.calls, 1);
}

// Each PRC heartbeat goes out on its own id and reaches only its own
// callback: the PRC boards now listen to each other.
TEST(Heartbeat, PrcFramesReachTheirOwnCallback) {
    Received engine, eth, lox;
    prc_intranet::prc_driver rx{};
    rx.on_heartbeat_prc_engine = Record;
    rx.on_heartbeat_dpr_eth    = Record;
    rx.on_heartbeat_dpr_lox    = Record;

    struct Case {
        void (*send)(prc_intranet::context*, heartbeat);
        uint16_t  id;
        Received* target;
    };
    const Case cases[] = {
        {prc_intranet::send_heartbeat_prc_engine, 0x511, &engine},
        {prc_intranet::send_heartbeat_dpr_eth,    0x521, &eth},
        {prc_intranet::send_heartbeat_dpr_lox,    0x531, &lox},
    };

    for (const Case& c : cases) {
        const Frame f = Send(c.send, Sample());
        EXPECT_EQ(f.can_id, c.id);
        rx.driver_ptr = c.target;  // every callback records into the expected board
        prc_intranet::context ctx = prc_intranet::create_context(rx);
        prc_intranet::dispatch_frame(&ctx, f.can_id, f.data, f.dlc);
        EXPECT_EQ(c.target->hb.alive_mask, 0x0Bu);
    }
    EXPECT_EQ(engine.calls, 1);
    EXPECT_EQ(eth.calls, 1);
    EXPECT_EQ(lox.calls, 1);
}

TEST(Heartbeat, ShortFrameIsIgnored) {
    const Frame frame = Send(prc_intranet::send_heartbeat_dpr_eth, Sample());
    Received r;
    prc_intranet::prc_driver rx{};
    rx.driver_ptr = &r;
    rx.on_heartbeat_dpr_eth = Record;
    prc_intranet::context ctx = prc_intranet::create_context(rx);
    prc_intranet::dispatch_frame(&ctx, frame.can_id, frame.data, 7);
    EXPECT_EQ(r.calls, 0);
}
