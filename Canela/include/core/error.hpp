#pragma once
#include <string>
#include <string_view>
#include <expected>
#include <format>
#include <cstdint>

namespace camila {

	enum class error_code : uint32_t {
		success = 0,
		process_not_found,
		access_denied,
		invalid_handle,
		read_failed,
		write_failed,
		protect_failed,
		allocation_failed,
		free_failed,
		module_not_found,
		export_not_found,
		region_query_failed,
		pattern_not_found,
		invalid_parameter,
		system_error
	};

	inline constexpr std::string_view to_string(error_code code) noexcept {
		switch (code) {
			case error_code::success: return "Success";
			case error_code::process_not_found: return "Process not found";
			case error_code::access_denied: return "Access denied";
			case error_code::invalid_handle: return "Invalid process handle";
			case error_code::read_failed: return "Failed to read virtual memory";
			case error_code::write_failed: return "Failed to write virtual memory";
			case error_code::protect_failed: return "Failed to change memory protection";
			case error_code::allocation_failed: return "Failed to allocate virtual memory";
			case error_code::free_failed: return "Failed to free virtual memory";
			case error_code::module_not_found: return "Module not found";
			case error_code::export_not_found: return "Exported function not found";
			case error_code::region_query_failed: return "Failed to query memory region";
			case error_code::pattern_not_found: return "Pattern signature not found";
			case error_code::invalid_parameter: return "Invalid parameter";
			case error_code::system_error: return "System error";
			default: return "Unknown error";
		}
	}

	struct error {
		error_code code{error_code::success};
		std::string message{};
		uint32_t native_status{0};

		error() = default;
		error(error_code c) : code(c), message(to_string(c)), native_status(0) {}
		error(error_code c, std::string_view msg, uint32_t status = 0)
			: code(c), message(msg), native_status(status) {}

		[[nodiscard]] std::string to_formatted_string() const {
			if (native_status != 0) {
				return std::format("[Error: {} (0x{:08X})] {}", to_string(code), native_status, message);
			}
			return std::format("[Error: {}] {}", to_string(code), message);
		}
	};

	template <typename T>
	using result = std::expected<T, camila::error>;

	using status_result = std::expected<void, camila::error>;

}
