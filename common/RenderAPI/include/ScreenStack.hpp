#pragma once

#include <vector>
#include <memory>
#include <mutex>
#include <3ds/types.h>

#include "BaseScreen.hpp"

class ScreenStack {
    private:
    std::vector<std::unique_ptr<BaseScreen>> mStack;
    std::mutex mStackLock;
    u8 mPendingPop = 0;

    public:
    void pushScreen(std::unique_ptr<BaseScreen>&& screen) {
        std::lock_guard lock(this->mStackLock);
        screen->init();
        this->mStack.push_back(std::move(screen));
    }

    void update() {
        std::lock_guard lock(this->mStackLock);
        if (this->mPendingPop > 0) {
            if (this->mPendingPop > this->mStack.size()) {
                return;
            }
            while (this->mPendingPop > 0) 
            {
                this->mStack.erase(--this->mStack.end());
                this->mPendingPop--;
            }
        }
        for (std::unique_ptr<BaseScreen>& screen : this->mStack) {
            screen->update();
        }
    }

    void drawScreens(ScreenID screen) {
        std::lock_guard lock(this->mStackLock);
        for (u32 i = 0; i < this->mStack.size(); i++) {
            this->mStack[i]->draw(screen);
        }
    }

    void schedulePopScreens(u8 num) {
        this->mPendingPop += num;
    }
};