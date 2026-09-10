#include "nt.hpp"

namespace camila::nt {

	bool ntdll_manager::init() {
		if (m_ntdll) return true;

		m_ntdll = GetModuleHandleA("ntdll.dll");
		if (!m_ntdll) {
			m_ntdll = LoadLibraryA("ntdll.dll");
		}
		if (!m_ntdll) return false;

		m_table.read_virtual_memory = (nt_read_virtual_memory)GetProcAddress(m_ntdll, "NtReadVirtualMemory");
		m_table.write_virtual_memory = (nt_write_virtual_memory)GetProcAddress(m_ntdll, "NtWriteVirtualMemory");
		m_table.protect_virtual_memory = (nt_protect_virtual_memory)GetProcAddress(m_ntdll, "NtProtectVirtualMemory");
		m_table.open_process = (nt_open_process)GetProcAddress(m_ntdll, "NtOpenProcess");
		m_table.allocate_virtual_memory = (nt_allocate_virtual_memory)GetProcAddress(m_ntdll, "NtAllocateVirtualMemory");
		m_table.free_virtual_memory = (nt_free_virtual_memory)GetProcAddress(m_ntdll, "NtFreeVirtualMemory");
		m_table.query_virtual_memory = (nt_query_virtual_memory)GetProcAddress(m_ntdll, "NtQueryVirtualMemory");

		return m_table.is_initialized();
	}

}
