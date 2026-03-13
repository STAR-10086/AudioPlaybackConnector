#pragma once

#include "pch.h"

namespace Util {
    inline std::wstring Utf8ToUtf16(const std::string& utf8) {
        int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
        std::wstring utf16(size - 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &utf16[0], size);
        return utf16;
    }

    inline std::string Utf16ToUtf8(const std::wstring& utf16) {
        int size = WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string utf8(size - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), -1, &utf8[0], size, nullptr, nullptr);
        return utf8;
    }

    inline std::wstring GetExePath() {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        std::wstring exePath(path);
        return exePath.substr(0, exePath.find_last_of(L'\\'));
    }

    inline std::wstring GetSettingsPath() {
        return GetExePath() + L"\\settings.json";
    }

    inline bool IsWindows11OrGreater() {
        OSVERSIONINFOEX osvi;
        ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        osvi.dwMajorVersion = 10;
        osvi.dwMinorVersion = 0;
        osvi.dwBuildNumber = 22000;

        DWORDLONG cond = VerSetConditionMask(
            VerSetConditionMask(
                VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL),
                VER_MINORVERSION, VER_GREATER_EQUAL),
            VER_BUILDNUMBER, VER_GREATER_EQUAL);

        return VerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER, cond);
    }

    inline void SetWindowMicaEffect(HWND hWnd) {
        if (IsWindows11OrGreater()) {
            DWM_SYSTEMBACKDROP_TYPE type = DWMSBT_MAINWINDOW;
            DwmSetWindowAttribute(hWnd, DWMWA_SYSTEMBACKDROP_TYPE, &type, sizeof(type));
        }
    }
}