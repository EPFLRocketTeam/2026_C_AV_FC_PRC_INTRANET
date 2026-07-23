
#include "prc_intranet/dispatch.hpp"
#include "prc_intranet/const.hpp"

#include <cstring>

namespace prc_intranet {

namespace {

    // Reads a payload struct out of the raw RX bytes via memcpy rather
    // than a pointer-cast, since the caller's frame buffer isn't
    // guaranteed aligned for the float-bearing payloads (pressures/temps)
    // and an unaligned load can fault on Cortex-M7. Returns a zeroed
    // struct if dlc is short, though dispatch_frame() already guards
    // that case before calling this.
    template <typename Payload>
    Payload read_payload (const uint8_t* data, uint32_t dlc) noexcept {
        Payload payload{};
        if (dlc >= sizeof(Payload)) {
            memcpy(&payload, data, sizeof(Payload));
        }
        return payload;
    }

};

void dispatch_frame (struct context* channel_ctx, uint16_t can_id, const uint8_t* data, uint32_t dlc) {
    switch (static_cast<constants::MessageId>(can_id)) {
#define X(name, id, payload_type, expected_dlc) \
        case constants::MessageId::name: \
            if (dlc >= (expected_dlc) && channel_ctx->driver.on_##name) { \
                channel_ctx->driver.on_##name( \
                    channel_ctx->driver.driver_ptr, \
                    read_payload<payload_type>(data, dlc) \
                ); \
            } \
            return ;
        PRC_INTRANET_MESSAGE_LIST
#undef X
    }

    // Unknown id: not a message this library knows about, silently
    // ignored, same as how fdr_intranet drops bytes it can't frame.
}

}
