#pragma once
#include <Windows.h>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <span>
#include <cstdint>
#include "../core/error.hpp"
#include "../core/nt.hpp"
#include "../module/module.hpp"
#include "memory_region.hpp"

namespace camila {

	class process {
	public:
		process() = default;
		explicit process(std::string_view process_name);
		explicit process(uint32_t pid);
		~process();

		process(const process&) = delete;
		process& operator=(const process&) = delete;
		process(process&& other) noexcept;
		process& operator=(process&& other) noexcept;

		bool attach(std::string_view process_name);
		bool attach(uint32_t pid);
		void detach();

		[[nodiscard]] bool is_valid() const noexcept { return m_handle != nullptr && (m_handle == GetCurrentProcess() || m_handle != INVALID_HANDLE_VALUE); }
		[[nodiscard]] explicit operator bool() const noexcept { return is_valid(); }
		[[nodiscard]] HANDLE get_handle() const noexcept { return m_handle; }
		[[nodiscard]] uint32_t get_pid() const noexcept { return m_process_id; }
		[[nodiscard]] uintptr_t get_base_address() const noexcept { return m_base_address; }
		[[nodiscard]] const std::string& get_name() const noexcept { return m_process_name; }

		template <typename T>
		[[nodiscard]] result<T> read(uintptr_t address) const {
			static_assert(!std::is_pointer_v<T>, "For pointer types, read the pointed object type instead");
			T buffer{};
			if (!read_raw(address, &buffer, sizeof(T))) {
				return std::unexpected(error(error_code::read_failed, std::format("Failed to read {} bytes at 0x{:X}", sizeof(T), address)));
			}
			return buffer;
		}

		template <typename T>
		[[nodiscard]] T read_value(uintptr_t address, T default_value = {}) const noexcept {
			auto res = read<T>(address);
			return res.value_or(default_value);
		}

		[[nodiscard]] result<std::vector<uint8_t>> read_bytes(uintptr_t address, size_t size) const;
		[[nodiscard]] result<std::string> read_string(uintptr_t address, size_t max_length = 256) const;
		[[nodiscard]] result<std::wstring> read_wstring(uintptr_t address, size_t max_length = 256) const;
		bool read_raw(uintptr_t address, void* buffer, size_t size) const;

		template <typename T>
		bool write(uintptr_t address, const T& value) const {
			return write_raw(address, &value, sizeof(T));
		}

		bool write_bytes(uintptr_t address, const void* data, size_t size) const;
		bool write_string(uintptr_t address, std::string_view str) const;
		bool write_raw(uintptr_t address, const void* buffer, size_t size) const;

		[[nodiscard]] result<uintptr_t> allocate(size_t size, uint32_t protect = PAGE_EXECUTE_READWRITE, uint32_t allocation_type = MEM_COMMIT | MEM_RESERVE, uintptr_t preferred_address = 0) const;
		[[nodiscard]] result<uint32_t> protect(uintptr_t address, size_t size, uint32_t new_protect) const;
		bool free(uintptr_t address, size_t size = 0, uint32_t free_type = MEM_RELEASE) const;

		[[nodiscard]] result<memory_region> query_region(uintptr_t address) const;
		[[nodiscard]] std::vector<memory_region> enumerate_regions(uintptr_t start_addr = 0, uintptr_t end_addr = 0x7FFFFFFFFFFF) const;

		[[nodiscard]] module_info get_module(std::string_view module_name) const;
		[[nodiscard]] std::vector<module_info> get_modules() const;

		[[nodiscard]] result<uintptr_t> pattern_scan(std::string_view pattern, std::string_view module_name = "") const;
		[[nodiscard]] result<uintptr_t> pattern_scan_region(std::string_view pattern, uintptr_t start_address, size_t region_size) const;

		[[nodiscard]] static uint32_t find_process_id(std::string_view process_name);

	private:
		HANDLE m_handle{nullptr};
		uint32_t m_process_id{0};
		uintptr_t m_base_address{0};
		std::string m_process_name{};
	};

	class memory {
	public:
		static bool attach_to_process(std::string_view process_name);
		static uint32_t find_process_id(std::string_view process_name);
		static uintptr_t find_module(std::string_view module_name);
		static bool read_memory(uintptr_t address, void* buffer, size_t size);
		static bool write_memory(uintptr_t address, const void* buffer, size_t size);

		template <typename T>
		static T read(uintptr_t address) {
			T buffer{};
			read_memory(address, &buffer, sizeof(T));
			return buffer;
		}

		template <typename T>
		static T write(uintptr_t address, T value) {
			write_memory(address, &value, sizeof(T));
			return value;
		}

		static bool protect(uintptr_t address, size_t size, uint32_t new_protect, uint32_t* old_protect);
		static void allocate(uintptr_t address, size_t size, uint32_t allocation_type, uint32_t protect);

		static HANDLE process_handle;
		static uint32_t process_id;
		static uintptr_t base_address;
	};

}