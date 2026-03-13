#pragma once

#include "pch.h"
#include "Util.hpp"

namespace SettingsUtil {
    struct Settings {
        bool autoReconnect = true;
        std::vector<std::string> pairedDevices;
        std::string language = "zh";
    };

    inline Settings LoadSettings() {
        Settings settings;
        std::wstring path = Util::GetSettingsPath();
        
        std::ifstream file(path);
        if (file.is_open()) {
            try {
                std::string content((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());
                
                // Simple JSON parsing (basic implementation)
                size_t autoReconnectPos = content.find("\"autoReconnect\":");
                if (autoReconnectPos != std::string::npos) {
                    size_t valuePos = content.find_first_of("truefalse", autoReconnectPos + 16);
                    if (valuePos != std::string::npos) {
                        std::string value = content.substr(valuePos, 4);
                        settings.autoReconnect = (value == "true");
                    }
                }
                
                size_t devicesPos = content.find("\"pairedDevices\":[");
                if (devicesPos != std::string::npos) {
                    size_t endArrayPos = content.find("]", devicesPos);
                    if (endArrayPos != std::string::npos) {
                        std::string devicesStr = content.substr(devicesPos + 16, endArrayPos - (devicesPos + 16));
                        size_t pos = 0;
                        while ((pos = devicesStr.find('"', pos)) != std::string::npos) {
                            size_t endPos = devicesStr.find('"', pos + 1);
                            if (endPos != std::string::npos) {
                                std::string device = devicesStr.substr(pos + 1, endPos - (pos + 1));
                                settings.pairedDevices.push_back(device);
                                pos = endPos + 1;
                            } else {
                                break;
                            }
                        }
                    }
                }
                
                size_t languagePos = content.find("\"language\":");
                if (languagePos != std::string::npos) {
                    size_t valuePos = content.find('"', languagePos + 10);
                    if (valuePos != std::string::npos) {
                        size_t endPos = content.find('"', valuePos + 1);
                        if (endPos != std::string::npos) {
                            settings.language = content.substr(valuePos + 1, endPos - (valuePos + 1));
                        }
                    }
                }
            } catch (...) {
                // Ignore parsing errors
            }
            file.close();
        }
        
        return settings;
    }

    inline void SaveSettings(const Settings& settings) {
        std::wstring path = Util::GetSettingsPath();
        
        std::ofstream file(path);
        if (file.is_open()) {
            file << "{" << std::endl;
            file << "  \"autoReconnect\": " << (settings.autoReconnect ? "true" : "false") << "," << std::endl;
            file << "  \"language\": \"" << settings.language << "\"," << std::endl;
            file << "  \"pairedDevices\": [" << std::endl;
            
            for (size_t i = 0; i < settings.pairedDevices.size(); ++i) {
                file << "    \"" << settings.pairedDevices[i] << "\"";
                if (i < settings.pairedDevices.size() - 1) {
                    file << ",";
                }
                file << std::endl;
            }
            
            file << "  ]" << std::endl;
            file << "}" << std::endl;
            file.close();
        }
    }
}