
#pragma once

#include "prc_intranet/dispatch.hpp"
#include "prc_intranet/message_list.hpp"

namespace prc_intranet {

    // One send_<name>(context*, payload) per message in message_list.hpp.
    // Builds the frame (copies `payload` into a MAX_PAYLOAD_SIZE buffer,
    // sized to the message's declared dlc) and hands it to driver.send().
    // Zero-payload commands still take a payload::empty argument for
    // uniformity; pass payload::empty{}.
#define X(name, can_id, payload_type, dlc) void send_##name (struct context* channel_ctx, payload_type payload);
    PRC_INTRANET_MESSAGE_LIST
#undef X

};
