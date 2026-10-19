#pragma once
// "Remember me" storage: the license key is kept in HKCU\Software\Lumin (per Windows user, no files on disk).
// NOTE: this is plain registry storage, fine for a demo. For a real product encrypt it (DPAPI / CryptProtectData).
#include <windows.h>
#include <wchar.h>
#pragma comment(lib, "advapi32.lib")

namespace LuminStore {

inline bool Load(char* key, int cap) {
    wchar_t w[128] = {};
    DWORD sz = (DWORD)sizeof(w);
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Lumin", L"SavedKey", RRF_RT_REG_SZ, nullptr, w, &sz) != ERROR_SUCCESS)
        return false;
    key[0] = 0;
    WideCharToMultiByte(CP_UTF8, 0, w, -1, key, cap, nullptr, nullptr);
    key[cap - 1] = 0;
    return key[0] != 0;
}

inline void Save(const char* key) {
    wchar_t w[128] = {};
    MultiByteToWideChar(CP_UTF8, 0, key, -1, w, 127);
    RegSetKeyValueW(HKEY_CURRENT_USER, L"Software\\Lumin", L"SavedKey", REG_SZ, w, (DWORD)((wcslen(w) + 1) * sizeof(wchar_t)));
}

inline void Clear() {
    RegDeleteKeyValueW(HKEY_CURRENT_USER, L"Software\\Lumin", L"SavedKey");
}

}
