#include "memory_region.hpp"

namespace camila {

	std::string memory_region::protect_string() const {
		std::string result;
		DWORD base_protect = protect & 0xFF;

		switch (base_protect) {
			case PAGE_NOACCESS:          result = "PAGE_NOACCESS"; break;
			case PAGE_READONLY:          result = "PAGE_READONLY"; break;
			case PAGE_READWRITE:         result = "PAGE_READWRITE"; break;
			case PAGE_WRITECOPY:         result = "PAGE_WRITECOPY"; break;
			case PAGE_EXECUTE:           result = "PAGE_EXECUTE"; break;
			case PAGE_EXECUTE_READ:      result = "PAGE_EXECUTE_READ"; break;
			case PAGE_EXECUTE_READWRITE: result = "PAGE_EXECUTE_READWRITE"; break;
			case PAGE_EXECUTE_WRITECOPY: result = "PAGE_EXECUTE_WRITECOPY"; break;
			default:                     result = "PAGE_UNKNOWN"; break;
		}

		if (protect & PAGE_GUARD)        result += " | PAGE_GUARD";
		if (protect & PAGE_NOCACHE)      result += " | PAGE_NOCACHE";
		if (protect & PAGE_WRITECOMBINE) result += " | PAGE_WRITECOMBINE";

		return result;
	}

	std::string memory_region::state_string() const {
		switch (state) {
			case MEM_COMMIT:  return "MEM_COMMIT";
			case MEM_RESERVE: return "MEM_RESERVE";
			case MEM_FREE:    return "MEM_FREE";
			default:          return "MEM_UNKNOWN";
		}
	}

	std::string memory_region::type_string() const {
		switch (type) {
			case MEM_IMAGE:   return "MEM_IMAGE";
			case MEM_MAPPED:  return "MEM_MAPPED";
			case MEM_PRIVATE: return "MEM_PRIVATE";
			default:          return "MEM_UNKNOWN";
		}
	}

}
