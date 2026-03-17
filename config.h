#pragma once
#include <Windows.h>
#include <string>

namespace Config {
    inline std::string g_DllDir;

    void Init(HMODULE hModule);
    void Save(const std::string& name = "default");
    void Load(const std::string& name = "default");
}