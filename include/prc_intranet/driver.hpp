
#pragma once

#include <cstdint>

#include "prc_intranet/message_list.hpp"

namespace prc_intranet {

    struct prc_driver {
        void* driver_ptr;

        /**
         * Send one CAN frame through the channel. Must be implemented by
         * the developer (wraps HAL_FDCAN_AddMessageToTxFifoQ or similar).
         *
         * @param driver_ptr the relevant driver information.
         * @param can_id the 11-bit standard arbitration id to send with.
         * @param buffer the payload bytes (length == dlc).
         * @param dlc the number of valid bytes in buffer (<= MAX_PAYLOAD_SIZE).
         */
        void (*send) (void* driver_ptr, uint16_t can_id, const uint8_t* buffer, uint32_t dlc) noexcept;

        // One callback per message in message_list.hpp, invoked by
        // dispatch_frame() when a matching, correctly-sized frame is
        // decoded. Leave a callback null to ignore that message; e.g. a
        // DPR_ETH board only needs its own on_dpr_eth_* callbacks set.
#define X(name, can_id, payload_type, dlc) void (*on_##name) (void* driver_ptr, payload_type payload) noexcept;
        PRC_INTRANET_MESSAGE_LIST
#undef X
    };

};
