#include "memory.hpp"
#include <tlhelp32.h>
#include <sstream>
#include <vector>
#include <algorithm>

namespace camila {

	namespace detail {
		inline bool iequals(std::string_view a, std::string_view b) {
			return std::ranges::equal(a, b, [](char c1, char c2) {
				return std::tolower(static_cast<unsigned char>(c1)) ==
				       std::tolower(static_cast<unsigned char>(c2));
			});
		}
	}

	HANDLE memory::process_handle = nullptr;
	uint32_t memory::process_id = 0;
	uintptr_t memory::base_address = 0;

	process::process(std::string_view process_name) {
		attach(process_name);
	}

	process::process(uint32_t pid) {
		attach(pid);
	}

	process::~process() {
		detach();
	}

	process::process(process&& other) noexcept
		: m_handle(other.m_handle),
		  m_process_id(other.m_process_id),
		  m_base_address(other.m_base_address),
		  m_process_name(std::move(other.m_process_name)) {
		other.m_handle = nullptr;
		other.m_process_id = 0;
		other.m_base_address = 0;
	}

	process& process::operator=(process&& other) noexcept {
		if (this != &other) {
			detach();
			m_handle = other.m_handle;
			m_process_id = other.m_process_id;
			m_base_address = other.m_base_address;
			m_process_name = std::move(other.m_process_name);

			other.m_handle = nullptr;
			other.m_process_id = 0;
			other.m_base_address = 0;
		}
		return *this;
	}

	void process::detach() {
		if (m_handle && m_handle != INVALID_HANDLE_VALUE && m_handle != GetCurrentProcess()) {
			CloseHandle(m_handle);
		}
		m_handle = nullptr;
		m_process_id = 0;
		m_base_address = 0;
		m_process_name.clear();
	}

	bool process::attach(std::string_view process_name) {
		detach();
		uint32_t pid = find_process_id(process_name);
		if (pid == 0) {
			return false;
		}
		m_process_name = std::string(process_name);
		return attach(pid);
	}

