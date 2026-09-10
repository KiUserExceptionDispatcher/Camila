# Camila

<div align="center">

```
  ____                      _ _       
 / ___|__ _ _ __ ___  _   _| (_) __ _ 
| |   / _` | '_ ` _ \| | | | | |/ _` |
| |__| (_| | | | | | | |_| | | | (_| |
 \____\__,_|_| |_| |_|\__,_|_|_|\__,_|
                                      
```

**Modern, Fast, Beginner-Friendly C++23 Native Memory Management Engine**

[![Language](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![Platform](https://img.shields.io/badge/Platform-Windows%20x64%20%7C%20x86-0078D6.svg)](https://microsoft.com)
[![Build](https://img.shields.io/badge/Build-MSVC%20%7C%20MSBuild%20%7C%20CMake-brightgreen.svg)](#building)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

</div>

---

## 🌟 Overview

**Camila** is a modern C++23 memory manipulation and inspection library designed for both high performance and maximum developer ergonomics. It provides low-level NT API capabilities through an intuitive, type-safe, and beginner-friendly interface.

Whether you're developing reverse-engineering tools, memory inspection utilities, or game enhancement software, Camila offers everything needed out of the box with zero boilerplate.

---

## ✨ Features

- **🚀 Beginner-Friendly API**: Fluent, chainable, and type-safe abstractions.
- **🔍 Module Enumeration**: Direct one-liner syntax to query base addresses, sizes, paths, and exports.
- **🛡️ Robust Error Handling**: Modern `std::expected` / `camila::result<T>` return types with descriptive error codes and messages.
- **📊 Memory Region Inspection**: Full querying of page states (`MEM_COMMIT`), protection flags (`PAGE_EXECUTE_READWRITE`), allocation types, and access helpers.
- **⚡ Native NT Backend**: Direct dynamic resolution of `NtReadVirtualMemory`, `NtWriteVirtualMemory`, `NtProtectVirtualMemory`, `NtAllocateVirtualMemory`, `NtFreeVirtualMemory`, and `NtQueryVirtualMemory`.
- **🎯 Signature / Pattern Scanning**: Fast byte pattern scanning with wildcards (`?` and `??`) across specific modules or all readable regions.
- **🛠️ Built-in Utilities**:
  - `camila::logger`: Thread-safe, ANSI-colored console logger with log levels and timestamps.
  - `camila::argparse`: Single-header, expressive command-line argument parser.

---

## 📋 Roadmap & Status

- [x] **Better API (Beginner-friendly usage)**
- [x] **Proper error handling (`std::expected` & rich error types)**
- [x] **Memory-region information retrieval**
- [x] **Module enumeration functionality**
- [x] **External dependencies / utilities (`logger` & `argparse`)**
- [x] **Comprehensive documentation and working example**

---

## 🚀 Quick Start

### 1. Single Master Header
Include the master header to access all features:
```cpp
#include "camila.hpp"
```

### 2. Module Enumeration
```cpp
// One-liner syntax to get base address and size
uintptr_t k32_base = camila::module{}.enumerate("kernel32.dll").base;
size_t    k32_size = camila::module{}.enumerate("kernel32.dll").size;

// Resolve exports directly
auto k32_info = camila::module{}.enumerate("kernel32.dll");
uintptr_t get_proc = k32_info.get_export("GetProcAddress");

// Enumerate all modules in a process
for (const auto& mod : camila::module::enumerate_all()) {
    std::cout << mod.name << " at 0x" << std::hex << mod.base << "\n";
}
```

### 3. Process Attachment & Memory Reading/Writing
```cpp
camila::process proc;

// Attach by name or PID
if (proc.attach("target_process.exe")) {
    // Type-safe templated read with std::expected
    auto health = proc.read<int>(0x12345678);
    if (health.has_value()) {
        std::cout << "Health: " << health.value() << "\n";
    } else {
        std::cout << "Read error: " << health.error().to_formatted_string() << "\n";
    }

    // Direct read with fallback default value
    int mana = proc.read_value<int>(0x1234567C, 0);

    // Type-safe write
    proc.write<int>(0x12345678, 9999);

    // String read / write
    auto name = proc.read_string(0x12345680);
    proc.write_string(0x12345680, "PlayerOne");
}
```

### 4. Memory Region Information
```cpp
auto region = proc.query_region(0x12345678);
if (region.has_value()) {
    std::cout << "Base:       0x" << std::hex << region->base_address << "\n";
    std::cout << "Size:       0x" << region->size << " bytes\n";
    std::cout << "State:      " << region->state_string() << "\n";       // e.g. MEM_COMMIT
    std::cout << "Protection: " << region->protect_string() << "\n";     // e.g. PAGE_READWRITE
    std::cout << "Type:       " << region->type_string() << "\n";        // e.g. MEM_PRIVATE
    std::cout << "Readable:   " << (region->is_readable() ? "Yes" : "No") << "\n";
    std::cout << "Writable:   " << (region->is_writable() ? "Yes" : "No") << "\n";
    std::cout << "Executable: " << (region->is_executable() ? "Yes" : "No") << "\n";
}

// Enumerate all memory regions in target process
auto all_regions = proc.enumerate_regions();
```

### 5. Memory Allocation & Protection
```cpp
// Allocate 4KB of executable and writable memory
auto alloc_result = proc.allocate(4096, PAGE_EXECUTE_READWRITE);
if (alloc_result.has_value()) {
    uintptr_t mem = alloc_result.value();

    // Modify protection
    auto old_protect = proc.protect(mem, 4096, PAGE_EXECUTE_READ);

    // Free memory
    proc.free(mem);
}
```

### 6. Pattern / Signature Scanning
```cpp
// Scan with wildcards (? or ??)
auto match = proc.pattern_scan("48 8B 05 ? ? ? ? 48 85 C0", "target_module.dll");
if (match.has_value()) {
    std::cout << "Found pattern at: 0x" << std::hex << match.value() << "\n";
}
```

### 7. Logging & Argument Parsing
```cpp
// Rich colored logging
camila::logger::info("Initializing application...");
camila::logger::success("Process attached successfully! PID: {}", proc.get_pid());
camila::logger::warn("Low memory warning.");
camila::logger::error("Failed to read memory address 0x{:X}", 0xDEADBEEF);

// Command line argument parser
camila::argument_parser parser("MyApp", "Memory inspection tool");
parser.add_argument("-p", "--process", "Target process name", "");
parser.add_flag("-v", "--verbose", "Enable verbose logging");

if (parser.parse(argc, argv)) {
    std::string target = parser.get("--process");
}
```

---

## 📁 Repository Structure

```
Camila/
├── Canela/
│   ├── include/
│   │   ├── camila.hpp                 # Master library header
│   │   ├── example.cpp                # Full CLI & showcase application
│   │   ├── core/
│   │   │   ├── error.hpp              # Error codes & result<T> types
│   │   │   ├── nt.hpp                 # NT native API dynamic bindings
│   │   │   └── nt.cpp                 # NT table implementation
│   │   ├── module/
│   │   │   ├── module.hpp             # Module enumeration & export lookup
│   │   │   └── module.cpp             # Module implementation
│   │   ├── memory/
│   │   │   ├── memory_region.hpp      # Memory region information & helpers
│   │   │   ├── memory_region.cpp      # Memory region string formatters
│   │   │   ├── memory.hpp             # Process & memory management API
│   │   │   └── memory.cpp             # Process & memory implementation
│   │   └── utils/
│   │       ├── logger.hpp             # Colored console logger
│   │       └── argparse.hpp           # CLI argument parser
│   ├── Canela.vcxproj                 # Visual Studio 2022 / 2026 project
│   └── Canela.vcxproj.filters         # Solution filters
├── camila.sln                         # Visual Studio solution
├── CMakeLists.txt                     # Modern CMake build configuration
├── data.json                          # Project metadata
└── README.md                          # Documentation
```

---

## 🔨 Building

### Requirements
- Windows 10/11
- C++23 compatible compiler (Visual Studio 2022 / 2026, MSVC v143 / v144 / v145, or Clang-cl)
- CMake 3.20+ *(optional)*

### Option A: Using Visual Studio / MSBuild
```bash
# Build Release x64
msbuild camila.sln /p:Configuration=Release /p:Platform=x64
```

### Option B: Using CMake
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

---

## 📄 License
This project is open-source and licensed under the MIT License.

# Note
- I just accepted a pull-request, feel free to add new things to Camila.
