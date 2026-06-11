#pragma once

#include "game/gstd/gstd_string.hpp"

namespace Minecraft {
    class BaseScreen {
        public:
        virtual ~BaseScreen() = default;

        virtual void _init();
        virtual void setSize(u16 width, u16 height);
        
        virtual void reserved1() = 0;
        virtual void reserved2() = 0;
        
        virtual void unknown1();

        virtual void onFocusGained();
        virtual void onFocusLost();
        virtual void terminate();

        virtual void unknown2();
        virtual void unknown3();

        virtual void reserved3() = 0;
        virtual void reserved4() = 0;

        virtual void tick();

        virtual void unknown4();
        virtual void unknown5();
        virtual void unknown6();
        virtual void unknown7();
        virtual void unknown8();
        virtual void unknown9();

        virtual void reserved5() = 0;

        virtual void unknown10();
        virtual void unknown11();
        virtual void unknown12();
        virtual void unknown13();

        virtual void reserved6() = 0;

        virtual void unknown14();
        virtual void unknown15();
        virtual void unknown16();
        virtual void unknown17();
        virtual void unknown18();
        virtual void unknown19();

        virtual void reserved7() = 0;

        virtual void unknown20();

        virtual void reserved8() = 0;
        virtual void reserved9() = 0;

        virtual void unknown21();
        virtual void unknown22();
        virtual void unknown23();
        virtual void unknown24();
        virtual void unknown25();
        virtual void unknown26();
        virtual void unknown27();
        virtual void unknown28();
        virtual void unknown29();
        virtual void unknown30();
        virtual void unknown31();

        virtual void reserved10() = 0;
        virtual void reserved11() = 0;
        virtual void reserved12() = 0;

        virtual void unknown32();
        virtual void unknown33();
        virtual void unknown34();

        virtual void reserved13() = 0;

        virtual void unknown35();

        virtual u32 getWidth();
        virtual u32 getHeight();

        virtual void reserved14() = 0;
        virtual void reserved15() = 0;

        virtual void unknown36();

        virtual gstd::string getScreenName();

        virtual void reserved16() = 0;
        virtual void reserved17() = 0;
        virtual void reserved18() = 0;

        virtual void unknown37();
        virtual void unknown38();

        virtual void _setupScreen();

        virtual void unknown39();
        virtual void unknown40();
        virtual void unknown41();

        virtual void _touchHandler();
    };
}