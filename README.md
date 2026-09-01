# Camila
Camila is a modern C++ memory management library which provides a set of tools to simplify memory allocation, deallocation, and much more.

# Current:
- Templated r/w
- NT-based backend through table struct
- Remote allocation
- Module/process lookup

# Next update:
- Better api (More beginner-friendly usage)
- Documentation on everything and how it looks.
- Proper error handling
- Memory-region information
- Module enumeration : e.g: 
```cpp
   camila::module{}.enumerate("kernel32.dll").base;
   // or
   camila::module{}.enumerate("kernel32.dll").size; // the module struct would contain base, size, name, etc.
```

# Current todo:
- [ ] Implement proper error handling for all functions
- [ ] Implement memory-region information retrieval
- [ ] Implement module enumeration functionality
- [ ] Implement better API for beginner-friendly usage
- [ ] Implement comprehensive documentation for all features and usage examples
- [ ] Implement external dependencies such as argparse and a logging library for better usability and debugging
