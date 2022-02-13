#pragma once

#include "pch.h"
#include "Endpoint.h"

BETTER_ENUM( AxisError, int32_t, 
	AXIS_ERROR_NONE = 0x0,
	AXIS_ERROR_INVALID_STATE = 0x1,
	AXIS_ERROR_WATCHDOG_TIMER_EXPIRED = 0x800,
	AXIS_ERROR_MIN_ENDSTOP_PRESSED = 0x1000,
	AXIS_ERROR_MAX_ENDSTOP_PRESSED = 0x2000,
	AXIS_ERROR_ESTOP_REQUESTED = 0x4000,
	AXIS_ERROR_HOMING_WITHOUT_ENDSTOP = 0x20000,
	AXIS_ERROR_OVER_TEMP = 0x40000,
	AXIS_ERROR_UNKNOWN_POSITION = 0x80000
)

static std::map<std::string, std::string> AxisErrorDesc = {
	{ "", "" }
};




BETTER_ENUM(MotorError, int32_t,
	MOTOR_ERROR_NONE = 0x0,
	MOTOR_ERROR_PHASE_RESISTANCE_OUT_OF_RANGE = 0x1,
	MOTOR_ERROR_PHASE_INDUCTANCE_OUT_OF_RANGE = 0x2,
	MOTOR_ERROR_DRV_FAULT = 0x8,
	MOTOR_ERROR_CONTROL_DEADLINE_MISSED = 0x10,
	MOTOR_ERROR_MODULATION_MAGNITUDE = 0x80,
	MOTOR_ERROR_CURRENT_SENSE_SATURATION = 0x400,
	MOTOR_ERROR_CURRENT_LIMIT_VIOLATION = 0x1000,
	MOTOR_ERROR_MODULATION_IS_NAN = 0x10000,
	MOTOR_ERROR_MOTOR_THERMISTOR_OVER_TEMP = 0x20000,
	MOTOR_ERROR_FET_THERMISTOR_OVER_TEMP = 0x40000,
	MOTOR_ERROR_TIMER_UPDATE_MISSED = 0x80000,
	MOTOR_ERROR_CURRENT_MEASUREMENT_UNAVAILABLE = 0x100000,
	MOTOR_ERROR_CONTROLLER_FAILED = 0x200000,
	MOTOR_ERROR_I_BUS_OUT_OF_RANGE = 0x400000,
	MOTOR_ERROR_BRAKE_RESISTOR_DISARMED = 0x800000,
	MOTOR_ERROR_SYSTEM_LEVEL = 0x1000000,
	MOTOR_ERROR_BAD_TIMING = 0x2000000,
	MOTOR_ERROR_UNKNOWN_PHASE_ESTIMATE = 0x4000000,
	MOTOR_ERROR_UNKNOWN_PHASE_VEL = 0x8000000,
	MOTOR_ERROR_UNKNOWN_TORQUE = 0x10000000,
	MOTOR_ERROR_UNKNOWN_CURRENT_COMMAND = 0x20000000,
	MOTOR_ERROR_UNKNOWN_CURRENT_MEASUREMENT = 0x40000000
	//MOTOR_ERROR_UNKNOWN_VBUS_VOLTAGE = 0x80000000,
	//MOTOR_ERROR_UNKNOWN_VOLTAGE_COMMAND = 0x100000000,
	//MOTOR_ERROR_UNKNOWN_GAINS = 0x200000000,
	//MOTOR_ERROR_CONTROLLER_INITIALIZING = 0x400000000,
	//MOTOR_ERROR_UNBALANCED_PHASES = 0x800000000,
);

static std::map<std::string, std::string> MotorErrorDesc = {
	{ "MOTOR_ERROR_CONTROL_DEADLINE_MISSED", "Is a result of other errors" }
};





BETTER_ENUM(EncoderError, int32_t,
	ENCODER_ERROR_NONE = 0x0,
	ENCODER_ERROR_UNSTABLE_GAIN = 0x1,
	ENCODER_ERROR_CPR_POLEPAIRS_MISMATCH = 0x2,
	ENCODER_ERROR_NO_RESPONSE = 0x4,
	ENCODER_ERROR_UNSUPPORTED_ENCODER_MODE = 0x8,
	ENCODER_ERROR_ILLEGAL_HALL_STATE = 0x10,
	ENCODER_ERROR_INDEX_NOT_FOUND_YET = 0x20,
	ENCODER_ERROR_ABS_SPI_TIMEOUT = 0x40,
	ENCODER_ERROR_ABS_SPI_COM_FAIL = 0x80,
	ENCODER_ERROR_ABS_SPI_NOT_READY = 0x100,
	ENCODER_ERROR_HALL_NOT_CALIBRATED_YET = 0x200
);

