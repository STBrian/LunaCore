#pragma once

#include "types.h"
#include <string.h>

namespace Minecraft {
    class Lang {
        public:
        u32 status;             // +0x00 (byte 0: status flag, 2 = loaded)
        char* name;             // +0x04 (std::string buffer)
        void* translationsMap;  // +0x08 (std::unordered_map*)

        static inline Lang*& current = *reinterpret_cast<Lang**>(0x00a3255c + 4);

        static Lang*** getLangList() {
            return reinterpret_cast<Lang***(*)(void)>(0x55fc1c)();
        }

        static Lang* findLang(const char* name) {
            Lang** start = getLangList()[0];
            Lang** end = getLangList()[1];
            while (start < end) {
                Lang* cur = *start;
                if (strcmp(cur->name, name) == 0)
                    return cur;
                start++;
            }
            return nullptr;
        }

        static void changeLang(Lang* nlang) {
            reinterpret_cast<void(*)(Lang*)>(0x55fae0)(nlang);
        }

        static void notifyLanguageChanged() {
            reinterpret_cast<void(*)(void)>(0x55fb5c)();
        }

        static void loadLanguageFile(Lang* lang) {
            reinterpret_cast<void(*)(Lang*, void*)>(0x1d9ee4)(lang, &lang->name);
        }

        static void reloadLang() {
            if (!current)
                return;

            loadLanguageFile(current);
            notifyLanguageChanged();
        }
    };
}