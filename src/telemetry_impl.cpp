#include "telemetry_impl.h"
#include "device_impl.h"
#include "math_conversions.h"
#include "global_include.h"
#include <cmath>
#include <functional>

namespace dronelink {

TelemetryImpl::TelemetryImpl(DeviceImpl *parent) :
    _parent(parent),
    _position_mutex(),
    _position({NAN, NAN, NAN, NAN}),
    _home_position_mutex(),
    _home_position({NAN, NAN, NAN, NAN}),
    _in_air(false),
    _attitude_quaternion_mutex(),
    _attitude_quaternion({NAN, NAN, NAN, NAN}),
    _ground_speed_ned_mutex(),
    _ground_speed_ned({NAN, NAN, NAN}),
    _gps_info_mutex(),
    _gps_info({0, 0}),
    _battery_mutex(),
    _battery({NAN, NAN})
{
}

TelemetryImpl::~TelemetryImpl()
{
}

void TelemetryImpl::init()
{
    using namespace std::placeholders; // for `_1`

    _parent->register_mavlink_message_handler(MAVLINK_MSG_ID_GLOBAL_POSITION_INT,
        std::bind(&TelemetryImpl::process_global_position_int, this, _1), (void *)this);

    _parent->register_mavlink_message_handler(MAVLINK_MSG_ID_HOME_POSITION,
        std::bind(&TelemetryImpl::process_home_position, this, _1), (void *)this);

    _parent->register_mavlink_message_handler(MAVLINK_MSG_ID_ATTITUDE_QUATERNION,
        std::bind(&TelemetryImpl::process_attitude_quaternion, this, _1), (void *)this);

    _parent->register_mavlink_message_handler(MAVLINK_MSG_ID_GPS_RAW_INT,
        std::bind(&TelemetryImpl::process_gps_raw_int, this, _1), (void *)this);

    _parent->register_mavlink_message_handler(MAVLINK_MSG_ID_EXTENDED_SYS_STATE,
        std::bind(&TelemetryImpl::process_extended_sys_state, this, _1), (void *)this);

    _parent->register_mavlink_message_handler(MAVLINK_MSG_ID_SYS_STATUS,
        std::bind(&TelemetryImpl::process_sys_status, this, _1), (void *)this);
}

void TelemetryImpl::deinit()
{
    _parent->unregister_all_mavlink_message_handlers((void *)this);
}

void TelemetryImpl::process_global_position_int(const mavlink_message_t &message)
{
    mavlink_global_position_int_t global_position_int;
    mavlink_msg_global_position_int_decode(&message, &global_position_int);
    set_position(Telemetry::Position({global_position_int.lat * 1e-7,
                                      global_position_int.lon * 1e-7,
                                      global_position_int.alt * 1e-3f,
                                      global_position_int.relative_alt * 1e-3f}));
    set_ground_speed_ned({global_position_int.vx * 1e-2f,
                          global_position_int.vy * 1e-2f,
                          global_position_int.vz * 1e-2f});
}

void TelemetryImpl::process_home_position(const mavlink_message_t &message)
{
    mavlink_home_position_t home_position;
    mavlink_msg_home_position_decode(&message, &home_position);
    set_home_position(Telemetry::Position({home_position.latitude * 1e-7,
                                           home_position.longitude * 1e-7,
                                           home_position.altitude * 1e-3f,
     // the relative altitude of home is 0 by definition.
                                           0.0f}));
}

void TelemetryImpl::process_attitude_quaternion(const mavlink_message_t &message)
{
    mavlink_attitude_quaternion_t attitude_quaternion;
    mavlink_msg_attitude_quaternion_decode(&message, &attitude_quaternion);

    Telemetry::Quaternion quaternion(
        {attitude_quaternion.q1,
         attitude_quaternion.q2,
         attitude_quaternion.q3,
         attitude_quaternion.q4}
    );

    set_attitude_quaternion(quaternion);
}

void TelemetryImpl::process_gps_raw_int(const mavlink_message_t &message)
{
    mavlink_gps_raw_int_t gps_raw_int;
    mavlink_msg_gps_raw_int_decode(&message, &gps_raw_int);
    set_gps_info({gps_raw_int.satellites_visible,
                  gps_raw_int.fix_type});
}

void TelemetryImpl::process_extended_sys_state(const mavlink_message_t &message)
{
    mavlink_extended_sys_state_t extended_sys_state;
    mavlink_msg_extended_sys_state_decode(&message, &extended_sys_state);
    if (extended_sys_state.landed_state == MAV_LANDED_STATE_IN_AIR) {
        set_in_air(true);
    } else if (extended_sys_state.landed_state == MAV_LANDED_STATE_IN_AIR) {
        set_in_air(false);
    }
    // If landed_state is undefined, we use what we have received last.
}

void TelemetryImpl::process_sys_status(const mavlink_message_t &message)
{
    mavlink_sys_status_t sys_status;
    mavlink_msg_sys_status_decode(&message, &sys_status);
    set_battery(Telemetry::Battery({sys_status.voltage_battery * 1e-3f,
                                    sys_status.battery_remaining * 1e-2f}));
}


Telemetry::Position TelemetryImpl::get_position() const
{
    std::lock_guard<std::mutex> lock(_position_mutex);
    return _position;
}

void TelemetryImpl::set_position(Telemetry::Position position)
{
    std::lock_guard<std::mutex> lock(_position_mutex);
    _position = position;
}
Telemetry::Position TelemetryImpl::get_home_position() const
{
    std::lock_guard<std::mutex> lock(_home_position_mutex);
    return _home_position;
}

void TelemetryImpl::set_home_position(Telemetry::Position home_position)
{
    std::lock_guard<std::mutex> lock(_home_position_mutex);
    _home_position = home_position;
}

bool TelemetryImpl::in_air() const
{
    return _in_air;
}

void TelemetryImpl::set_in_air(bool in_air)
{
    _in_air = in_air;
}

Telemetry::Quaternion TelemetryImpl::get_attitude_quaternion() const
{
    std::lock_guard<std::mutex> lock(_attitude_quaternion_mutex);
    return _attitude_quaternion;
}

Telemetry::EulerAngle TelemetryImpl::get_attitude_euler_angle() const
{
    std::lock_guard<std::mutex> lock(_attitude_quaternion_mutex);
    return to_euler_angle_from_quaternion(_attitude_quaternion);
}

void TelemetryImpl::set_attitude_quaternion(Telemetry::Quaternion quaternion)
{
    std::lock_guard<std::mutex> lock(_attitude_quaternion_mutex);
    _attitude_quaternion = quaternion;
}

Telemetry::GroundSpeedNED TelemetryImpl::get_ground_speed_ned() const
{
    std::lock_guard<std::mutex> lock(_ground_speed_ned_mutex);
    return _ground_speed_ned;
}

void TelemetryImpl::set_ground_speed_ned(Telemetry::GroundSpeedNED ground_speed_ned)
{
    std::lock_guard<std::mutex> lock(_ground_speed_ned_mutex);
    _ground_speed_ned = ground_speed_ned;
}

Telemetry::GPSInfo TelemetryImpl::get_gps_info() const
{
    std::lock_guard<std::mutex> lock(_gps_info_mutex);
    return _gps_info;
}

void TelemetryImpl::set_gps_info(Telemetry::GPSInfo gps_info)
{
    std::lock_guard<std::mutex> lock(_gps_info_mutex);
    _gps_info = gps_info;
}

Telemetry::Battery TelemetryImpl::get_battery() const
{
    std::lock_guard<std::mutex> lock(_battery_mutex);
    return _battery;
}

void TelemetryImpl::set_battery(Telemetry::Battery battery)
{
    std::lock_guard<std::mutex> lock(_battery_mutex);
    _battery = battery;
}


} // namespace dronelink