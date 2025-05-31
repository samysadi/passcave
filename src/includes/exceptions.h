#pragma once

#include <stdexcept>

namespace passcave {
    class IOException;
}

class passcave::IOException: public std::runtime_error {
public:
	IOException(std::string const& __arg): std::runtime_error(__arg) {}
};
