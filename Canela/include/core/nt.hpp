#pragma once
#include <Windows.h>
#include <winternl.h>
#include <cstdint>

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

namespace camila::nt {

	typedef struct _CLIENT_ID_EX {
		HANDLE UniqueProcess;
		HANDLE UniqueThread;
	} CLIENT_ID_EX, *PCLIENT_ID_EX;

	typedef enum _MEMORY_INFORMATION_CLASS {
		MemoryBasicInformation = 0,
		MemoryWorkingSetInformation = 1,
		MemoryMappedFilenameInformation = 2,
		MemoryRegionInformation = 3,
		MemoryWorkingSetExInformation = 4
	} MEMORY_INFORMATION_CLASS;

	using nt_read_virtual_memory = NTSTATUS(NTAPI*)(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, SIZE_T NumberOfBytesToRead, PSIZE_T NumberOfBytesRead);
	using nt_write_virtual_memory = NTSTATUS(NTAPI*)(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, SIZE_T NumberOfBytesToWrite, PSIZE_T NumberOfBytesWritten);
	using nt_protect_virtual_memory = NTSTATUS(NTAPI*)(HANDLE ProcessHandle, PVOID* BaseAddress, PSIZE_T RegionSize, ULONG NewProtect, PULONG OldProtect);
	using nt_open_process = NTSTATUS(NTAPI*)(PHANDLE ProcessHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PCLIENT_ID_EX ClientId);
	using nt_allocate_virtual_memory = NTSTATUS(NTAPI*)(HANDLE ProcessHandle, PVOID* BaseAddress, ULONG_PTR ZeroBits, PSIZE_T RegionSize, ULONG AllocationType, ULONG Protect);
	using nt_free_virtual_memory = NTSTATUS(NTAPI*)(HANDLE ProcessHandle, PVOID* BaseAddress, PSIZE_T RegionSize, ULONG FreeType);
	using nt_query_virtual_memory = NTSTATUS(NTAPI*)(HANDLE ProcessHandle, PVOID BaseAddress, MEMORY_INFORMATION_CLASS MemoryInformationClass, PVOID MemoryInformation, SIZE_T MemoryInformationLength, PSIZE_T ReturnLength);

	struct api_table {
		nt_read_virtual_memory read_virtual_memory{nullptr};
		nt_write_virtual_memory write_virtual_memory{nullptr};
		nt_protect_virtual_memory protect_virtual_memory{nullptr};
		nt_open_process open_process{nullptr};
		nt_allocate_virtual_memory allocate_virtual_memory{nullptr};
		nt_free_virtual_memory free_virtual_memory{nullptr};
		nt_query_virtual_memory query_virtual_memory{nullptr};

		bool is_initialized() const noexcept {
			return read_virtual_memory != nullptr &&
			       write_virtual_memory != nullptr &&
			       open_process != nullptr;
		}
	};

	class ntdll_manager {
	public:
		static ntdll_manager& instance() {
			static ntdll_manager inst;
			return inst;
		}

		bool init();
		const api_table& get_table() const noexcept { return m_table; }

	private:
		ntdll_manager() { init(); }
		HMODULE m_ntdll{nullptr};
		api_table m_table{};
	};

	inline const api_table& get_api() {
		return ntdll_manager::instance().get_table();
	}

}
