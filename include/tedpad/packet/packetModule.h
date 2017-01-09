#pragma once

#include <inttypes.h>
#include <string>
#include <vector>
#include <algorithm>

#include "../definition.h"
#include "../socket.h"

namespace tedpad {
	struct PacketModule {
		std::string name;
		std::vector<uint8_t> data;
		//Whether or not to trust module data (eg there might have been a parsing error). Program must ensure set after the module has been manuallly set.
		bool valid = false;

		void set_dataSize(size_t const p0);
		size_t get_dataSize() const;
		size_t get_totalSize() const;

		std::vector<uint8_t> to_netbuf() const;
		void from_netbuf(std::vector<uint8_t> const &p0);
	};
}
