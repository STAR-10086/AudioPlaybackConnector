#include "pch.h"
#include "resource.h"
#include "BluetoothManager.hpp"
#include "TrayIcon.hpp"
#include "SettingsUtil.hpp"
#include "Util.hpp"

struct AppState {
    std::unique_ptr<BluetoothManager::Manager> bluetoothManager;
    std::unique_ptr<TrayIcon::Icon> trayIcon;
    HWND hHiddenWnd = nullptr;
    SettingsUtil::Settings settings;
};

AppState g_app;

// Language support
std::wstring GetText(const std::string& key) {
    if (g_app.settings.language == "zh") {
        if (key == "no_devices") return L"\u65e0\u53ef\u7528\u8bbe\u5907";
        if (key == "connected") return L" [\u5df2\u8fde\u63a5]";
        if (key == "settings") return L"\u8bbe\u7f6e";
        if (key == "about") return L"\u5173\u4e8e";
        if (key == "exit") return L"\u9000\u51fa";
        if (key == "about_text") return L"AudioPlaybackConnector v1.0\n\n\u84dd\u7259A2DP\u63a5\u6536\u5668\u7ba1\u7406\u5668";
        if (key == "about_title") return L"\u5173\u4e8e";
        if (key == "tray_tip") return L"\u97f3\u9891\u64ad\u653e\u8fde\u63a5\u5668";
        if (key == "language_zh") return L"\u4e2d\u6587";
        if (key == "language_en") return L"English";
        if (key == "auto_reconnect_on") return L"\u81ea\u52a8\u91cd\u8fde: \u5f00\u542f";
        if (key == "auto_reconnect_off") return L"\u81ea\u52a8\u91cd\u8fde: \u5173\u95ed";
    } else {
        if (key == "no_devices") return L"No devices available";
        if (key == "connected") return L" [Connected]";
        if (key == "settings") return L"Settings";
        if (key == "about") return L"About";
        if (key == "exit") return L"Exit";
        if (key == "about_text") return L"AudioPlaybackConnector v1.0\n\nBluetooth A2DP Sink Manager";
        if (key == "about_title") return L"About";
        if (key == "tray_tip") return L"AudioPlaybackConnector";
        if (key == "language_zh") return L"Chinese";
        if (key == "language_en") return L"English";
        if (key == "auto_reconnect_on") return L"Auto Reconnect: On";
        if (key == "auto_reconnect_off") return L"Auto Reconnect: Off";
    }
    return L"";
}

LRESULT CALLBACK HiddenWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CreateHiddenWindow(HINSTANCE hInstance);
void RefreshDeviceList();
void ConnectDevice(int index);
void DisconnectDevice(int index);
void SaveSettings();
void LoadSettings();

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    // Check for single instance
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"AudioPlaybackConnectorMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // Another instance is already running
        CloseHandle(hMutex);
        return 0;
    }

    winrt::init_apartment(winrt::apartment_type::single_threaded);

    LoadSettings();

    g_app.bluetoothManager = std::make_unique<BluetoothManager::Manager>();
    
    CreateHiddenWindow(hInstance);
    
    g_app.trayIcon = std::make_unique<TrayIcon::Icon>(hInstance, g_app.hHiddenWnd);
    g_app.trayIcon->Show(GetText("tray_tip"));

    MSG msg = { };
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    SaveSettings();

    // Release mutex
    CloseHandle(hMutex);

    return static_cast<int>(msg.wParam);
}

void LoadSettings() {
    g_app.settings = SettingsUtil::LoadSettings();
}

void SaveSettings() {
    SettingsUtil::SaveSettings(g_app.settings);
}

void CreateHiddenWindow(HINSTANCE hInstance) {
    const wchar_t CLASS_NAME[] = L"AudioPlaybackConnectorHiddenWindow";

    WNDCLASSW wc = { };
    wc.lpfnWndProc = HiddenWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClassW(&wc);

    g_app.hHiddenWnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"AudioPlaybackConnector Hidden Window",
        0,
        0, 0, 0, 0,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );
}

void RefreshDeviceList() {
    // Refresh device list
    g_app.bluetoothManager->RefreshDevices();
    // The device list will be updated the next time the menu is opened
}

void ConnectDevice(int index) {
    auto devices = g_app.bluetoothManager->GetDevices();
    if (index >= 0 && index < static_cast<int>(devices.size())) {
        g_app.bluetoothManager->ConnectDevice(devices[index].id);
    }
}

void DisconnectDevice(int index) {
    auto devices = g_app.bluetoothManager->GetDevices();
    if (index >= 0 && index < static_cast<int>(devices.size())) {
        g_app.bluetoothManager->DisconnectDevice(devices[index].id);
    }
}

