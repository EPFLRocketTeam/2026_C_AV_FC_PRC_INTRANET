
#pragma once

#include "prc_intranet/can_id.hpp"
#include "prc_intranet/payload.hpp"

/**
 * Single source of truth for the PRC<->FC CAN message dictionary.
 *
 * This is an X-macro: PRC_INTRANET_MESSAGE_LIST expands to nothing by
 * itself, it just replays one X(...) call per message. const.hpp,
 * driver.hpp, dispatch.cpp and transmit.cpp each #define X to whatever
 * shape they need (an enum, a struct field, a switch case, ...), include
 * this list, then #undef X. Add a message here once and every call site
 * picks it up automatically, so the id enum, the driver callback, the
 * dispatch switch and the send function can never drift apart.
 *
 * X(name, can_id, payload_type, dlc)
 *   name        lower_snake_case identifier, becomes the enumerator,
 *               the driver's on_<name> callback and the send_<name>()
 *               function.
 *   can_id      the 11-bit standard CAN arbitration id.
 *   payload_type a type from prc_intranet::payload.
 *   dlc         payload size actually put on the wire, in bytes. Kept
 *               explicit (rather than derived from sizeof(payload_type))
 *               because zero-payload commands use payload::empty, whose
 *               C++ sizeof is 1, not 0.
 */
