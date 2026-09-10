#pragma once
#include <Windows.h>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <cstdint>
#include "../core/error.hpp"

namespace camila {

	struct module_info {
		std::string name{};
		std::string path{};
		uintptr_t base{0};
		size_t size{0};
		uintptr_t entry_point{0};
		uint32_t process_id{0};

		[[nodiscard]] bool is_valid() const noexcept {
			return base != 0;
		}

		[[nodiscard]] uintptr_t get_export(std::string_view export_name, HANDLE process_handle = nullptr) const;
	};

	class module {
	public:
		module() : m_process_id(0) {}
		explicit module(uint32_t process_id) : m_process_id(process_id) {}

		[[nodiscard]] module_info enumerate(std::string_view module_name, uint32_t pid = 0) const;

		[[nodiscard]] static result<module_info> find(std::string_view module_name, uint32_t pid = 0);
		[[nodiscard]] static std::vector<module_info> enumerate_all(uint32_t pid = 0);

		void set_process_id(uint32_t pid) noexcept { m_process_id = pid; }
		[[nodiscard]] uint32_t get_process_id() const noexcept { return m_process_id; }

	private:
		uint32_t m_process_id{0};
	};

}
