#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <optional>
#include <iostream>
#include <format>

namespace camila {

	class argument_parser {
	public:
		struct argument {
			std::string name;
			std::string short_flag;
			std::string long_flag;
			std::string description;
			std::string default_value;
			bool is_flag{false};
			bool required{false};
			bool present{false};
			std::string parsed_value;
		};

		explicit argument_parser(std::string program_name, std::string description = "")
			: m_program_name(std::move(program_name)), m_description(std::move(description)) {
			add_flag("-h", "--help", "Show help and usage information");
		}

		argument_parser& add_argument(std::string short_flag, std::string long_flag, std::string description, std::string default_val = "", bool required = false) {
			argument arg;
			arg.name = !long_flag.empty() ? long_flag : short_flag;
			arg.short_flag = std::move(short_flag);
			arg.long_flag = std::move(long_flag);
			arg.description = std::move(description);
			arg.default_value = std::move(default_val);
			arg.parsed_value = arg.default_value;
			arg.is_flag = false;
			arg.required = required;
			m_args.push_back(std::move(arg));
			return *this;
		}

		argument_parser& add_flag(std::string short_flag, std::string long_flag, std::string description) {
			argument arg;
			arg.name = !long_flag.empty() ? long_flag : short_flag;
			arg.short_flag = std::move(short_flag);
			arg.long_flag = std::move(long_flag);
			arg.description = std::move(description);
			arg.is_flag = true;
			arg.present = false;
			m_args.push_back(std::move(arg));
			return *this;
		}

		bool parse(int argc, char* argv[]) {
			for (int i = 1; i < argc; ++i) {
				std::string arg_str = argv[i];

				bool matched = false;
				for (auto& arg : m_args) {
					if ((!arg.short_flag.empty() && arg_str == arg.short_flag) ||
					    (!arg.long_flag.empty() && arg_str == arg.long_flag)) {
						arg.present = true;
						matched = true;

						if (!arg.is_flag) {
							if (i + 1 < argc) {
								arg.parsed_value = argv[++i];
							} else {
								std::cerr << "Error: Flag '" << arg_str << "' requires a value.\n";
								return false;
							}
						}
						break;
					}
				}

				if (!matched) {
					m_positional.push_back(arg_str);
				}
			}

			if (is_set("-h") || is_set("--help")) {
				print_help();
				return false;
			}

			for (const auto& arg : m_args) {
				if (arg.required && !arg.present) {
					std::cerr << "Error: Required argument '" << arg.name << "' is missing.\n";
					print_help();
					return false;
				}
			}

			return true;
		}

		[[nodiscard]] bool is_set(std::string_view flag) const {
			for (const auto& arg : m_args) {
				if (arg.short_flag == flag || arg.long_flag == flag || arg.name == flag) {
					return arg.present;
				}
			}
			return false;
		}

		[[nodiscard]] std::string get(std::string_view name) const {
			for (const auto& arg : m_args) {
				if (arg.short_flag == name || arg.long_flag == name || arg.name == name) {
					return arg.parsed_value;
				}
			}
			return "";
		}

		template <typename T>
		[[nodiscard]] T get_as(std::string_view name, T default_val = {}) const {
			std::string val_str = get(name);
			if (val_str.empty()) return default_val;

			try {
				if constexpr (std::is_integral_v<T>) {
					if (val_str.starts_with("0x") || val_str.starts_with("0X")) {
						return static_cast<T>(std::stoull(val_str, nullptr, 16));
					}
					return static_cast<T>(std::stoull(val_str, nullptr, 10));
				} else if constexpr (std::is_floating_point_v<T>) {
					return static_cast<T>(std::stod(val_str));
				} else {
					return val_str;
				}
			} catch (...) {
				return default_val;
			}
		}

		[[nodiscard]] const std::vector<std::string>& get_positional() const noexcept {
			return m_positional;
		}

		void print_help() const {
			std::cout << "\n=======================================================\n";
			std::cout << "  " << m_program_name << "\n";
			if (!m_description.empty()) {
				std::cout << "  " << m_description << "\n";
			}
			std::cout << "=======================================================\n";
			std::cout << "\nUsage:\n  " << m_program_name << " [options]\n\nOptions:\n";

			for (const auto& arg : m_args) {
				std::string flags;
				if (!arg.short_flag.empty() && !arg.long_flag.empty()) {
					flags = std::format("{}, {}", arg.short_flag, arg.long_flag);
				} else if (!arg.long_flag.empty()) {
					flags = arg.long_flag;
				} else {
					flags = arg.short_flag;
				}

				if (!arg.is_flag) {
					flags += " <val>";
				}

				std::cout << std::format("  {:<25} {}", flags, arg.description);
				if (!arg.default_value.empty()) {
					std::cout << " (default: " << arg.default_value << ")";
				}
				if (arg.required) {
					std::cout << " [required]";
				}
				std::cout << "\n";
			}
			std::cout << "\n";
		}

	private:
		std::string m_program_name;
		std::string m_description;
		std::vector<argument> m_args;
		std::vector<std::string> m_positional;
	};

}
