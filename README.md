
# FC - PRC Intranet

This repository is an implementation of the FC - PRC intranet, decoding and encoding
CAN frames exchanged between the Flight Computer (FC) and the Propulsion Computer
nodes (engine PRC, DPR Ethanol, DPR LOX) on the main CAN bus.

Unlike a byte-stream channel (see `fdr_intranet`), CAN already gives you frame
boundaries, an arbitration id and a hardware CRC per message, so there is no
byte-level framing/checksum layer here. This library's job is purely to decode a
received `(can_id, data, dlc)` frame into a typed payload and dispatch it to the
right callback, and to encode a typed payload back into a frame to send.

The full message dictionary (id, payload type, wire size) lives in one place,
`include/prc_intranet/message_list.hpp`, as an X-macro. Every other piece (the
`MessageId` enum, the `prc_driver` callback table, the receive dispatcher, the
send functions) is generated from that single list, so adding or changing a
message can't leave those pieces out of sync with each other.

# Getting Started

```c++
// You should create the context.
prc_intranet::context my_channel;

void cb_send(void* driver_ptr, uint16_t can_id, const uint8_t* buffer, uint32_t dlc) noexcept {
    // TODO: send through the FDCAN peripheral, e.g.
    //   FDCAN_TxHeaderTypeDef header = { .Identifier = can_id, .DataLength = dlc_to_hal(dlc), ... };
    //   HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, &header, buffer);
}

// Only implement the callbacks this board actually cares about, leave the
// rest null-initialized. E.g. a DPR_ETH board only needs its own
// on_dpr_eth_* callbacks, not on_prc_* or on_dpr_lox_*.
void cb_on_dpr_eth_abort(void*, prc_intranet::payload::safety_key key) noexcept {
    if (key.safety_key != prc_intranet::constants::SAFETY_KEY_DPR_ETH_ABORT) return;
    // TODO: abort
}
void cb_on_dpr_eth_pressurize(void*, prc_intranet::payload::on_off cmd) noexcept {
    // TODO: pressurize on/off depending on cmd.state
}

int main (void) {
    prc_intranet::prc_driver driver{};
    driver.driver_ptr = NULL;
    driver.send = cb_send;
    driver.on_dpr_eth_abort = cb_on_dpr_eth_abort;
    driver.on_dpr_eth_pressurize = cb_on_dpr_eth_pressurize;
    // ... set only the callbacks this board needs, leave the rest null.

    my_channel = prc_intranet::create_context(driver);

    // Call this from your FDCAN RX FIFO ISR/poll loop, once per received frame.
    // dispatch_frame(&my_channel, rx_header.Identifier, rx_data, dlc);

    // Send frames (PRC => FC)
    prc_intranet::payload::dpr_state state{};
    prc_intranet::send_dpr_eth_state(&my_channel, state);

    // Send frames (FC => PRC)
    prc_intranet::payload::on_off pressurize{ prc_intranet::constants::CMD_ON };
    prc_intranet::send_dpr_eth_pressurize(&my_channel, pressurize);
}
```

# Critical Implementation Rules

All of your driver functions should be `noexcept` functions. If some of your
functions may throw exceptions, then you should wrap their content inside try/catch
blocks. This design choice is because some of your driver functions will likely be
executed inside some interrupts and as such shouldn't throw exceptions. Also in our
codebase, we ask you to refrain from using such exceptions and instead try to find
ways to report the error whilst continuing execution as best as possible.

Business-logic validation (safety keys, reset magics, FSM legality, ...) is
deliberately **not** this library's responsibility; it only decodes frames into
typed payloads. Check `payload::safety_key`/`payload::reset` fields against the
constants in `const.hpp` inside your own callbacks, same as `fdr_intranet` leaves
packet content validation to the application layer.

# Adding a message

Add one row to `PRC_INTRANET_MESSAGE_LIST` in `include/prc_intranet/message_list.hpp`
(name, CAN id, payload type, wire size). The `MessageId` enum, the `prc_driver`
callback, the `dispatch_frame` decode case and the `send_<name>` function are all
generated from that one list; no other file needs to change.
