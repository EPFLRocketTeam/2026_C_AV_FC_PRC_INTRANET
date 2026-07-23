
#pragma once

#include <cstdint>

#include "prc_intranet/message_list.hpp"

namespace prc_intranet::constants {

    // Every arbitration id this library knows about, generated from
    // message_list.hpp so it can never drift from the dispatch/transmit
    // tables built from the same list.
    enum class MessageId : uint16_t {
#define X(name, can_id, payload_type, dlc) name = (can_id),
        PRC_INTRANET_MESSAGE_LIST
#undef X
    };

    // Classic CAN frame payload limit (this bus doesn't use CAN-FD).
    constexpr uint8_t MAX_PAYLOAD_SIZE = 8;

    // Wire-format size for a given message id, as declared in
    // message_list.hpp. Returns 0 for an id this library doesn't know
    // (dispatch_frame() also independently rejects unknown ids, this is
    // just so the function is total).
    constexpr uint32_t dlc_for (MessageId id) noexcept {
        switch (id) {
#define X(name, can_id, payload_type, dlc) case MessageId::name: return dlc;
            PRC_INTRANET_MESSAGE_LIST
#undef X
        }
        return 0;
    }

    // Common command values
    constexpr uint8_t  CMD_ON      = 0x64;
    constexpr uint8_t  CMD_OFF     = 0x0D;
    constexpr uint32_t RESET_MAGIC = 0xDEADBEEF;

    // Valve ids (payload::cmd_valves::valve_id)
    constexpr uint8_t VALVE_SOL1 = 0xA;
    constexpr uint8_t VALVE_SOL2 = 0xB;
    constexpr uint8_t VALVE_SOL3 = 0xC;
    constexpr uint8_t VALVE_SOL4 = 0xD;

    // DPR (DPR_ETH/DPR_LOX) valve ids. PLACEHOLDERS, confirm before use.
    constexpr uint8_t VALVE_MAIN      = 0xE0;
    constexpr uint8_t VALVE_SAFETY    = 0xE1;
    constexpr uint8_t VALVE_BALLVALVE = 0xE2;

    constexpr uint8_t VALVE_STATE_OPEN   = 0x1;
    constexpr uint8_t VALVE_STATE_CLOSED = 0x0;

    // Safety keys (EMERGENCY payload validation constants).
    // All placeholders except BROADCAST_ABORT and PRC_PASSIVATE, which
    // were given in CAN.md. Confirm/replace before flight; each must be
    // unique per command and agreed with the whole team. Checking these
    // against payload::safety_key is application logic, not this
    // library's job (this library only handles framing/decoding).
    constexpr uint32_t SAFETY_KEY_BROADCAST_ABORT   = 0xABBABBFF;
    constexpr uint32_t SAFETY_KEY_PRC_PASSIVATE     = 0x42424242;
    constexpr uint32_t SAFETY_KEY_DPR_ETH_ABORT     = 0x2A2A2A2A;
    constexpr uint32_t SAFETY_KEY_DPR_ETH_PASSIVATE = 0x2B2B2B2B;
    constexpr uint32_t SAFETY_KEY_DPR_LOX_ABORT     = 0x3A3A3A3A;
    constexpr uint32_t SAFETY_KEY_DPR_LOX_PASSIVATE = 0x3B3B3B3B;

};
