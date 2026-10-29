#include <gtest/gtest.h>

#include "prc_intranet/const.hpp"
#include "prc_intranet/payload.hpp"

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
