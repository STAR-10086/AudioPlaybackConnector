#pragma once

#include "pch.h"
#include "resource.h"
#include <windows.h>

namespace TrayIcon {
    class Icon {
    public:
        Icon(HINSTANCE hInstance, HWND hWnd) : m_hInstance(hInstance), m_hWnd(hWnd) {
            Initialize();
        }

        ~Icon() {
            Cleanup();
        }

        void Show(const std::wstring& tip) {
            NOTIFYICONDATAW nid = { sizeof(nid) };
            nid.hWnd = m_hWnd;
            nid.uID = 1;
            nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
            nid.uCallbackMessage = WM_TRAYICON;
            
            // Get executable path
            wchar_t exePath[MAX_PATH];
            GetModuleFileNameW(nullptr, exePath, MAX_PATH);
            
            // Extract directory from executable path
            std::wstring exeDir = exePath;
            size_t lastSlash = exeDir.find_last_of(L"\\/");
            if (lastSlash != std::wstring::npos) {
                exeDir = exeDir.substr(0, lastSlash + 1);
            }
            
            // Construct icon path
            std::wstring iconPath = exeDir + L"AudioPlaybackConnector.ico";
            
            // Load icon from file
            nid.hIcon = (HICON)LoadImageW(
                nullptr,
                iconPath.c_str(),
                IMAGE_ICON,
                0, 0,
                LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED
            );
            
            // If icon loading fails, use default icon
            if (nid.hIcon == nullptr) {
                nid.hIcon = LoadIconW(nullptr, (LPCWSTR)IDI_APPLICATION);
            }
            
            lstrcpyW(nid.szTip, tip.c_str());
            Shell_NotifyIconW(NIM_ADD, &nid);
        }
        
        void UpdateTip(const std::wstring& tip) {
            NOTIFYICONDATAW nid = { sizeof(nid) };
            nid.hWnd = m_hWnd;
            nid.uID = 1;
            nid.uFlags = NIF_TIP;
            lstrcpyW(nid.szTip, tip.c_str());
            Shell_NotifyIconW(NIM_MODIFY, &nid);
        }

        void Hide() {
            NOTIFYICONDATAW nid = { sizeof(nid) };
            nid.hWnd = m_hWnd;
            nid.uID = 1;
            Shell_NotifyIconW(NIM_DELETE, &nid);
        }

        void ShowContextMenu() {
            POINT pt;
            GetCursorPos(&pt);

            HMENU hMenu = CreatePopupMenu();
            if (hMenu) {
                AppendMenuW(hMenu, MF_STRING, IDM_SETTINGS, L"Settings");
                AppendMenuW(hMenu, MF_STRING, IDM_ABOUT, L"About");
                AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
                AppendMenuW(hMenu, MF_STRING, IDM_EXIT, L"Exit");

                SetForegroundWindow(m_hWnd);
                TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, m_hWnd, nullptr);
                DestroyMenu(hMenu);
            }
        }

        void OnLeftButtonDown() {
            if (IsWindowVisible(m_hWnd)) {
                ShowWindow(m_hWnd, SW_HIDE);
            } else {
                ShowWindow(m_hWnd, SW_SHOW);
                SetForegroundWindow(m_hWnd);
            }
        }

        static constexpr UINT WM_TRAYICON = WM_USER + 1;

    private:
        void Initialize() {
            // Initialize tray icon
        }

        void Cleanup() {
            Hide();
        }

        HINSTANCE m_hInstance;
        HWND m_hWnd;
    };
}