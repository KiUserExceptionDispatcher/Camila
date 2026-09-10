#include "camila.hpp"
#include <iostream>
#include <iomanip>

int main(int argc, char* argv[]) {
	camila::argument_parser parser("Camila Demo & CLI", "Modern C++23 Memory Management Library");
	parser.add_argument("-p", "--process", "Target process name to attach (e.g. notepad.exe)", "");
	parser.add_argument("-i", "--pid", "Target process ID to attach", "0");
	parser.add_argument("-m", "--module", "Find module info by name (e.g. ntdll.dll)", "");
	parser.add_flag("-l", "--list-modules", "Enumerate all loaded modules");
	parser.add_flag("-r", "--list-regions", "Enumerate memory regions");
	parser.add_flag("-v", "--verbose", "Enable trace/debug logging");

	if (!parser.parse(argc, argv)) {
		return 0;
	}

	if (parser.is_set("--verbose")) {
		camila::logger::set_level(camila::log_level::trace);
		camila::logger::debug("Verbose / trace logging enabled.");
	}

	camila::logger::info("=== Camila v2.0 - Memory Management Library ===");

	camila::logger::info("--- Module Enumeration Demo ---");
	{
		uintptr_t k32_base = camila::module{}.enumerate("kernel32.dll").base;
		size_t k32_size = camila::module{}.enumerate("kernel32.dll").size;
		camila::logger::success("camila::module{{}}.enumerate(\"kernel32.dll\").base = 0x{:X}", k32_base);
		camila::logger::success("camila::module{{}}.enumerate(\"kernel32.dll\").size = {} bytes (0x{:X})", k32_size, k32_size);

		auto k32_info = camila::module{}.enumerate("kernel32.dll");
		uintptr_t get_proc_addr = k32_info.get_export("GetProcAddress");
		camila::logger::info("kernel32.dll!GetProcAddress = 0x{:X}", get_proc_addr);
	}

	camila::process proc;
	std::string target_proc = parser.get("--process");
	uint32_t target_pid = parser.get_as<uint32_t>("--pid", 0);

	if (!target_proc.empty()) {
		camila::logger::info("Attaching to process by name: '{}'...", target_proc);
		if (!proc.attach(target_proc)) {
			camila::logger::error("Failed to attach to process '{}'. Ensure it is running with appropriate permissions.", target_proc);
			return 1;
		}
		camila::logger::success("Successfully attached to '{}' (PID: {})", proc.get_name(), proc.get_pid());
	} else if (target_pid != 0) {
		camila::logger::info("Attaching to process by PID: {}...", target_pid);
		if (!proc.attach(target_pid)) {
			camila::logger::error("Failed to attach to PID {}.", target_pid);
			return 1;
		}
		camila::logger::success("Successfully attached to PID {} (Base: 0x{:X})", proc.get_pid(), proc.get_base_address());
	} else {
		camila::logger::info("No target specified. Attaching to current process (PID: {}) for demonstration...", GetCurrentProcessId());
		proc.attach(GetCurrentProcessId());
		camila::logger::success("Attached to current process (PID: {})", proc.get_pid());
	}

	if (parser.is_set("--list-modules") || target_proc.empty()) {
		camila::logger::info("--- Loaded Modules in Target Process ---");
		auto modules = proc.get_modules();
		for (const auto& mod : modules) {
			camila::logger::info("  [0x{:016X}] {:<30} (Size: 0x{:08X})", mod.base, mod.name, mod.size);
		}
	}

	camila::logger::info("--- Memory Region Inspection Demo ---");
	{
		uintptr_t test_addr = proc.get_base_address();
		auto reg_res = proc.query_region(test_addr);
		if (reg_res.has_value()) {
			const auto& reg = reg_res.value();
			camila::logger::success("Region at 0x{:X}:", reg.base_address);
			camila::logger::info("  Size:        0x{:X} ({} KB)", reg.size, reg.size / 1024);
			camila::logger::info("  State:       {}", reg.state_string());
			camila::logger::info("  Protect:     {}", reg.protect_string());
			camila::logger::info("  Type:        {}", reg.type_string());
			camila::logger::info("  Readable:    {} | Writable: {} | Executable: {}",
				reg.is_readable() ? "Yes" : "No",
				reg.is_writable() ? "Yes" : "No",
				reg.is_executable() ? "Yes" : "No");
		} else {
			camila::logger::warn("Could not query region: {}", reg_res.error().to_formatted_string());
		}

		if (parser.is_set("--list-regions")) {
			camila::logger::info("--- First 10 Memory Regions ---");
			auto regions = proc.enumerate_regions();
			size_t count = 0;
			for (const auto& reg : regions) {
				if (count++ >= 10) break;
				camila::logger::info("  [0x{:016X} - 0x{:016X}] {:<12} {:<24} (Size: 0x{:X})",
					reg.base_address, reg.base_address + reg.size,
					reg.state_string(), reg.protect_string(), reg.size);
			}
		}
	}

	camila::logger::info("--- Templated Read / Write Demo ---");
	{
		volatile int local_secret = 1337;
		uintptr_t secret_addr = reinterpret_cast<uintptr_t>(&local_secret);

		auto read_res = proc.read<int>(secret_addr);
		if (read_res.has_value()) {
			camila::logger::success("Read int from 0x{:X}: {}", secret_addr, read_res.value());
		}

		int fast_val = proc.read_value<int>(secret_addr, -1);
		camila::logger::info("Fast read_value fallback: {}", fast_val);

		bool write_ok = proc.write<int>(secret_addr, 9999);
		int current_secret = local_secret;
		camila::logger::success("Wrote new value 9999 (success: {}) -> Value is now: {}", write_ok, current_secret);
	}

	camila::logger::info("--- Remote / Local Allocation & Protection Demo ---");
	{
		auto alloc_res = proc.allocate(4096, PAGE_READWRITE);
		if (alloc_res.has_value()) {
			uintptr_t allocated_addr = alloc_res.value();
			camila::logger::success("Allocated 4096 bytes at 0x{:X}", allocated_addr);

			proc.write_string(allocated_addr, "Camila Memory Management Engine");

			auto str_res = proc.read_string(allocated_addr);
			if (str_res.has_value()) {
				camila::logger::info("Read string from allocated buffer: \"{}\"", str_res.value());
			}

			auto prot_res = proc.protect(allocated_addr, 4096, PAGE_EXECUTE_READ);
			if (prot_res.has_value()) {
				camila::logger::success("Changed memory protection to PAGE_EXECUTE_READ (Old protect: 0x{:X})", prot_res.value());
			}

			bool free_ok = proc.free(allocated_addr);
			camila::logger::success("Freed allocated memory at 0x{:X}: {}", allocated_addr, free_ok ? "OK" : "Failed");
		} else {
			camila::logger::error("Allocation failed: {}", alloc_res.error().to_formatted_string());
		}
	}

	camila::logger::success("=== All Camila operations completed successfully! ===");
	return 0;
}
