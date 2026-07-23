
#include "prc_intranet/transmit.hpp"
#include "prc_intranet/const.hpp"

#include <cstring>

namespace prc_intranet {

namespace {

    template <typename Payload>
    void send_frame (context* channel_ctx, constants::MessageId id, uint32_t dlc, const Payload& payload) noexcept {
        uint8_t buffer[constants::MAX_PAYLOAD_SIZE] = {};
        memcpy(buffer, &payload, dlc);
        channel_ctx->driver.send(channel_ctx->driver.driver_ptr, static_cast<uint16_t>(id), buffer, dlc);
    }

};

#define X(name, id, payload_type, dlc) \
    void send_##name (struct context* channel_ctx, payload_type payload) { \
        send_frame(channel_ctx, constants::MessageId::name, (dlc), payload); \
    }
    PRC_INTRANET_MESSAGE_LIST
#undef X

}
