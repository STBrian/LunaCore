#pragma once

#include "types.h"
#include <string.h>

namespace Minecraft {
    class Lang {
        public:
        undefined4 ukn1;
        char* name;
        undefined4 ukn2;

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

        static void reloadLang() {
            Lang** list = getLangList()[0];
            Lang* oldCurrent = current;
            if (current != list[0]) {
                changeLang(list[0]);
                changeLang(oldCurrent);
            } else {
                changeLang(list[1]);
                changeLang(oldCurrent);
            }
        }
    };
}