	bool process::attach(uint32_t pid) {
		if (m_handle) {
			detach();
		}

		m_process_id = pid;
		if (pid == GetCurrentProcessId()) {
			m_handle = GetCurrentProcess();
		} else {
			const auto& api = nt::get_api();
			if (api.open_process) {
				OBJECT_ATTRIBUTES obj_attr{};
				obj_attr.Length = sizeof(OBJECT_ATTRIBUTES);
				nt::CLIENT_ID_EX cid{};
				cid.UniqueProcess = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(pid));
				cid.UniqueThread = nullptr;

				HANDLE h = nullptr;
				NTSTATUS status = api.open_process(&h, PROCESS_ALL_ACCESS, &obj_attr, &cid);
				if (NT_SUCCESS(status) && h) {
					m_handle = h;
				}
			}

			if (!m_handle) {
				m_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
				if (!m_handle) {
					m_handle = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, FALSE, pid);
				}
			}
		}

		if (!is_valid()) {
			m_handle = nullptr;
			return false;
		}

		auto main_mod = module::find("", m_process_id);
		if (main_mod.has_value()) {
			m_base_address = main_mod->base;
			if (m_process_name.empty()) {
				m_process_name = main_mod->name;
			}
		}

		return true;
	}

	uint32_t process::find_process_id(std::string_view process_name) {
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (snapshot == INVALID_HANDLE_VALUE) {
			return 0;
		}

		PROCESSENTRY32W pe{};
		pe.dwSize = sizeof(PROCESSENTRY32W);

		if (Process32FirstW(snapshot, &pe)) {
			do {
				char exe_name_a[MAX_PATH]{};
				WideCharToMultiByte(CP_UTF8, 0, pe.szExeFile, -1, exe_name_a, sizeof(exe_name_a), nullptr, nullptr);

				if (detail::iequals(exe_name_a, process_name)) {
					CloseHandle(snapshot);
					return pe.th32ProcessID;
				}
			} while (Process32NextW(snapshot, &pe));
		}

		CloseHandle(snapshot);
		return 0;
	}

	bool process::read_raw(uintptr_t address, void* buffer, size_t size) const {
		if (!is_valid() || !buffer || size == 0) return false;

		const auto& api = nt::get_api();
		if (api.read_virtual_memory) {
			SIZE_T bytes_read = 0;
			NTSTATUS status = api.read_virtual_memory(m_handle, reinterpret_cast<PVOID>(address), buffer, size, &bytes_read);
			if (NT_SUCCESS(status) && bytes_read == size) {
				return true;
			}
		}

		SIZE_T win32_bytes = 0;
		return ReadProcessMemory(m_handle, reinterpret_cast<LPCVOID>(address), buffer, size, &win32_bytes) && (win32_bytes == size);
	}

	bool process::write_raw(uintptr_t address, const void* buffer, size_t size) const {
		if (!is_valid() || !buffer || size == 0) return false;

		const auto& api = nt::get_api();
		if (api.write_virtual_memory) {
			SIZE_T bytes_written = 0;
			NTSTATUS status = api.write_virtual_memory(m_handle, reinterpret_cast<PVOID>(address), const_cast<PVOID>(buffer), size, &bytes_written);
			if (NT_SUCCESS(status) && bytes_written == size) {
				return true;
			}
		}

		SIZE_T win32_bytes = 0;
		return WriteProcessMemory(m_handle, reinterpret_cast<LPVOID>(address), buffer, size, &win32_bytes) && (win32_bytes == size);
	}

	result<std::vector<uint8_t>> process::read_bytes(uintptr_t address, size_t size) const {
		if (size == 0) return std::vector<uint8_t>{};
		std::vector<uint8_t> buffer(size);
		if (!read_raw(address, buffer.data(), size)) {
			return std::unexpected(error(error_code::read_failed, std::format("Failed to read {} bytes from 0x{:X}", size, address)));
		}
		return buffer;
	}

	result<std::string> process::read_string(uintptr_t address, size_t max_length) const {
		std::string str;
		str.reserve(max_length);

		for (size_t i = 0; i < max_length; ++i) {
			char ch = 0;
			if (!read_raw(address + i, &ch, sizeof(ch))) {
				if (str.empty()) {
					return std::unexpected(error(error_code::read_failed, std::format("Failed to read string from 0x{:X}", address)));
				}
				break;
			}
			if (ch == '\0') break;
			str.push_back(ch);
		}
		return str;
	}

	result<std::wstring> process::read_wstring(uintptr_t address, size_t max_length) const {
		std::wstring str;
		str.reserve(max_length);

		for (size_t i = 0; i < max_length; ++i) {
			wchar_t ch = 0;
			if (!read_raw(address + (i * sizeof(wchar_t)), &ch, sizeof(ch))) {
				if (str.empty()) {
					return std::unexpected(error(error_code::read_failed, std::format("Failed to read wide string from 0x{:X}", address)));
				}
				break;
			}
			if (ch == L'\0') break;
			str.push_back(ch);
		}
		return str;
	}

	bool process::write_bytes(uintptr_t address, const void* data, size_t size) const {
		return write_raw(address, data, size);
	}

	bool process::write_string(uintptr_t address, std::string_view str) const {
		return write_raw(address, str.data(), str.length() + 1);
	}

	result<uintptr_t> process::allocate(size_t size, uint32_t protect, uint32_t allocation_type, uintptr_t preferred_address) const {
		if (!is_valid()) {
			return std::unexpected(error(error_code::invalid_handle, "Process is not attached"));
		}

		const auto& api = nt::get_api();
		if (api.allocate_virtual_memory) {
			PVOID base = reinterpret_cast<PVOID>(preferred_address);
			SIZE_T region_sz = size;
			NTSTATUS status = api.allocate_virtual_memory(m_handle, &base, 0, &region_sz, allocation_type, protect);
			if (NT_SUCCESS(status) && base) {
				return reinterpret_cast<uintptr_t>(base);
			}
		}

		LPVOID result_addr = VirtualAllocEx(m_handle, reinterpret_cast<LPVOID>(preferred_address), size, allocation_type, protect);
		if (!result_addr) {
			return std::unexpected(error(error_code::allocation_failed, "VirtualAllocEx failed", GetLastError()));
		}
		return reinterpret_cast<uintptr_t>(result_addr);
	}

	result<uint32_t> process::protect(uintptr_t address, size_t size, uint32_t new_protect) const {
		if (!is_valid()) {
			return std::unexpected(error(error_code::invalid_handle, "Process is not attached"));
		}

		const auto& api = nt::get_api();
		if (api.protect_virtual_memory) {
			PVOID base = reinterpret_cast<PVOID>(address);
			SIZE_T region_sz = size;
			ULONG old_protect = 0;
			NTSTATUS status = api.protect_virtual_memory(m_handle, &base, &region_sz, new_protect, &old_protect);
			if (NT_SUCCESS(status)) {
				return static_cast<uint32_t>(old_protect);
			}
		}

		DWORD old_protect = 0;
		if (!VirtualProtectEx(m_handle, reinterpret_cast<LPVOID>(address), size, new_protect, &old_protect)) {
			return std::unexpected(error(error_code::protect_failed, "VirtualProtectEx failed", GetLastError()));
		}
		return static_cast<uint32_t>(old_protect);
	}

	bool process::free(uintptr_t address, size_t size, uint32_t free_type) const {
		if (!is_valid()) return false;

		const auto& api = nt::get_api();
		if (api.free_virtual_memory) {
			PVOID base = reinterpret_cast<PVOID>(address);
			SIZE_T region_sz = size;
			NTSTATUS status = api.free_virtual_memory(m_handle, &base, &region_sz, free_type);
			if (NT_SUCCESS(status)) {
				return true;
			}
		}

		return VirtualFreeEx(m_handle, reinterpret_cast<LPVOID>(address), size, free_type) != FALSE;
	}

	result<memory_region> process::query_region(uintptr_t address) const {
		if (!is_valid()) {
			return std::unexpected(error(error_code::invalid_handle, "Process is not attached"));
		}

		MEMORY_BASIC_INFORMATION mbi{};
		const auto& api = nt::get_api();
		if (api.query_virtual_memory) {
			SIZE_T ret_len = 0;
			NTSTATUS status = api.query_virtual_memory(m_handle, reinterpret_cast<PVOID>(address), nt::MemoryBasicInformation, &mbi, sizeof(mbi), &ret_len);
			if (!NT_SUCCESS(status)) {
				if (VirtualQueryEx(m_handle, reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi)) == 0) {
					return std::unexpected(error(error_code::region_query_failed, std::format("VirtualQueryEx failed for 0x{:X}", address), GetLastError()));
				}
			}
		} else {
			if (VirtualQueryEx(m_handle, reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi)) == 0) {
				return std::unexpected(error(error_code::region_query_failed, std::format("VirtualQueryEx failed for 0x{:X}", address), GetLastError()));
			}
		}

		memory_region region{
			.base_address = reinterpret_cast<uintptr_t>(mbi.BaseAddress),
			.allocation_base = reinterpret_cast<uintptr_t>(mbi.AllocationBase),
			.allocation_protect = mbi.AllocationProtect,
			.size = mbi.RegionSize,
			.state = mbi.State,
			.protect = mbi.Protect,
			.type = mbi.Type
		};

		return region;
	}

	std::vector<memory_region> process::enumerate_regions(uintptr_t start_addr, uintptr_t end_addr) const {
		std::vector<memory_region> regions;
		if (!is_valid()) return regions;

		uintptr_t current = start_addr;
		while (current < end_addr) {
			auto reg_res = query_region(current);
			if (!reg_res.has_value() || reg_res->size == 0) {
				break;
			}
			regions.push_back(reg_res.value());
			current = reg_res->base_address + reg_res->size;
		}

		return regions;
	}

	module_info process::get_module(std::string_view module_name) const {
		return module(m_process_id).enumerate(module_name);
	}

	std::vector<module_info> process::get_modules() const {
		return module::enumerate_all(m_process_id);
	}

	result<uintptr_t> process::pattern_scan(std::string_view pattern, std::string_view module_name) const {
		if (!is_valid()) {
			return std::unexpected(error(error_code::invalid_handle, "Process is not attached"));
		}

		uintptr_t scan_start = m_base_address;
		size_t scan_size = 0;

		if (!module_name.empty()) {
			auto mod = module::find(module_name, m_process_id);
			if (!mod.has_value()) {
				return std::unexpected(mod.error());
			}
			scan_start = mod->base;
			scan_size = mod->size;
		} else {
			auto mod = module::find("", m_process_id);
			if (mod.has_value()) {
				scan_start = mod->base;
				scan_size = mod->size;
			}
		}

		if (scan_size > 0) {
			return pattern_scan_region(pattern, scan_start, scan_size);
		}

		auto regions = enumerate_regions();
		for (const auto& reg : regions) {
			if (reg.is_readable() && reg.is_committed()) {
				auto res = pattern_scan_region(pattern, reg.base_address, reg.size);
				if (res.has_value()) {
					return res.value();
				}
			}
		}

		return std::unexpected(error(error_code::pattern_not_found, std::format("Pattern '{}' not found", pattern)));
	}

	result<uintptr_t> process::pattern_scan_region(std::string_view pattern, uintptr_t start_address, size_t region_size) const {
		struct pattern_byte {
			uint8_t value{0};
			bool wildcard{false};
		};

		std::vector<pattern_byte> pattern_bytes;
		std::string pattern_str(pattern);
		std::istringstream stream(pattern_str);
		std::string token;

		while (stream >> token) {
			if (token == "?" || token == "??") {
				pattern_bytes.push_back({0, true});
			} else {
				try {
					uint8_t b = static_cast<uint8_t>(std::stoul(token, nullptr, 16));
					pattern_bytes.push_back({b, false});
				} catch (...) {
					return std::unexpected(error(error_code::invalid_parameter, std::format("Invalid pattern byte '{}'", token)));
				}
			}
		}

		if (pattern_bytes.empty() || region_size < pattern_bytes.size()) {
			return std::unexpected(error(error_code::invalid_parameter, "Invalid pattern or region size"));
		}

		std::vector<uint8_t> buffer(region_size);
		if (!read_raw(start_address, buffer.data(), region_size)) {
			return std::unexpected(error(error_code::read_failed, std::format("Failed to read region 0x{:X} ({} bytes)", start_address, region_size)));
		}

		const size_t max_idx = region_size - pattern_bytes.size();
		for (size_t i = 0; i <= max_idx; ++i) {
			bool found = true;
			for (size_t j = 0; j < pattern_bytes.size(); ++j) {
				if (!pattern_bytes[j].wildcard && buffer[i + j] != pattern_bytes[j].value) {
					found = false;
					break;
				}
			}
			if (found) {
				return start_address + i;
			}
		}

		return std::unexpected(error(error_code::pattern_not_found, "Pattern not found in region"));
	}

	bool memory::attach_to_process(std::string_view process_name) {
		process proc(process_name);
		if (proc.is_valid()) {
			process_id = proc.get_pid();
			base_address = proc.get_base_address();
			process_handle = proc.get_handle();
			return true;
		}
		return false;
	}

	uint32_t memory::find_process_id(std::string_view process_name) {
		return process::find_process_id(process_name);
	}

	uintptr_t memory::find_module(std::string_view module_name) {
		return module(process_id).enumerate(module_name).base;
	}

	bool memory::read_memory(uintptr_t address, void* buffer, size_t size) {
		if (!process_handle) return false;
		SIZE_T read_bytes = 0;
		return ReadProcessMemory(process_handle, reinterpret_cast<LPCVOID>(address), buffer, size, &read_bytes) && (read_bytes == size);
	}

	bool memory::write_memory(uintptr_t address, const void* buffer, size_t size) {
		if (!process_handle) return false;
		SIZE_T written_bytes = 0;
		return WriteProcessMemory(process_handle, reinterpret_cast<LPVOID>(address), buffer, size, &written_bytes) && (written_bytes == size);
	}

	bool memory::protect(uintptr_t address, size_t size, uint32_t new_protect, uint32_t* old_protect) {
		if (!process_handle) return false;
		DWORD old_p = 0;
		BOOL ok = VirtualProtectEx(process_handle, reinterpret_cast<LPVOID>(address), size, new_protect, &old_p);
		if (old_protect) *old_protect = old_p;
		return ok != FALSE;
	}

	void memory::allocate(uintptr_t address, size_t size, uint32_t allocation_type, uint32_t protect) {
		if (!process_handle) return;
		VirtualAllocEx(process_handle, reinterpret_cast<LPVOID>(address), size, allocation_type, protect);
	}

}