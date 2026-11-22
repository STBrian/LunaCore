#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "lua_common.h"

namespace Core {
    enum class SettingType {
        Slider,
        Toggle,
        Dropdown,
        Button
    };

    struct SettingItem {
        SettingType type;
        std::string id;
        std::string label;

        // Slider:
        float floatValue = 0.0f;
        float minValue = 0.0f;
        float maxValue = 1.0f;
        float step = 1.0f;

        // Toggle:
        bool boolValue = false;

        // Dropdown:
        std::vector<std::string> options;
        int selectedIndex = 0;

        // Button:
        int luaCallbackRef = LUA_NOREF;
    };

    struct ModSettingsConfig {
        std::string modId;
        std::vector<SettingItem> items;
        int onCloseCallbackRef = LUA_NOREF;

        bool hasSettings() const {
            return !items.empty();
        }

        SettingItem* findItem(const std::string& id) {
            for (auto& item : items) {
                if (item.id == id) return &item;
            }
            return nullptr;
        }
    };

    class IModSettingsScreen {
    public:
        virtual ~IModSettingsScreen() = default;
        virtual const std::string& getModId() const = 0;
        virtual void refreshUI() = 0;
    };

    class ModSettingsManager {
    public:
        static ModSettingsManager& getInstance();

        ModSettingsConfig* getOrCreateConfig(const std::string& modId);
        ModSettingsConfig* getConfig(const std::string& modId);
        bool hasSettings(const std::string& modId);

        void notifyClose(const std::string& modId);
        void triggerButton(int callbackRef);

        void registerActiveScreen(IModSettingsScreen* screen);
        void unregisterActiveScreen(IModSettingsScreen* screen);
        void notifySettingChanged(const std::string& modId);

    private:
        std::unordered_map<std::string, ModSettingsConfig> mConfigs;
        IModSettingsScreen* mActiveScreen = nullptr;
    };
}
