#pragma once
// Small persistent storage in HKCU\Software\Lumin (per Windows user, no files on disk):
//  - the saved license key ("Remember me"), encrypted with DPAPI (bound to the Windows user account)
//  - a few UI settings (language, theme, ...)
#include <windows.h>
#include <wincrypt.h>
#include <wchar.h>
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "crypt32.lib")

namespace LuminStore {

inline bool Load(char* key, int cap) {
    key[0] = 0;
    BYTE enc[1024]; DWORD esz = (DWORD)sizeof(enc);
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Lumin", L"SavedKeyEnc", RRF_RT_REG_BINARY, nullptr, enc, &esz) == ERROR_SUCCESS) {
        DATA_BLOB in = { esz, enc }, out = { 0, nullptr };
        if (CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out)) {
            int n = (int)out.cbData < cap - 1 ? (int)out.cbData : cap - 1;
            memcpy(key, out.pbData, (size_t)n); key[n] = 0;
            LocalFree(out.pbData);
            return key[0] != 0;
        }
    }
    // older builds stored the key as plain text: read it once (it is re-saved encrypted on the next login)
    wchar_t w[128] = {};
    DWORD sz = (DWORD)sizeof(w);
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Lumin", L"SavedKey", RRF_RT_REG_SZ, nullptr, w, &sz) != ERROR_SUCCESS)
        return false;
    WideCharToMultiByte(CP_UTF8, 0, w, -1, key, cap, nullptr, nullptr);
    key[cap - 1] = 0;
    return key[0] != 0;
}

inline void Save(const char* key) {
    DATA_BLOB in = { (DWORD)strlen(key), (BYTE*)key }, out = { 0, nullptr };
    if (CryptProtectData(&in, L"Eclipse license", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out)) {
        RegSetKeyValueW(HKEY_CURRENT_USER, L"Software\\Lumin", L"SavedKeyEnc", REG_BINARY, out.pbData, out.cbData);
        LocalFree(out.pbData);
        RegDeleteKeyValueW(HKEY_CURRENT_USER, L"Software\\Lumin", L"SavedKey");
    }
}

inline void Clear() {
    RegDeleteKeyValueW(HKEY_CURRENT_USER, L"Software\\Lumin", L"SavedKeyEnc");
    RegDeleteKeyValueW(HKEY_CURRENT_USER, L"Software\\Lumin", L"SavedKey");
}

inline int LoadInt(const wchar_t* name, int def) {
    DWORD v = 0, sz = (DWORD)sizeof(v);
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Lumin", name, RRF_RT_REG_DWORD, nullptr, &v, &sz) != ERROR_SUCCESS) return def;
    return (int)v;
}

inline void SaveInt(const wchar_t* name, int val) {
    DWORD v = (DWORD)val;
    RegSetKeyValueW(HKEY_CURRENT_USER, L"Software\\Lumin", name, REG_DWORD, &v, (DWORD)sizeof(v));
}

}
