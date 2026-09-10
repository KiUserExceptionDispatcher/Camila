#pragma once
#include <Windows.h>
#include <string>
#include <string_view>
#include <cstdint>

namespace camila {

	struct memory_region {
		uintptr_t base_address{0};
		uintptr_t allocation_base{0};
		uint32_t allocation_protect{0};
		size_t size{0};
		uint32_t state{0};
		uint32_t protect{0};
		uint32_t type{0};

		[[nodiscard]] bool is_committed() const noexcept { return state == MEM_COMMIT; }
		[[nodiscard]] bool is_reserved() const noexcept { return state == MEM_RESERVE; }
		[[nodiscard]] bool is_free() const noexcept { return state == MEM_FREE; }

		[[nodiscard]] bool is_readable() const noexcept {
			if (!is_committed() || (protect & PAGE_NOACCESS) || (protect & PAGE_GUARD)) return false;
			return (protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY |
			                   PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0;
		}

		[[nodiscard]] bool is_writable() const noexcept {
			if (!is_committed() || (protect & PAGE_NOACCESS) || (protect & PAGE_GUARD)) return false;
			return (protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0;
		}

		[[nodiscard]] bool is_executable() const noexcept {
			if (!is_committed() || (protect & PAGE_NOACCESS) || (protect & PAGE_GUARD)) return false;
			return (protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0;
		}

		[[nodiscard]] bool is_guarded() const noexcept {
			return (protect & PAGE_GUARD) != 0;
		}

		[[nodiscard]] std::string protect_string() const;
		[[nodiscard]] std::string state_string() const;
		[[nodiscard]] std::string type_string() const;
	};

}
