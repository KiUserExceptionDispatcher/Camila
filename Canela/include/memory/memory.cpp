#include "memory.hpp"
HANDLE camila::memory::process_handle = nullptr;
uint64_t camila::memory::process_id = 0;
uint64_t camila::memory::base_address = 0;

bool camila::memory::attach_to_process(std::string_view a1) {
	process_id = find_process_id(a1);
	if (!process_id) {
		return false;
	}

	OBJECT_ATTRIBUTES object_attributes{};
	object_attributes.Length = sizeof(object_attributes);
	//
	CLIENT_ID client_id{};
	client_id.UniqueProcess = (HANDLE)process_id;
	client_id.UniqueThread = nullptr;


	HANDLE handle = nullptr;

	if (!g_table || !g_table->open_process) {
		return false;
	}
	NTSTATUS status = g_table->open_process(&handle, PROCESS_ALL_ACCESS, &object_attributes, &client_id);

	if (status < 0 || !handle) {
		return false;
	}

	process_handle = handle;

	base_address = find_module(a1);
	if (!base_address) {
		return false;
	}

	return true;
}

uint64_t camila::memory::find_process_id(std::string_view a1) {
	PROCESSENTRY32 process_entry{};
	process_entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE) {
		return 0;
	}

	if (Process32First(snapshot, &process_entry)) {
		do {
			if (a1 == process_entry.szExeFile) {
				CloseHandle(snapshot);
				return process_entry.th32ProcessID;
			}
		} while (Process32Next(snapshot, &process_entry));
	}

	CloseHandle(snapshot);
	return 0;
}

uint64_t camila::memory::find_module(std::string_view a1) {
	MODULEENTRY32 module_entry{};
	module_entry.dwSize = sizeof(MODULEENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, process_id);
	if (snapshot == INVALID_HANDLE_VALUE) {
		return 0;
	}

	if (Module32First(snapshot, &module_entry)) {
		do {
			if (a1 == module_entry.szModule) {
				CloseHandle(snapshot);
				return (uint64_t)module_entry.modBaseAddr;
			}
		} while (Module32Next(snapshot, &module_entry));
	}

	CloseHandle(snapshot);
	return 0;
}

bool camila::memory::read_memory(uint64_t a1, void* a2, size_t a3) {
	SIZE_T bytes_read{};
	NTSTATUS status = table{}.read_memory(process_handle, (PVOID)a1, a2, a3, &bytes_read);

	return NT_SUCCESS(status) && bytes_read == a3;
}

bool camila::memory::write_memory(uint64_t a1, void* a2, size_t a3) {
	SIZE_T bytes_written{};
	NTSTATUS status = table{}.write_memory(process_handle, (PVOID)a1, a2, a3, &bytes_written);

	return NT_SUCCESS(status) && bytes_written == a3;
}

template<typename t>
t camila::memory::read(uint64_t a1) {
	t buffer{};
	read_memory(a1, &buffer, sizeof(t));

	return buffer;
}

template<typename t>
t camila::memory::write(uint64_t a1, t a2) {
	write_memory(a1, &a2, sizeof(t));
	return a2;
}

void camila::memory::allocate(uint64_t a1, size_t a2, uint32_t a3, uint32_t a4) {
	table{}.allocate_memory(process_handle, (PVOID*)&a1, 0, (PSIZE_T)&a2, a3, a4);
}