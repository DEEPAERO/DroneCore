#pragma once

#include "error_handling.h"
#include "mavlink_include.h"
#include "info.h"
#include "info_impl.h"
#include "telemetry.h"
#include "telemetry_impl.h"
#include "control.h"
#include "control_impl.h"

#include <cstdint>
#include <functional>
#include <atomic>
#include <vector>

namespace dronelink {

class DroneLinkImpl;

class DeviceImpl
{
public:
    explicit DeviceImpl(DroneLinkImpl *parent);
    ~DeviceImpl();
    void process_mavlink_message(const mavlink_message_t &message);

    const Info &get_info() const { return _info; }
    const Telemetry &get_telemetry() const { return _telemetry; }
    const Control &get_control() const { return _control; }

    struct CommandParams {
        float v[7];
    };

    typedef std::function<void(const mavlink_message_t &)> mavlink_message_handler_t;

    void register_mavlink_message_handler(uint8_t msg_id, mavlink_message_handler_t callback,
                                          const void *cookie);


    void unregister_all_mavlink_message_handlers(const void *cookie);

    Result send_command(uint16_t command, const DeviceImpl::CommandParams &params);
    Result send_command_with_ack(uint16_t command, const DeviceImpl::CommandParams &params);

    // Non-copyable
    DeviceImpl(const DeviceImpl &) = delete;
    const DeviceImpl &operator=(const DeviceImpl &) = delete;

private:
    void process_heartbeat(const mavlink_message_t &message);
    void process_command_ack(const mavlink_message_t &message);

    void try_to_initialize_autopilot_capabilites();

    struct HandlerTableEntry {
        uint8_t msg_id;
        mavlink_message_handler_t callback;
        const void *cookie; // This is the identification to unregister.
    };

    std::vector<HandlerTableEntry> _handler_table;

    uint8_t _system_id;
    uint8_t _component_id;

    Info _info;
    InfoImpl _info_impl;

    Telemetry _telemetry;
    TelemetryImpl _telemetry_impl;

    Control _control;
    ControlImpl _control_impl;

    DroneLinkImpl *_parent;

    enum class CommandState : unsigned {
        NONE = 0,
        WAITING,
        RECEIVED
    };

    std::atomic<MAV_RESULT> _command_result;
    std::atomic<CommandState> _command_state;
};

} // namespace dronelink