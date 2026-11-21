#include <gtest/gtest.h>

#include "prc_intranet/const.hpp"
#include "prc_intranet/link_matrix.hpp"

using prc_intranet::link::LinkMatrix;
using prc_intranet::payload::heartbeat;
using namespace prc_intranet::constants;

namespace {

constexpr uint8_t FC  = LINK_BIT_FC;
constexpr uint8_t ENG = LINK_BIT_ENGINE;
constexpr uint8_t ETH = LINK_BIT_ETH;
constexpr uint8_t LOX = LINK_BIT_LOX;

heartbeat Beat(uint32_t uptime_ms, uint16_t counter, uint8_t alive_mask = 0) {
    heartbeat hb{};
    hb.uptime_ms  = uptime_ms;
    hb.counter    = counter;
    hb.alive_mask = alive_mask;
    return hb;
}

} // namespace

// --- Timeout -----------------------------------------------------------------

TEST(LinkMatrix, OnlySelfAliveBeforeAnyBeat) {
    LinkMatrix m(FC);
    EXPECT_TRUE(m.alive(FC, 0));
    EXPECT_FALSE(m.alive(ENG, 0));
    EXPECT_EQ(m.ownRow(0), 0x1u);
}

TEST(LinkMatrix, AliveUntil500ms) {
    LinkMatrix m(FC);
    m.onHeartbeat(ENG, Beat(10000, 0), 1000);
    EXPECT_TRUE(m.alive(ENG, 1000));
    EXPECT_TRUE(m.alive(ENG, 1499));
    EXPECT_FALSE(m.alive(ENG, 1500));
    m.onHeartbeat(ENG, Beat(10600, 12), 1600);
    EXPECT_TRUE(m.alive(ENG, 1600));
}

TEST(LinkMatrix, ClockWrapKeepsLiveBoardAlive) {
    LinkMatrix m(FC);
    m.onHeartbeat(ETH, Beat(5000, 1), 0xFFFFFF00u);
    EXPECT_TRUE(m.alive(ETH, 0x00000010u));    // 272 ms later
    EXPECT_FALSE(m.alive(ETH, 0x00000200u));   // 768 ms later
}

TEST(LinkMatrix, OwnAndOutOfRangeBeatsAreIgnored) {
    LinkMatrix m(ENG);
    m.onHeartbeat(ENG, Beat(1000, 0), 1000);   // own id looped back
    m.onHeartbeat(7,   Beat(1000, 0), 1000);   // no such board
    EXPECT_EQ(m.peer(ENG).received, 0u);
    EXPECT_EQ(m.ownRow(1000), 0x2u);
}

// --- Rows and matrix ---------------------------------------------------------

TEST(LinkMatrix, OwnRowHasSelfAndLiveBoards) {
    LinkMatrix m(ETH);
    m.onHeartbeat(FC,  Beat(1000, 0), 100);
    m.onHeartbeat(LOX, Beat(1000, 0), 400);
    EXPECT_EQ(m.ownRow(450), 0xDu);            // LOX, ETH, FC
    EXPECT_EQ(m.ownRow(650), 0xCu);            // FC timed out
}

TEST(LinkMatrix, MatrixAssemblesRows) {
    LinkMatrix m(FC);
    m.onHeartbeat(ENG, Beat(1000, 0, 0x3), 100);   // engine hears FC and itself
    m.onHeartbeat(ETH, Beat(1000, 0, 0xF), 100);   // eth hears everyone
    m.onHeartbeat(LOX, Beat(1000, 0, 0x9), 100);   // lox hears FC and itself
    // FC row 0xF (bits 0-3), engine 0x3 (4-7), eth 0xF (8-11), lox 0x9 (12-15).
    EXPECT_EQ(m.matrix(200), 0x9F3Fu);
    EXPECT_EQ(m.row(ENG, 200), 0x3u);
}

