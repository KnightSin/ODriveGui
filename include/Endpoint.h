#pragma once

#include <string>
#include <vector>
#include <optional>

// TODO: Function inputs & outputs

struct Endpoint {
	std::string identifier;
	std::string name;
	std::string type;
	uint16_t id = 0;
	bool readonly = false;	// Only valid for numeric types

	std::vector<Endpoint> children;
	std::vector<Endpoint> inputs;	// Only valid for functions
	std::vector<Endpoint> outputs;	// Only valid for functions
};