#define PRC_INTRANET_MESSAGE_LIST \
    /* name                    can_id                                                                                      payload                        dlc */ \
    X(broadcast_abort,         (::prc_intranet::can::make_id(::prc_intranet::can::Priority::Emergency, ::prc_intranet::can::Node::Fc,     0x0)), ::prc_intranet::payload::safety_key,      4) \
    X(time_sync,               (::prc_intranet::can::make_id(::prc_intranet::can::Priority::System,    ::prc_intranet::can::Node::Fc,     0x0)), ::prc_intranet::payload::timestamp,       4) \
    \
    X(prc_passivate,           (::prc_intranet::can::make_id(::prc_intranet::can::Priority::Emergency, ::prc_intranet::can::Node::PrcP,   0x0)), ::prc_intranet::payload::safety_key,      4) \
    X(prc_ignite,              (::prc_intranet::can::make_id(::prc_intranet::can::Priority::HighCmd,   ::prc_intranet::can::Node::PrcP,   0x0)), ::prc_intranet::payload::empty,           0) \
    X(prc_clear_to_ignite,     (::prc_intranet::can::make_id(::prc_intranet::can::Priority::LowCmd,    ::prc_intranet::can::Node::PrcP,   0x0)), ::prc_intranet::payload::empty,           0) \
    X(prc_cmd_valves,          (::prc_intranet::can::make_id(::prc_intranet::can::Priority::LowCmd,    ::prc_intranet::can::Node::PrcP,   0x1)), ::prc_intranet::payload::cmd_valves,      2) \
    X(prc_reset,               (::prc_intranet::can::make_id(::prc_intranet::can::Priority::LowCmd,    ::prc_intranet::can::Node::PrcP,   0x2)), ::prc_intranet::payload::reset,           4) \
    X(prc_state,               (::prc_intranet::can::make_id(::prc_intranet::can::Priority::TelProp,   ::prc_intranet::can::Node::PrcP,   0x0)), ::prc_intranet::payload::prc_state,       2) \
    X(prc_p_chamber,           (::prc_intranet::can::make_id(::prc_intranet::can::Priority::TelProp,   ::prc_intranet::can::Node::PrcP,   0x1)), ::prc_intranet::payload::prc_p_chamber,   8) \
    X(prc_p_injector,          (::prc_intranet::can::make_id(::prc_intranet::can::Priority::TelProp,   ::prc_intranet::can::Node::PrcP,   0x2)), ::prc_intranet::payload::prc_p_injector,  8) \
    X(prc_t_chamber,           (::prc_intranet::can::make_id(::prc_intranet::can::Priority::TelHealth, ::prc_intranet::can::Node::PrcP,   0x0)), ::prc_intranet::payload::prc_t_chamber,   8) \
    X(prc_t_injector,          (::prc_intranet::can::make_id(::prc_intranet::can::Priority::TelHealth, ::prc_intranet::can::Node::PrcP,   0x1)), ::prc_intranet::payload::prc_t_injector,  8) \
    \
    X(dpr_eth_abort,           (::prc_intranet::can::make_id(::prc_intranet::can::Priority::Emergency, ::prc_intranet::can::Node::DprEth, 0x0)), ::prc_intranet::payload::safety_key,      4) \
    X(dpr_eth_passivate,       (::prc_intranet::can::make_id(::prc_intranet::can::Priority::Emergency, ::prc_intranet::can::Node::DprEth, 0x1)), ::prc_intranet::payload::safety_key,      4) \
    X(dpr_eth_pressurize,      (::prc_intranet::can::make_id(::prc_intranet::can::Priority::HighCmd,   ::prc_intranet::can::Node::DprEth, 0x0)), ::prc_intranet::payload::on_off,          1) \
    X(dpr_eth_cmd_valves,      (::prc_intranet::can::make_id(::prc_intranet::can::Priority::LowCmd,    ::prc_intranet::can::Node::DprEth, 0x0)), ::prc_intranet::payload::cmd_valves,      2) \
    X(dpr_eth_reset,           (::prc_intranet::can::make_id(::prc_intranet::can::Priority::LowCmd,    ::prc_intranet::can::Node::DprEth, 0x1)), ::prc_intranet::payload::reset,           4) \
    X(dpr_eth_state,           (::prc_intranet::can::make_id(::prc_intranet::can::Priority::TelProp,   ::prc_intranet::can::Node::DprEth, 0x0)), ::prc_intranet::payload::dpr_state,       2) \
    X(dpr_eth_pressures,       (::prc_intranet::can::make_id(::prc_intranet::can::Priority::TelProp,   ::prc_intranet::can::Node::DprEth, 0x1)), ::prc_intranet::payload::dpr_pressures,   8) \
    X(dpr_eth_temps_1,         (::prc_intranet::can::make_id(::prc_intranet::can::Priority::TelHealth, ::prc_intranet::can::Node::DprEth, 0x0)), ::prc_intranet::payload::dpr_temps_1,     8) \
    X(dpr_eth_temps_2,         (::prc_intranet::can::make_id(::prc_intranet::can::Priority::TelHealth, ::prc_intranet::can::Node::DprEth, 0x1)), ::prc_intranet::payload::dpr_eth_temps_2, 8) \
    \
    X(dpr_lox_abort,           (::prc_intranet::can::make_id(::prc_intranet::can::Priority::Emergency, ::prc_intranet::can::Node::DprLox, 0x0)), ::prc_intranet::payload::safety_key,      4) \
    X(dpr_lox_passivate,       (::prc_intranet::can::make_id(::prc_intranet::can::Priority::Emergency, ::prc_intranet::can::Node::DprLox, 0x1)), ::prc_intranet::payload::safety_key,      4) \
    X(dpr_lox_pressurize,      (::prc_intranet::can::make_id(::prc_intranet::can::Priority::HighCmd,   ::prc_intranet::can::Node::DprLox, 0x0)), ::prc_intranet::payload::on_off,          1) \
    X(dpr_lox_cmd_valves,      (::prc_intranet::can::make_id(::prc_intranet::can::Priority::LowCmd,    ::prc_intranet::can::Node::DprLox, 0x0)), ::prc_intranet::payload::cmd_valves,      2) \
    X(dpr_lox_reset,           (::prc_intranet::can::make_id(::prc_intranet::can::Priority::LowCmd,    ::prc_intranet::can::Node::DprLox, 0x1)), ::prc_intranet::payload::reset,           4) \
    X(dpr_lox_state,           (::prc_intranet::can::make_id(::prc_intranet::can::Priority::TelProp,   ::prc_intranet::can::Node::DprLox, 0x0)), ::prc_intranet::payload::dpr_state,       2) \
    X(dpr_lox_pressures,       (::prc_intranet::can::make_id(::prc_intranet::can::Priority::TelProp,   ::prc_intranet::can::Node::DprLox, 0x1)), ::prc_intranet::payload::dpr_pressures,   8) \
    X(dpr_lox_temps_1,         (::prc_intranet::can::make_id(::prc_intranet::can::Priority::TelHealth, ::prc_intranet::can::Node::DprLox, 0x0)), ::prc_intranet::payload::dpr_temps_1,     8) \
    X(dpr_lox_temps_2,         (::prc_intranet::can::make_id(::prc_intranet::can::Priority::TelHealth, ::prc_intranet::can::Node::DprLox, 0x1)), ::prc_intranet::payload::dpr_lox_temps_2, 8) \
    \
    /* Forwarded serial log lines, chunked 8 bytes/frame, always sent full
     * (short chunks zero-padded) -- see log_aggregator. */ \
    X(log_chunk_prc_engine,    (::prc_intranet::can::make_id(::prc_intranet::can::Priority::Log,        ::prc_intranet::can::Node::PrcP,   0x0)), ::prc_intranet::payload::log_chunk,      8) \
    X(log_chunk_dpr_eth,       (::prc_intranet::can::make_id(::prc_intranet::can::Priority::Log,        ::prc_intranet::can::Node::DprEth, 0x0)), ::prc_intranet::payload::log_chunk,      8) \
    X(log_chunk_dpr_lox,       (::prc_intranet::can::make_id(::prc_intranet::can::Priority::Log,        ::prc_intranet::can::Node::DprLox, 0x0)), ::prc_intranet::payload::log_chunk,      8)
