#ifndef DEMANGLE_H
#define DEMANGLE_H

#include <string>
#include <cxxabi.h> // Required for abi::__cxa_demangle
#include <memory>   // For std::unique_ptr


static inline std::string demangle(const std::string& mangled_name) {
    int status = 0;
    char* demangled = abi::__cxa_demangle(mangled_name.c_str(), nullptr, nullptr, &status);
 
    std::unique_ptr<char, decltype(&std::free)> demangled_ptr(demangled, &std::free);
  
    return (status == 0) ? demangled_ptr.get() : mangled_name;
}

#endif // DEMANGLE_H
