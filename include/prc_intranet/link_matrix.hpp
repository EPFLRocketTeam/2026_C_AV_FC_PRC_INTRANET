#pragma once

// 4x4 CAN connectivity matrix, built from the heartbeats.
//
// Every board (FC, Engine, Eth, LOX) runs one LinkMatrix. It is fed with the
// heartbeats of the 3 other boards and answers:
//   - is board i alive?         (a beat less than TIMEOUT_MS ago)
//   - my own row                (bit i = "I hear board i", own bit set),
//                               sent in my heartbeat's alive_mask
//   - the full matrix           (my row + the rows the others sent me,
//                               a silent board's row at 0), used by the FC
//   - lost beats and reboots    per board
//
// Pure and HAL free: the caller passes the receive time (its own ms clock),
// so it is host tested (tests/test_link_matrix.cpp).
//
// Board indices and bit positions: constants::LINK_BIT_* (0 FC, 1 Engine,
// 2 Eth, 3 LOX). Matrix layout: row i at bits 4i..4i+3.

#include <cstdint>

#include "prc_intranet/const.hpp"
#include "prc_intranet/payload.hpp"

namespace prc_intranet::link {

    constexpr uint8_t  BOARD_COUNT   = constants::LINK_BOARD_COUNT;
    constexpr uint32_t TIMEOUT_MS    = 500;   // 10 beats at 50 ms
    constexpr uint32_t BOOT_GRACE_MS = 2000;  // counted from the first takeReport()

    // Reboot rule: between two beats, the sender's uptime must advance as
    // much as our own clock did. If it advanced less, by more than the
    // tolerance, the sender restarted from 0 in between.
    //
    // Tolerance = REBOOT_TOLERANCE_MS + elapsed / REBOOT_DRIFT_DIVISOR:
    //   - 250 ms fixed covers timing jitter: our receive time is when the
    //     main loop drains the frame, not when it was sent, and a frame can
    //     wait in the TX FIFO. These are a few ms to a few tens of ms.
    //   - elapsed / 64 (about 1.6 %) covers the drift between the two
    //     clocks during a long silence. Two crystals differ by about
    //     100 ppm, the internal HSI by about 1 %, so 1.6 % is enough
    //     either way.
    // A real reboot is still caught: the gap is at least the uptime the
    // board had at its last beat before it reset (hundreds of ms on the
    // FC, about 12 s on a PRC), well above the tolerance.
    constexpr uint32_t REBOOT_TOLERANCE_MS  = 250;
    constexpr uint32_t REBOOT_DRIFT_DIVISOR = 64;

    // What we know about one other board.
    struct Peer {
        bool     seen           = false; // at least one beat received
        uint32_t last_rx_ms     = 0;     // our clock, at the last beat
        uint32_t last_uptime_ms = 0;     // its uptime, at the last beat
        uint16_t last_counter   = 0;
        uint8_t  row            = 0;     // its alive_mask, at the last beat
        uint32_t received       = 0;     // beats received
        uint32_t missed         = 0;     // beats lost, from counter jumps
        uint32_t reboots        = 0;     // reboots detected
    };

    class LinkMatrix {
    public:
        // self: this board's index (constants::LINK_BIT_*).
        explicit LinkMatrix(uint8_t self) : self_(self) {}

        // Call for each heartbeat received from board `board`.
        void onHeartbeat(uint8_t board, const payload::heartbeat& hb, uint32_t now_ms) {
            if (board >= BOARD_COUNT || board == self_) return;
            Peer& p = peers_[board];

            if (p.seen) {
                if (isReboot(p, hb.uptime_ms, now_ms)) {
                    // Its counter restarted from 0: a jump is not a loss.
                    p.reboots++;
                    pending_reboots_ |= bit(board);
                } else {
                    // Modulo 2^16: 0xFFFF -> 0x0000 is a step of 1.
                    // A step of 0 is a duplicate. A step of 2^15 or more
                    // is a frame older than the last one: no loss counted.
                    const uint16_t step = static_cast<uint16_t>(hb.counter - p.last_counter);
                    if (step > 1 && step < 0x8000u) p.missed += step - 1u;
                }
            }

            p.seen           = true;
            p.last_rx_ms     = now_ms;
            p.last_uptime_ms = hb.uptime_ms;
            p.last_counter   = hb.counter;
            p.row            = hb.alive_mask & 0x0Fu;
            p.received++;
        }

