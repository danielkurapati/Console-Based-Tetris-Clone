#pragma once
// Environment variables: settings and secrets are read from the environment at run time,
// they are never typed into a source file.

#include <cstdlib>   // std::getenv
#include <string>

inline std::string getEnv(const char* name, const std::string& fallback = "") {
#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 4996)
#endif
    const char* value = std::getenv(name);
#ifdef _MSC_VER
    #pragma warning(pop)
#endif
    return (value != nullptr && *value != '\0') ? std::string(value) : fallback;
}
