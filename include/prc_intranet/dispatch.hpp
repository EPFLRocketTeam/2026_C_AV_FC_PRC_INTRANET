
#pragma once

#include "prc_intranet/driver.hpp"

namespace prc_intranet {

    /**
     * This structure represents the current status of the PRC-FC Intranet driver.
     */
    struct context {
        prc_driver driver;
    };

    /**
     * Create an empty context.
     *
     * @param driver the driver information
     */
    struct context create_context (prc_driver driver);

    /**
     * Decode one received CAN frame and invoke the matching driver
     * callback. This must be called by the developer whenever a frame is
     * received (typically from the FDCAN RX FIFO ISR/poll loop).
     *
     * Unlike fdr_intranet's recv_bytes, there is no byte-stream framing to
     * do here: the CAN peripheral already delivers whole, CRC-checked
     * frames one at a time, so this is a pure id to payload decode:
     *
     *   - can_id not in message_list.hpp: ignored. Expected, since the
     *     bus carries frames for other nodes too.
     *   - dlc smaller than the message's declared size: ignored. The
     *     hardware already guarantees the frame's integrity, so a short
     *     dlc means the sender built a malformed frame, not something
     *     this layer can recover from by guessing.
     *   - driver callback left null: ignored, same as an unknown id.
     *
     * @param channel_ctx the context of the channel.
     * @param can_id the 11-bit standard arbitration id the frame arrived on.
     * @param data the frame's payload bytes.
     * @param dlc the number of valid bytes in data.
     */
    void dispatch_frame (struct context* channel_ctx, uint16_t can_id, const uint8_t* data, uint32_t dlc);

};
