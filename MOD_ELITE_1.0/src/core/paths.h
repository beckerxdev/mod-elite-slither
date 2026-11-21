#pragma once
#include <cstdlib>
#include <string>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif
namespace me {
// Pasta de dados do usuário: %APPDATA%\MOD_ELITE (Windows) ou ~/.config/MOD_ELITE.
inline std::string dataDir() {
    std::string base;
#ifdef _WIN32
    const char* a = std::getenv("APPDATA"); base = a ? a : ".";
    std::string d = base + "\\MOD_ELITE"; _mkdir(d.c_str()); return d;
#else
    const char* h = std::getenv("HOME"); base = h ? std::string(h) + "/.config" : ".";
    mkdir(base.c_str(), 0755);
    std::string d = base + "/MOD_ELITE"; mkdir(d.c_str(), 0755); return d;
#endif
}
}  // namespace me
