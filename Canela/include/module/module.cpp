#include "module.hpp"
#include <tlhelp32.h>
#include <Psapi.h>
#include <algorithm>
#include <cctype>

namespace camila {

	namespace detail {
		inline bool iequals(std::string_view a, std::string_view b) {
			return std::ranges::equal(a, b, [](char c1, char c2) {
				return std::tolower(static_cast<unsigned char>(c1)) ==
				       std::tolower(static_cast<unsigned char>(c2));
			});
		}
	}

	uintptr_t module_info::get_export(std::string_view export_name, HANDLE process_handle) const {
		if (!is_valid()) return 0;

		auto read_proc_mem = [process_handle](uintptr_t addr, void* buf, size_t sz) -> bool {
			if (!process_handle || process_handle == GetCurrentProcess()) {
				memcpy(buf, reinterpret_cast<const void*>(addr), sz);
				return true;
			}
			SIZE_T read_bytes = 0;
			return ReadProcessMemory(process_handle, reinterpret_cast<LPCVOID>(addr), buf, sz, &read_bytes) && (read_bytes == sz);
		};

		IMAGE_DOS_HEADER dos_header{};
		if (!read_proc_mem(base, &dos_header, sizeof(dos_header)) || dos_header.e_magic != IMAGE_DOS_SIGNATURE) {
			return 0;
		}

		IMAGE_NT_HEADERS nt_headers{};
		if (!read_proc_mem(base + dos_header.e_lfanew, &nt_headers, sizeof(nt_headers)) || nt_headers.Signature != IMAGE_NT_SIGNATURE) {
			return 0;
		}

		const auto& export_entry = nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
		if (export_entry.VirtualAddress == 0 || export_entry.Size == 0) {
			return 0;
		}

		IMAGE_EXPORT_DIRECTORY export_dir{};
		if (!read_proc_mem(base + export_entry.VirtualAddress, &export_dir, sizeof(export_dir))) {
			return 0;
		}

		std::vector<DWORD> functions(export_dir.NumberOfFunctions);
		std::vector<DWORD> names(export_dir.NumberOfNames);
		std::vector<WORD> ordinals(export_dir.NumberOfNames);

		if (!read_proc_mem(base + export_dir.AddressOfFunctions, functions.data(), functions.size() * sizeof(DWORD)) ||
		    !read_proc_mem(base + export_dir.AddressOfNames, names.data(), names.size() * sizeof(DWORD)) ||
		    !read_proc_mem(base + export_dir.AddressOfNameOrdinals, ordinals.data(), ordinals.size() * sizeof(WORD))) {
			return 0;
		}

		for (DWORD i = 0; i < export_dir.NumberOfNames; ++i) {
			char name_buf[256]{};
			if (read_proc_mem(base + names[i], name_buf, sizeof(name_buf) - 1)) {
				if (export_name == name_buf) {
					WORD ordinal = ordinals[i];
					if (ordinal < export_dir.NumberOfFunctions) {
						return base + functions[ordinal];
					}
				}
			}
		}

		return 0;
	}

	module_info module::enumerate(std::string_view module_name, uint32_t pid) const {
		uint32_t target_pid = pid != 0 ? pid : (m_process_id != 0 ? m_process_id : GetCurrentProcessId());
		auto res = find(module_name, target_pid);
		if (res.has_value()) {
			return res.value();
		}
		return module_info{};
	}

	result<module_info> module::find(std::string_view module_name, uint32_t pid) {
		uint32_t target_pid = pid != 0 ? pid : GetCurrentProcessId();

		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, target_pid);
		if (snapshot == INVALID_HANDLE_VALUE) {
			return std::unexpected(error(error_code::process_not_found, "Failed to create module snapshot", GetLastError()));
		}

		MODULEENTRY32W me{};
		me.dwSize = sizeof(MODULEENTRY32W);

		if (Module32FirstW(snapshot, &me)) {
			do {
				char mod_name_a[MAX_MODULE_NAME32 + 1]{};
				WideCharToMultiByte(CP_UTF8, 0, me.szModule, -1, mod_name_a, sizeof(mod_name_a), nullptr, nullptr);

				char exe_path_a[MAX_PATH]{};
				WideCharToMultiByte(CP_UTF8, 0, me.szExePath, -1, exe_path_a, sizeof(exe_path_a), nullptr, nullptr);

				if (module_name.empty() || detail::iequals(mod_name_a, module_name)) {
					module_info info{
						.name = mod_name_a,
						.path = exe_path_a,
						.base = reinterpret_cast<uintptr_t>(me.modBaseAddr),
						.size = static_cast<size_t>(me.modBaseSize),
						.entry_point = 0,
						.process_id = target_pid
					};
					CloseHandle(snapshot);
					return info;
				}
			} while (Module32NextW(snapshot, &me));
		}

		CloseHandle(snapshot);
		return std::unexpected(error(error_code::module_not_found, std::format("Module '{}' not found in process {}", module_name, target_pid)));
	}

	std::vector<module_info> module::enumerate_all(uint32_t pid) {
		std::vector<module_info> modules;
		uint32_t target_pid = pid != 0 ? pid : GetCurrentProcessId();

		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, target_pid);
		if (snapshot == INVALID_HANDLE_VALUE) {
			return modules;
		}

		MODULEENTRY32W me{};
		me.dwSize = sizeof(MODULEENTRY32W);

		if (Module32FirstW(snapshot, &me)) {
			do {
				char mod_name_a[MAX_MODULE_NAME32 + 1]{};
				WideCharToMultiByte(CP_UTF8, 0, me.szModule, -1, mod_name_a, sizeof(mod_name_a), nullptr, nullptr);

				char exe_path_a[MAX_PATH]{};
				WideCharToMultiByte(CP_UTF8, 0, me.szExePath, -1, exe_path_a, sizeof(exe_path_a), nullptr, nullptr);

				modules.push_back(module_info{
					.name = mod_name_a,
					.path = exe_path_a,
					.base = reinterpret_cast<uintptr_t>(me.modBaseAddr),
					.size = static_cast<size_t>(me.modBaseSize),
					.entry_point = 0,
					.process_id = target_pid
				});
			} while (Module32NextW(snapshot, &me));
		}

		CloseHandle(snapshot);
		return modules;
	}

}
