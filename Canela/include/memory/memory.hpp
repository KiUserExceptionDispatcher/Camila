#pragma once
#include <Windows.h>
#include <iostream>
#include <vector>
#include <memory>
#include <tlhelp32.h>
#include <Psapi.h>
#include <winternl.h>
#include "../wrapper/wrapper.hpp"

namespace camila {
	typedef struct _CLIENT_ID { // < didn't have the struct.
		HANDLE UniqueProcess;
		HANDLE UniqueThread;
	} CLIENT_ID, * PCLIENT_ID;

	using nt_read_virtual_memory = NTSTATUS(NTAPI*)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
	using nt_write_virtual_memory = NTSTATUS(NTAPI*)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
	using nt_protect_virtual_memory = NTSTATUS(NTAPI*)(HANDLE, PVOID*, PSIZE_T, ULONG, PULONG);
	using nt_open_process = NTSTATUS(NTAPI*)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PCLIENT_ID);
	using nt_allocate_virtual_memory = NTSTATUS(NTAPI*)(HANDLE, PVOID*, ULONG_PTR, PSIZE_T, ULONG, ULONG);

	struct table {
		static nt_read_virtual_memory read_memory;
		static nt_write_virtual_memory write_memory;
		static nt_protect_virtual_memory protect_memory;
		static nt_open_process open_process;
		static nt_allocate_virtual_memory allocate_memory;
	};

	class ntdll {
	public:
		void init() {
			nt = GetModuleHandleA("ntdll.dll");
			table{}.read_memory = (nt_read_virtual_memory)GetProcAddress(nt, "NtReadVirtualMemory");
			table{}.write_memory = (nt_write_virtual_memory)GetProcAddress(nt, "NtWriteVirtualMemory");
			table{}.protect_memory = (nt_protect_virtual_memory)GetProcAddress(nt, "NtProtectVirtualMemory");
			table{}.open_process = (nt_open_process)GetProcAddress(nt, "NtOpenProcess");
			table{}.allocate_memory = (nt_allocate_virtual_memory)GetProcAddress(nt, "NtAllocateVirtualMemory");
		}

	private:
		HMODULE nt = nullptr;
	};
	inline std::shared_ptr<ntdll> g_ntdll = std::make_shared<ntdll>();

	class memory {
	public:
		bool attach_to_process(std::string_view a1);
		uint64_t find_process_id(std::string_view a1);
		uint64_t find_module(std::string_view a1);
		bool read_memory(uint64_t a1, void* a2, size_t a3);
		bool write_memory(uint64_t a1, void* a2, size_t a3);
	
	public:
		template<typename t>
		t read(uint64_t a1);

		template<typename t>
		t write(uint64_t a1, t a2);

		bool protect(uint64_t a1, size_t a2, uint32_t a3, uint32_t* a4) {
			return table{}.protect_memory(process_handle, (PVOID*)&a1, (PSIZE_T)&a2, a3, (PULONG)a4) >= 0;
		}

		void allocate(uint64_t a1, size_t a2, uint32_t a3, uint32_t a4);

	public:
		static HANDLE process_handle;
		static uint64_t process_id;
		static uint64_t base_address;
	};
}