        // True for this board itself, and for a board heard from less than
        // TIMEOUT_MS ago. Unsigned subtraction stays right across the
        // 2^32 ms wrap of the clock.
        bool alive(uint8_t board, uint32_t now_ms) const {
            if (board >= BOARD_COUNT) return false;
            if (board == self_) return true;
            const Peer& p = peers_[board];
            return p.seen && (now_ms - p.last_rx_ms) < TIMEOUT_MS;
        }

        // This board's row, to put in its heartbeat's alive_mask.
        uint8_t ownRow(uint32_t now_ms) const {
            uint8_t row = 0;
            for (uint8_t i = 0; i < BOARD_COUNT; i++) {
                if (alive(i, now_ms)) row |= bit(i);
            }
            return row;
        }

        // Row of board `board`: our own row for ourselves, the last row it
        // sent while it is alive, 0 once it went silent.
        uint8_t row(uint8_t board, uint32_t now_ms) const {
            if (board >= BOARD_COUNT) return 0;
            if (board == self_) return ownRow(now_ms);
            return alive(board, now_ms) ? peers_[board].row : 0;
        }

        // The 16-bit matrix: row i at bits 4i..4i+3.
        uint16_t matrix(uint32_t now_ms) const {
            uint16_t m = 0;
            for (uint8_t i = 0; i < BOARD_COUNT; i++) {
                m |= static_cast<uint16_t>(row(i, now_ms) << (4 * i));
            }
            return m;
        }

        // Boards whose state should be printed now (same bits as a row,
        // own bit never set): every alive/dead edge, plus one "dead" for
        // each board never heard from BOOT_GRACE_MS after the first call.
        // The grace counts from the first call, not from reset, so a long
        // setup with frames not yet drained reports no false "dead".
        uint8_t takeReport(uint32_t now_ms) {
            if (!started_) {
                started_  = true;
                first_ms_ = now_ms;
            }
            const uint8_t others = static_cast<uint8_t>(ownRow(now_ms) & ~bit(self_));
            uint8_t report = others ^ reported_;
            reported_ = others;

            if (!grace_done_ && (now_ms - first_ms_) >= BOOT_GRACE_MS) {
                grace_done_ = true;
                for (uint8_t i = 0; i < BOARD_COUNT; i++) {
                    if (i != self_ && !peers_[i].seen) report |= bit(i);
                }
            }
            return report;
        }

        // Boards whose reboot was detected since the last call.
        uint8_t takeRebootReport() {
            const uint8_t r = pending_reboots_;
            pending_reboots_ = 0;
            return r;
        }

        const Peer& peer(uint8_t board) const { return peers_[board < BOARD_COUNT ? board : 0]; }
        uint8_t self() const { return self_; }

    private:
        static uint8_t bit(uint8_t board) { return static_cast<uint8_t>(1u << board); }

        // True when the uptime advanced less than our clock, by more than
        // the tolerance (see REBOOT_TOLERANCE_MS). Computed in 64 bits so
        // no subtraction can overflow; the int32 cast keeps the uptime
        // difference right across its 2^32 ms wrap.
        static bool isReboot(const Peer& p, uint32_t uptime_ms, uint32_t now_ms) {
            const int64_t elapsed   = static_cast<int64_t>(now_ms - p.last_rx_ms);
            const int64_t advanced  = static_cast<int32_t>(uptime_ms - p.last_uptime_ms);
            const int64_t tolerance = REBOOT_TOLERANCE_MS + elapsed / REBOOT_DRIFT_DIVISOR;
            return (elapsed - advanced) > tolerance;
        }

        uint8_t  self_;
        Peer     peers_[BOARD_COUNT];
        uint8_t  reported_        = 0;
        uint8_t  pending_reboots_ = 0;
        bool     started_         = false;
        bool     grace_done_      = false;
        uint32_t first_ms_        = 0;
    };

};
