#pragma once
#include <iostream>
#include <string>
#include <string_view>
#include <format>
#include <chrono>
#include <mutex>
#include <Windows.h>

namespace camila {

	enum class log_level {
		trace = 0,
		debug,
		info,
		warn,
		error,
		success
	};

	class logger {
	public:
		static void set_level(log_level level) noexcept {
			get_instance().m_min_level = level;
		}

		static void set_timestamps(bool enabled) noexcept {
			get_instance().m_timestamps = enabled;
		}

		template <typename... Args>
		static void trace(std::format_string<Args...> fmt, Args&&... args) {
			log(log_level::trace, fmt, std::forward<Args>(args)...);
		}

		template <typename... Args>
		static void debug(std::format_string<Args...> fmt, Args&&... args) {
			log(log_level::debug, fmt, std::forward<Args>(args)...);
		}

		template <typename... Args>
		static void info(std::format_string<Args...> fmt, Args&&... args) {
			log(log_level::info, fmt, std::forward<Args>(args)...);
		}

		template <typename... Args>
		static void warn(std::format_string<Args...> fmt, Args&&... args) {
			log(log_level::warn, fmt, std::forward<Args>(args)...);
		}

		template <typename... Args>
		static void error(std::format_string<Args...> fmt, Args&&... args) {
			log(log_level::error, fmt, std::forward<Args>(args)...);
		}

		template <typename... Args>
		static void success(std::format_string<Args...> fmt, Args&&... args) {
			log(log_level::success, fmt, std::forward<Args>(args)...);
		}

	private:
		static logger& get_instance() {
			static logger inst;
			return inst;
		}

		logger() {
			HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
			if (hOut != INVALID_HANDLE_VALUE) {
				DWORD mode = 0;
				if (GetConsoleMode(hOut, &mode)) {
					SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
				}
			}
		}

		template <typename... Args>
		static void log(log_level level, std::format_string<Args...> fmt, Args&&... args) {
			auto& inst = get_instance();
			if (level < inst.m_min_level) return;

			std::string formatted_msg = std::format(fmt, std::forward<Args>(args)...);

			std::lock_guard<std::mutex> lock(inst.m_mutex);

			std::string time_str;
			if (inst.m_timestamps) {
				auto now = std::chrono::system_clock::now();
				time_str = std::format("[{:%H:%M:%S}] ", std::chrono::floor<std::chrono::seconds>(now));
			}

			const char* color_code = "";
			const char* level_tag = "";

			switch (level) {
				case log_level::trace:
					color_code = "\033[90m";
					level_tag = "[TRACE]";
					break;
				case log_level::debug:
					color_code = "\033[36m";
					level_tag = "[DEBUG]";
					break;
				case log_level::info:
					color_code = "\033[34m";
					level_tag = "[INFO ]";
					break;
				case log_level::warn:
					color_code = "\033[33m";
					level_tag = "[WARN ]";
					break;
				case log_level::error:
					color_code = "\033[31m";
					level_tag = "[ERROR]";
					break;
				case log_level::success:
					color_code = "\033[32m";
					level_tag = "[SUCCESS]";
					break;
			}

			std::cout << color_code << time_str << level_tag << " " << formatted_msg << "\033[0m\n";
		}

		log_level m_min_level{log_level::info};
		bool m_timestamps{true};
		std::mutex m_mutex;
	};

}
