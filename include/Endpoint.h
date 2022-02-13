#pragma once

#include <string>
#include <vector>
#include <optional>
#include <type_traits>

enum class EndpointType {
	INVALID,
	JSON,
	BOOL,
	FLOAT,
	UINT8,
	UINT16,
	UINT32,
	UINT64,
	INT32,
	FUNCTION,
	OBJECT
};

enum class EndpointValueType {
	INVALID,
	BOOL,
	FLOAT,
	UINT8,
	UINT16,
	UINT32,
	UINT64,
	INT32
};

struct BasicEndpoint {
	std::string identifier;
	std::string name;
	std::string type;
	std::string fullPath;
	int odriveID = 0;
	uint16_t id = 0;
	bool readonly = false;	// Only valid for numeric types
};

struct Endpoint {

	BasicEndpoint basic;

	std::vector<Endpoint> children;
	std::vector<Endpoint> inputs;	// Only valid for functions
	std::vector<Endpoint> outputs;	// Only valid for functions

	BasicEndpoint* operator->() {
		return &basic;
	}
};

class EndpointValue {
public:
	EndpointValue() : _type(EndpointValueType::INVALID) {
	}

	EndpointValue(enum EndpointValueType type) : _type(type) {
	}

	EndpointValue(const std::string& type) {
		if (type == "bool")		_type = EndpointValueType::BOOL;
		if (type == "float")	_type = EndpointValueType::FLOAT;
		if (type == "uint8")	_type = EndpointValueType::UINT8;
		if (type == "uint16")	_type = EndpointValueType::UINT16;
		if (type == "uint32")	_type = EndpointValueType::UINT32;
		if (type == "uint64")	_type = EndpointValueType::UINT64;
		if (type == "int32")	_type = EndpointValueType::INT32;
	}

	EndpointValue(bool value)		{ set(value); _type = EndpointValueType::BOOL; }
	EndpointValue(float value)		{ set(value); _type = EndpointValueType::FLOAT; }
	EndpointValue(uint8_t value)	{ set(value); _type = EndpointValueType::UINT8; }
	EndpointValue(uint16_t value)	{ set(value); _type = EndpointValueType::UINT16; }
	EndpointValue(uint32_t value)	{ set(value); _type = EndpointValueType::UINT32; }
	EndpointValue(uint64_t value)	{ set(value); _type = EndpointValueType::UINT64; }
	EndpointValue(int32_t value)	{ set(value); _type = EndpointValueType::INT32; }

	template<typename T>
	T get() const {
		T value;
		memcpy(&value, &this->value, sizeof(T));
		return value;
	}

	template<typename T>
	void set(T value) {
		memcpy(&this->value, &value, sizeof(T));
	}
	
	std::string toString() {
		std::string str;
		std::stringstream s;
		switch (type()) {
		case EndpointValueType::BOOL:	str = get<bool>() ? "true" : "false";	break;
		case EndpointValueType::FLOAT:	s << std::fixed << std::setprecision(6) << get<float>() << "f"; str = s.str(); break;
		case EndpointValueType::UINT8:	str = std::to_string(get<uint8_t>()); break;
		case EndpointValueType::UINT16:	str = std::to_string(get<uint16_t>()); break;
		case EndpointValueType::UINT32:	str = std::to_string(get<uint32_t>()); break;
		case EndpointValueType::UINT64:	str = std::to_string(get<uint64_t>()); break;
		case EndpointValueType::INT32:	str = std::to_string(get<int32_t>()); break;
		}
		return str;
	}

	bool fromString(const std::string& str) {
		try {
			switch (type()) {
			case EndpointValueType::FLOAT:	set<float>(std::stof(str)); return true;
			case EndpointValueType::UINT8:	set<uint8_t>(std::stoi(str)); return true;
			case EndpointValueType::UINT16:	set<uint16_t>(std::stoi(str)); return true;
			case EndpointValueType::UINT32:	set<uint32_t>(std::stol(str)); return true;
			case EndpointValueType::UINT64:	set<uint64_t>(std::stoll(str)); return true;
			case EndpointValueType::INT32:	set<int32_t>(std::stol(str)); return true;
			}
		}
		catch (...) {}
		return false;
	}

	template<typename T>
	void operator=(T value) {
		set<T>(value);
	}

	bool operator==(const EndpointValue& other) {
		return (_type == other._type) && (this->value == other.value);
	}

	bool operator!=(const EndpointValue& other) {
		return !operator==(other);
	}

	enum EndpointValueType type() const {
		return _type;
	}

private:
	uint64_t value = 0;
	enum EndpointValueType _type = EndpointValueType::INVALID;
};