TEST(LinkMatrix, SilentBoardRowIsZero) {
    LinkMatrix m(FC);
    m.onHeartbeat(ENG, Beat(1000, 0, 0x3), 100);
    m.onHeartbeat(ETH, Beat(1000, 0, 0x5), 500);
    // At 700 the engine is silent for 600 ms: its row and FC bit 1 go to 0.
    EXPECT_EQ(m.row(ENG, 700), 0u);
    EXPECT_EQ(m.matrix(700), 0x0505u);             // eth row 0x5, FC row 0x5
}

TEST(LinkMatrix, NeverHeardBoardRowIsZero) {
    LinkMatrix m(FC);
    EXPECT_EQ(m.matrix(0), 0x0001u);               // only the FC's own bit
}

TEST(LinkMatrix, ReceivedRowIsMaskedToFourBits) {
    LinkMatrix m(FC);
    m.onHeartbeat(LOX, Beat(1000, 0, 0xF9), 100);
    EXPECT_EQ(m.row(LOX, 100), 0x9u);
}

// --- Lost beats --------------------------------------------------------------

TEST(LinkMatrix, MissedCountsCounterJumps) {
    LinkMatrix m(FC);
    m.onHeartbeat(ENG, Beat(1000, 10), 0);
    m.onHeartbeat(ENG, Beat(1050, 11), 50);
    m.onHeartbeat(ENG, Beat(1250, 15), 250);       // 12, 13, 14 lost
    EXPECT_EQ(m.peer(ENG).missed, 3u);
    EXPECT_EQ(m.peer(ENG).received, 3u);
}

TEST(LinkMatrix, CounterWrapIsOneStep) {
    LinkMatrix m(FC);
    m.onHeartbeat(ETH, Beat(1000, 0xFFFF), 0);
    m.onHeartbeat(ETH, Beat(1050, 0x0000), 50);
    EXPECT_EQ(m.peer(ETH).missed, 0u);
    m.onHeartbeat(ETH, Beat(1200, 0x0003), 200);   // 1, 2 lost across the wrap
    EXPECT_EQ(m.peer(ETH).missed, 2u);
}

TEST(LinkMatrix, DuplicateBeatCountsNoLoss) {
    LinkMatrix m(FC);
    m.onHeartbeat(LOX, Beat(1000, 5), 100);
    m.onHeartbeat(LOX, Beat(1000, 5), 150);
    EXPECT_EQ(m.peer(LOX).missed, 0u);
    EXPECT_EQ(m.peer(LOX).received, 2u);
}

TEST(LinkMatrix, BackwardCounterStepCountsNoLoss) {
    LinkMatrix m(FC);
    m.onHeartbeat(LOX, Beat(1000, 100), 0);
    m.onHeartbeat(LOX, Beat(1050, 99), 50);        // step 0xFFFF: older frame
    EXPECT_EQ(m.peer(LOX).missed, 0u);
}

// --- Reboot ------------------------------------------------------------------

TEST(LinkMatrix, NormalBeatsAreNoReboot) {
    LinkMatrix m(FC);
    for (uint16_t i = 0; i < 100; i++) {
        m.onHeartbeat(ENG, Beat(20000 + 50u * i, i), 1000 + 50u * i);
    }
    EXPECT_EQ(m.peer(ENG).reboots, 0u);
    EXPECT_EQ(m.peer(ENG).missed, 0u);
}

TEST(LinkMatrix, UptimeGoingBackIsReboot) {
    LinkMatrix m(FC);
    m.onHeartbeat(ENG, Beat(60000, 1200), 0);
    m.onHeartbeat(ENG, Beat(12000, 0), 13000);     // reset, first beat at 12 s
    EXPECT_EQ(m.peer(ENG).reboots, 1u);
    EXPECT_EQ(m.peer(ENG).missed, 0u);             // the counter jump is no loss
    EXPECT_EQ(m.takeRebootReport(), 0x2u);
    EXPECT_EQ(m.takeRebootReport(), 0u);           // reported once
}

