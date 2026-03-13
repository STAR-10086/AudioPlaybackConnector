#pragma once

#include "pch.h"

// Core Audio API headers
#include <mmdeviceapi.h>
#include <endpointvolume.h>

namespace BluetoothManager {
    struct BluetoothDevice {
        std::wstring id;
        std::wstring name;
        bool isConnected = false;
    };

    class Manager {
    public:
        Manager() {
            Initialize();
        }

        ~Manager() {
            Cleanup();
        }

        std::vector<BluetoothDevice> GetDevices() {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_devices;
        }

        void RefreshDevices() {
            // Run device discovery in a separate thread to avoid blocking UI
            std::thread([this]() {
                DiscoverDevices();
            }).detach();
        }

        void ConnectDevice(const std::wstring& deviceId) {
            // Run connection in a separate thread to avoid blocking UI
            std::thread([this, deviceId]() {
                try {
                    using namespace winrt::Windows::Devices::Enumeration;
                    using namespace winrt::Windows::Media::Audio;

                    auto device = DeviceInformation::CreateFromIdAsync(deviceId).get();
                    if (device) {
                        auto connection = AudioPlaybackConnection::TryCreateFromId(device.Id());
                        if (connection) {
                            // Add to connections map
                            {
                                std::lock_guard<std::mutex> lock(m_mutex);
                                m_connections.insert(std::make_pair(deviceId, connection));
                            }

                            // Set up state changed event
                            connection.StateChanged([this](const auto& sender, const auto&) {
                                if (sender.State() == AudioPlaybackConnectionState::Closed) {
                                    winrt::hstring deviceIdHStr = sender.DeviceId();
                                    std::wstring deviceId(deviceIdHStr.c_str());
                                    {
                                        std::lock_guard<std::mutex> lock(m_mutex);
                                        auto it = m_connections.find(deviceId);
                                        if (it != m_connections.end()) {
                                            m_connections.erase(it);
                                        }
                                        // Update device status
                                        for (auto& device : m_devices) {
                                            if (device.id == deviceId) {
                                                device.isConnected = false;
                                                break;
                                            }
                                        }
                                    }
                                    sender.Close();
                                }
                            });

                            // Start and open connection
                            connection.StartAsync().get();
                            auto result = connection.OpenAsync().get();

                            switch (result.Status()) {
                            case AudioPlaybackConnectionOpenResultStatus::Success:
                                // Update device status
                                {
                                    std::lock_guard<std::mutex> lock(m_mutex);
                                    for (auto& device : m_devices) {
                                        if (device.id == deviceId) {
                                            device.isConnected = true;
                                            break;
                                        }
                                    }
                                }
                                // First mute then unmute to trigger audio device initialization
                                // This ensures the device is properly initialized for audio playback
                                SetDeviceMute(deviceId, true);
                                // Small delay to ensure the mute command is processed
                                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                // Now unmute
                                SetDeviceMute(deviceId, false);
                                // Additional delay to ensure the unmute command is processed
                                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                break;
                            case AudioPlaybackConnectionOpenResultStatus::RequestTimedOut:
                                // Handle timeout
                                break;
                            case AudioPlaybackConnectionOpenResultStatus::DeniedBySystem:
                                // Handle system denial
                                break;
                            case AudioPlaybackConnectionOpenResultStatus::UnknownFailure:
                                // Handle unknown failure
                                winrt::throw_hresult(result.ExtendedError());
                                break;
                            }
                        }
                    }
                } catch (const winrt::hresult_error& ex) {
                    // Log error
                    OutputDebugStringW((std::wstring(L"Connection error: ") + ex.message().c_str() + L"\n").c_str());
                } catch (...) {
                    // Ignore other errors
                }
            }).detach();
        }

        void DisconnectDevice(const std::wstring& deviceId) {
            // Run disconnection in a separate thread to avoid blocking UI
            std::thread([this, deviceId]() {
                try {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    auto it = m_connections.find(deviceId);
                    if (it != m_connections.end()) {
                        it->second.Close();
                        m_connections.erase(it);
                        // Update device status
                        for (auto& device : m_devices) {
                            if (device.id == deviceId) {
                                device.isConnected = false;
                                break;
                            }
                        }
                    }
                } catch (...) {
                    // Ignore errors
                }
            }).detach();
        }

    private:
        void Initialize() {
            DiscoverDevices();
        }

        void Cleanup() {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (auto& connection : m_connections) {
                connection.second.Close();
            }
            m_connections.clear();
        }

        // Helper function to set audio device mute state
        void SetDeviceMute(const std::wstring& deviceId, bool mute) {
            try {
                CoInitialize(nullptr);
                
                IMMDeviceEnumerator* pEnumerator = nullptr;
                HRESULT hr = CoCreateInstance(
                    __uuidof(MMDeviceEnumerator),
                    nullptr,
                    CLSCTX_ALL,
                    __uuidof(IMMDeviceEnumerator),
                    (void**)&pEnumerator
                );
                
                if (SUCCEEDED(hr)) {
                    // Get all audio output devices to find the matching one
                    IMMDeviceCollection* pCollection = nullptr;
                    hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection);
                    
                    if (SUCCEEDED(hr)) {
                        UINT count = 0;
                        pCollection->GetCount(&count);
                        
                        for (UINT i = 0; i < count; i++) {
                            IMMDevice* pDevice = nullptr;
                            hr = pCollection->Item(i, &pDevice);
                            
                            if (SUCCEEDED(hr)) {
                                LPWSTR pDeviceId = nullptr;
                                hr = pDevice->GetId(&pDeviceId);
                                
                                if (SUCCEEDED(hr)) {
                                    // Check if this device matches the Bluetooth device
                                    std::wstring deviceIdStr(pDeviceId);
                                    if (deviceIdStr.find(deviceId) != std::wstring::npos) {
                                        // Found matching device
                                        IAudioEndpointVolume* pEndpointVolume = nullptr;
                                        hr = pDevice->Activate(
                                            __uuidof(IAudioEndpointVolume),
                                            CLSCTX_ALL,
                                            nullptr,
                                            (void**)&pEndpointVolume
                                        );
                                        
                                        if (SUCCEEDED(hr)) {
                                            pEndpointVolume->SetMute(mute, nullptr);
                                            pEndpointVolume->Release();
                                        }
                                    }
                                    CoTaskMemFree(pDeviceId);
                                }
                                pDevice->Release();
                            }
                        }
                        pCollection->Release();
                    }
                    pEnumerator->Release();
                }
                
                CoUninitialize();
            } catch (...) {
                // Ignore errors
            }
        }

        void DiscoverDevices() {
            try {
                using namespace winrt::Windows::Media::Audio;
                
                auto selector = AudioPlaybackConnection::GetDeviceSelector();
                auto devices = winrt::Windows::Devices::Enumeration::DeviceInformation::FindAllAsync(selector).get();
                
                std::lock_guard<std::mutex> lock(m_mutex);
                m_devices.clear();
                
                for (auto&& device : devices) {
                    BluetoothDevice btDevice;
                    btDevice.id = device.Id();
                    btDevice.name = device.Name();
                    // Check if device is already connected
                    std::wstring deviceIdStr(device.Id().c_str());
                    btDevice.isConnected = (m_connections.find(deviceIdStr) != m_connections.end());
                    m_devices.push_back(btDevice);
                }
            } catch (...) {
                // Ignore errors
            }
        }

        std::vector<BluetoothDevice> m_devices;
        std::unordered_map<std::wstring, winrt::Windows::Media::Audio::AudioPlaybackConnection> m_connections;
        std::mutex m_mutex;
    };
}