LRESULT CALLBACK HiddenWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        case IDM_SETTINGS:
        {
            HMENU hSettingsMenu = CreatePopupMenu();
            if (hSettingsMenu) {
                AppendMenuW(hSettingsMenu, MF_STRING, IDM_LANGUAGE_ZH, GetText("language_zh").c_str());
                AppendMenuW(hSettingsMenu, MF_STRING, IDM_LANGUAGE_EN, GetText("language_en").c_str());
                AppendMenuW(hSettingsMenu, MF_SEPARATOR, 0, nullptr);
                AppendMenuW(hSettingsMenu, MF_STRING, IDM_AUTO_RECONNECT, (g_app.settings.autoReconnect ? GetText("auto_reconnect_on").c_str() : GetText("auto_reconnect_off").c_str()));
                
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hWnd);
                int result = TrackPopupMenu(hSettingsMenu, TPM_RETURNCMD | TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, nullptr);
                DestroyMenu(hSettingsMenu);
                
                switch (result) {
                case IDM_LANGUAGE_ZH:
                    g_app.settings.language = "zh";
                    SaveSettings();
                    g_app.trayIcon->UpdateTip(GetText("tray_tip"));
                    break;
                case IDM_LANGUAGE_EN:
                    g_app.settings.language = "en";
                    SaveSettings();
                    g_app.trayIcon->UpdateTip(GetText("tray_tip"));
                    break;
                case IDM_AUTO_RECONNECT:
                    g_app.settings.autoReconnect = !g_app.settings.autoReconnect;
                    SaveSettings();
                    break;
                }
            }
            break;
        }
        case IDM_ABOUT:
            MessageBoxW(hWnd, GetText("about_text").c_str(), GetText("about_title").c_str(), MB_OK);
            break;
        default:
            // Handle device connect/disconnect commands
                if (LOWORD(wParam) >= 100 && LOWORD(wParam) < 1000) {
                    int deviceIndex = LOWORD(wParam) - 100;
                    auto devices = g_app.bluetoothManager->GetDevices();
                    if (deviceIndex >= 0 && deviceIndex < static_cast<int>(devices.size())) {
                        if (devices[deviceIndex].isConnected) {
                            DisconnectDevice(deviceIndex);
                        } else {
                            ConnectDevice(deviceIndex);
                        }
                        // Refresh device list to show updated status
                        g_app.bluetoothManager->RefreshDevices();
                    }
                }
            break;
        }
        return 0;

    case TrayIcon::Icon::WM_TRAYICON:
        if (lParam == WM_LBUTTONDOWN || lParam == WM_LBUTTONUP) {
            // Left click to show device list menu
            POINT pt;
            GetCursorPos(&pt);

            HMENU hMenu = CreatePopupMenu();
                    if (hMenu) {
                        // Add device list
                        auto devices = g_app.bluetoothManager->GetDevices();
                        if (devices.empty()) {
                            AppendMenuW(hMenu, MF_STRING | MF_GRAYED, 0, GetText("no_devices").c_str());
                        } else {
                            for (int i = 0; i < static_cast<int>(devices.size()); ++i) {
                                std::wstring displayText = devices[i].name;
                                if (devices[i].isConnected) {
                                    displayText += GetText("connected");
                                }
                                AppendMenuW(hMenu, MF_STRING, 100 + i, displayText.c_str());
                            }
                        }

                        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(hMenu, MF_STRING, IDM_SETTINGS, GetText("settings").c_str());
                        AppendMenuW(hMenu, MF_STRING, IDM_ABOUT, GetText("about").c_str());
                        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
                        AppendMenuW(hMenu, MF_STRING, IDM_EXIT, GetText("exit").c_str());

                SetForegroundWindow(hWnd);
                TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, nullptr);
                DestroyMenu(hMenu);
            }
        } else if (lParam == WM_RBUTTONUP) {
            // Right click to show main menu
            POINT pt;
            GetCursorPos(&pt);

            HMENU hMenu = CreatePopupMenu();
            if (hMenu) {
                AppendMenuW(hMenu, MF_STRING, IDM_SETTINGS, GetText("settings").c_str());
                AppendMenuW(hMenu, MF_STRING, IDM_ABOUT, GetText("about").c_str());
                AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
                AppendMenuW(hMenu, MF_STRING, IDM_EXIT, GetText("exit").c_str());

                SetForegroundWindow(hWnd);
                TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, nullptr);
                DestroyMenu(hMenu);
            }
        }
        return 0;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
}