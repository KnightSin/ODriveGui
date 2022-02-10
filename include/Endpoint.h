#pragma once

#include <string>
#include <vector>
#include <optional>

// TODO: Function inputs & outputs

struct Endpoint {
	std::string path;
	std::string name;
	size_t id = 0;
	bool readonly = false;	// Only valid for numeric types
	bool isNumeric = false;
	bool isFunction = false;

	std::vector<Endpoint> children;
	std::vector<Endpoint> inputs;	// Only valid for functions
	std::vector<Endpoint> outputs;	// Only valid for functions
};
