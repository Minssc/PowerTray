#ifndef API_MEDIAKEY_H
#define API_MEDIAKEY_H

#include <windows.h>
#include <iostream>
#include <chrono>

namespace api {

    class mediakey {
    private:
        const int delay_ms = 250;
        HHOOK hHook = nullptr;
        static mediakey* instance;
        int click_stacks = 0;
        HANDLE hTimer = nullptr;
        bool ignore_pp = false;

        static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
        static VOID CALLBACK TimerProc(PVOID lpParam, BOOLEAN TimerOrWaitFired);

    public:
        mediakey();
        ~mediakey();

        void hook();
        void unhook();
        void sendKey(WORD key);
        void ppPressed();
        void ppTimer();
    };
}

#endif
