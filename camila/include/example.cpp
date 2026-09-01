#include <iostream>
#include <thread>

#include "wrapper/wrapper.hpp"
#include "memory/memory.hpp"

auto main() -> int {

	if (!camila::memory{}.attach_to_process("Notepad.exe")) {
		printf("failed to attach\n");
		return 0;
	}
	printf("attached to process\n");

	printf("process id: %llu\n", static_cast<unsigned long long>(camila::memory{}.process_id));
	printf("process handle: %p\n", camila::memory{}.process_handle);
	printf("base address: %llx\n", static_cast<unsigned long long>(camila::memory{}.base_address));

	camila::memory{}.allocate(0, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	camila::memory{}.protect(0, 0x1000, PAGE_EXECUTE_READWRITE, nullptr);

	std::cin.get();
}
