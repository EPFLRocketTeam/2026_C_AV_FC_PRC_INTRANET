
#pragma once

#include <cstdint>

namespace prc_intranet::payload {

#pragma pack(push, 1)

    // Zero-byte payload, used by pure commands (ignite, clear-to-ignite, ...)
    // that carry no data beyond their arbitration id.
    struct empty {};

    struct safety_key {
        uint32_t safety_key;
    };

    struct timestamp {
        uint32_t timestamp_ms;
    };

    struct on_off {
        uint8_t state; // CMD_ON / CMD_OFF, see const.hpp
    };

    // Single valve-ID + state. Always one valve at a time.
    struct cmd_valves {
        uint8_t valve_id; // VALVE_SOL1..4 (PRC) or VALVE_SAFETY/VENT/BALLVALVE (DPR)
        uint8_t state;    // VALVE_STATE_OPEN / VALVE_STATE_CLOSED
    };

    struct reset {
        uint32_t magic; // RESET_MAGIC
    };

    struct prc_state {
        uint8_t fsm_state;
        uint8_t valve_mask;
    };

    struct prc_p_chamber {
        float p_oin;
        float p_ein;
    };

    struct prc_p_injector {
        float p_ccc;
        float p_cig;
    };

    struct prc_t_chamber {
        float t_oin;
        float t_ein;
    };

    struct prc_t_injector {
        float t_ccc;
        uint8_t reserved[4];
    };

    struct dpr_state {
        uint8_t fsm_state;
        uint8_t valve_mask; // VALVE_MASK_BIT_SAFETY / VALVE_MASK_BIT_VENT, see const.hpp
    };

    struct dpr_pressures {
        float p_xta;
        float p_nco;
    };

    struct dpr_temps_1 {
        float t_xta;
        float t_nco;
    };

    // DPR_ETH_TEMPS_2: T_COPV_EXT, T_FLS_90. Distinct from LOX's, see
    // dpr_lox_temps_2 below.
    struct dpr_eth_temps_2 {
        float t_copv_ext;
        float t_fls_90;
    };

    // DPR_LOX_TEMPS_2: T_FLS_80, T_FLS_50. No COPV-external field, so it
    // can't share a struct with DPR_ETH's TEMPS_2.
    struct dpr_lox_temps_2 {
        float t_fls_80;
        float t_fls_50;
    };

    struct cam_status {
        uint8_t is_recording;
        uint8_t power_state;
    };

    // Raw forwarded log bytes. Always sent as a full 8-byte frame; when a
    // chunk has fewer than 8 real bytes, the sender zero-pads the rest,
    // and the receiver stops appending at the first \0 it sees in a given
    // frame. See log_aggregator for the chunking/reassembly logic built
    // on top of this.
    struct log_chunk {
        uint8_t bytes[8];
    };

#pragma pack(pop)

    static_assert(sizeof(safety_key)      == 4);
    static_assert(sizeof(timestamp)       == 4);
    static_assert(sizeof(on_off)          == 1);
    static_assert(sizeof(cmd_valves)      == 2);
    static_assert(sizeof(reset)           == 4);
    static_assert(sizeof(prc_state)       == 2);
    static_assert(sizeof(prc_p_chamber)   == 8);
    static_assert(sizeof(prc_p_injector)  == 8);
    static_assert(sizeof(prc_t_chamber)   == 8);
    static_assert(sizeof(prc_t_injector)  == 8);
    static_assert(sizeof(dpr_state)       == 2);
    static_assert(sizeof(dpr_pressures)   == 8);
    static_assert(sizeof(dpr_temps_1)     == 8);
    static_assert(sizeof(dpr_eth_temps_2) == 8);
    static_assert(sizeof(dpr_lox_temps_2) == 8);
    static_assert(sizeof(cam_status)      == 2);
    static_assert(sizeof(log_chunk)       == 8);

};