// A board up only 5 s reboots; its first new beat carries 12 s of uptime,
// more than before. "Uptime went down" would miss it; the clock rule does not.
TEST(LinkMatrix, ShortRunRebootIsCaughtByElapsedTime) {
    LinkMatrix m(FC);
    m.onHeartbeat(LOX, Beat(5000, 3), 10000);
    m.onHeartbeat(LOX, Beat(12000, 0), 22500);     // 12.5 s elapsed, uptime +7 s
    EXPECT_EQ(m.peer(LOX).reboots, 1u);
}

TEST(LinkMatrix, JitterWithinToleranceIsNoReboot) {
    LinkMatrix m(FC);
    m.onHeartbeat(ETH, Beat(10000, 0), 1000);
    m.onHeartbeat(ETH, Beat(10050, 1), 1290);      // drained 240 ms late
    EXPECT_EQ(m.peer(ETH).reboots, 0u);
    m.onHeartbeat(ETH, Beat(10100, 2), 1600);      // 310 - 50 = 260 ms behind
    EXPECT_EQ(m.peer(ETH).reboots, 1u);
}

TEST(LinkMatrix, LongSilenceDriftIsNoReboot) {
    LinkMatrix m(FC);
    m.onHeartbeat(ETH, Beat(10000, 0), 0);
    // 10 min of silence, the sender's clock 1 % slow: 6 s behind, under
    // 250 + 600000 / 64 = 9625 ms.
    m.onHeartbeat(ETH, Beat(10000 + 594000, 12000), 600000);
    EXPECT_EQ(m.peer(ETH).reboots, 0u);
}

TEST(LinkMatrix, UptimeWrapIsNoReboot) {
    LinkMatrix m(FC);
    m.onHeartbeat(ENG, Beat(0xFFFFFFF0u, 0), 1000);
    m.onHeartbeat(ENG, Beat(0x00000022u, 1), 1050); // uptime wrapped, +50 ms
    EXPECT_EQ(m.peer(ENG).reboots, 0u);
}

// --- Reports and boot grace --------------------------------------------------

TEST(LinkMatrix, BootGraceReportsNeverSeenOnce) {
    LinkMatrix m(FC);
    EXPECT_EQ(m.takeReport(100), 0u);              // first call: grace runs to 2100
    m.onHeartbeat(ENG, Beat(1000, 0), 1800);
    EXPECT_EQ(m.takeReport(1800), 0x2u);           // engine alive: edge
    EXPECT_EQ(m.takeReport(2099), 0u);
    EXPECT_EQ(m.takeReport(2100), 0xCu);           // eth and lox never seen: once
    EXPECT_EQ(m.takeReport(2150), 0u);
}

TEST(LinkMatrix, GraceAnchorsOnFirstCall) {
    LinkMatrix m(FC);
    EXPECT_EQ(m.takeReport(5000), 0u);             // first call 5 s after reset
    m.onHeartbeat(ENG, Beat(5000, 0), 5010);
    EXPECT_EQ(m.takeReport(5010), 0x2u);
    m.onHeartbeat(ENG, Beat(6980, 39), 6990);
    EXPECT_EQ(m.takeReport(6999), 0u);             // still inside the grace
    EXPECT_EQ(m.takeReport(7000), 0xCu);
}

TEST(LinkMatrix, AfterGraceOnlyEdges) {
    LinkMatrix m(ETH);                             // a PRC tracking its 3 peers
    EXPECT_EQ(m.takeReport(0), 0u);
    m.onHeartbeat(FC, Beat(1000, 0), 1900);
    EXPECT_EQ(m.takeReport(1900), 0x1u);           // FC alive
    EXPECT_EQ(m.takeReport(2100), 0xAu);           // grace: engine and lox never seen
    EXPECT_EQ(m.takeReport(2200), 0u);
    EXPECT_EQ(m.takeReport(2400), 0x1u);           // FC dead
    m.onHeartbeat(LOX, Beat(1000, 0), 3000);
    EXPECT_EQ(m.takeReport(3000), 0x8u);           // lox comes back
}

TEST(LinkMatrix, ReportNeverHasOwnBit) {
    LinkMatrix m(LOX);
    EXPECT_EQ(m.takeReport(0), 0u);
    EXPECT_EQ(m.takeReport(3000) & 0x8u, 0u);
}