static std::map<std::string, std::string> EncoderErrorDesc = {
	{ "", "" }
};





BETTER_ENUM(ControllerError, int32_t,
	CONTROLLER_ERROR_NONE = 0x0,
	CONTROLLER_ERROR_OVERSPEED = 0x1,
	CONTROLLER_ERROR_INVALID_INPUT_MODE = 0x2,
	CONTROLLER_ERROR_UNSTABLE_GAIN = 0x4,
	CONTROLLER_ERROR_INVALID_MIRROR_AXIS = 0x8,
	CONTROLLER_ERROR_INVALID_LOAD_ENCODER = 0x10,
	CONTROLLER_ERROR_INVALID_ESTIMATE = 0x20,
	CONTROLLER_ERROR_INVALID_CIRCULAR_RANGE = 0x40,
	CONTROLLER_ERROR_SPINOUT_DETECTED = 0x80
);

static std::map<std::string, std::string> ControllerErrorDesc = {
	{ "", "" }
};




BETTER_ENUM(AxisRequestedState, int32_t,
	AXIS_STATE_UNDEFINED							= 0x00,
	AXIS_STATE_IDLE									= 0x01,
	AXIS_STATE_STARTUP_SEQUENCE						= 0x02,
	AXIS_STATE_FULL_CALIBRATION_SEQUENCE			= 0x03,
	AXIS_STATE_MOTOR_CALIBRATION					= 0x04,
	AXIS_STATE_ENCODER_INDEX_SEARCH					= 0x06,
	AXIS_STATE_ENCODER_OFFSET_CALIBRATION			= 0x07,
	AXIS_STATE_CLOSED_LOOP_CONTROL					= 0x08,
	AXIS_STATE_LOCKIN_SPIN							= 0x09,
	AXIS_STATE_ENCODER_DIR_FIND						= 0x0A,
	AXIS_STATE_HOMING								= 0x0B,
	AXIS_STATE_ENCODER_HALL_POLARITY_CALIBRATION	= 0x0C,
	AXIS_STATE_ENCODER_HALL_PHASE_CALIBRATION		= 0x0D
);











#define ASSIGN_ENUM_STRING(string, _enum) else if (ep.identifier == string) { return _enum::_from_integral(value)._to_string(); }

inline static const std::string EndpointValueToEnumName(const BasicEndpoint& ep, int32_t value) {
	
	if (false) {}
	ASSIGN_ENUM_STRING("axis0.requested_state", AxisRequestedState)
	ASSIGN_ENUM_STRING("axis1.requested_state", AxisRequestedState)
	ASSIGN_ENUM_STRING("axis0.current_state", AxisRequestedState)
	ASSIGN_ENUM_STRING("axis1.current_state", AxisRequestedState)

	return "";
}

#define ASSIGN_ENUM_INDEX(string, _enum) else if (ep.identifier == string) { return (size_t)_enum::_from_index(index); }

inline static size_t EnumIndexToValue(const BasicEndpoint& ep, size_t index) {
	
	if (false) {}
	ASSIGN_ENUM_INDEX("axis0.requested_state", AxisRequestedState)
	ASSIGN_ENUM_INDEX("axis1.requested_state", AxisRequestedState)
	ASSIGN_ENUM_INDEX("axis0.current_state", AxisRequestedState)
	ASSIGN_ENUM_INDEX("axis1.current_state", AxisRequestedState)

	return -1;
}

template<typename T>
constexpr static std::vector<std::string> ListEnumValues() {
	std::vector<std::string> vector;
	for (T element : T::_values()) {
		vector.push_back(element._to_string());
	}
	return vector;
}

#define ASSIGN_ENUM_LIST(string, _enum) else if (ep.identifier == string) { return ListEnumValues<_enum>(); }

inline static const std::vector<std::string> ListEnumValues(const BasicEndpoint& ep) {

	if (false) {}
	ASSIGN_ENUM_LIST("axis0.requested_state", AxisRequestedState)
	ASSIGN_ENUM_LIST("axis1.requested_state", AxisRequestedState)
	ASSIGN_ENUM_LIST("axis0.current_state", AxisRequestedState)
	ASSIGN_ENUM_LIST("axis1.current_state", AxisRequestedState)

	return {};
}
