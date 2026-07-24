
#pragma once

#include <cstdint>

/**
 * This file contains the raw 11-bit standard CAN arbitration ID layout
 * shared by every message on the PRC<->FC bus:
 *
 *   Bits [10:8] = Priority / Type
 *   Bits [7:4]  = Target Node ID
 *   Bits [3:0]  = Message Index
 *
 * The actual message dictionary (which id maps to which payload) lives in
 * message_list.hpp, built from these building blocks.
 */
namespace prc_intranet::can {

    enum class Priority : uint8_t {
        Emergency = 0x0, // Abort, Passivate. Preempts all other traffic.
        HighCmd   = 0x1, // Time-critical commands (Ignite, Pressurize).
        LowCmd    = 0x2, // State changes, manual valve controls, resets.
        TelProp   = 0x3, // High-rate telemetry (Pressures, FSM State).
        TelHealth = 0x4, // Low-rate telemetry (Temps, INA228/TMP1075/LMT85).
        System    = 0x5, // Timestamp sync, Heartbeats.
        Log       = 0x7, // Forwarded serial log lines. Max value in this
                          // 3-bit field on purpose: on real CAN, a smaller
                          // arbitration id wins arbitration, so this always
                          // loses to every other message class and can never
                          // delay a command/telemetry frame.
    };

    enum class Node : uint8_t {
        Fc     = 0x0, // Flight Computer (FC) / Broadcast
        PrcP   = 0x1, // Propulsion Computer ENGINE
        DprEth = 0x2, // PRC/DPR Ethanol
        DprLox = 0x3, // PRC/DPR LOX
    };

    constexpr uint16_t make_id (Priority prio, Node node, uint8_t idx) noexcept {
        return static_cast<uint16_t>(
            ((static_cast<uint8_t>(prio) & 0x7) << 8) |
            ((static_cast<uint8_t>(node) & 0xF) << 4) |
            (idx & 0xF)
        );
    }

};
