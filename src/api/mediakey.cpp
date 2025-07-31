#include "mediakey.h"

namespace api {
    mediakey::mediakey() {
        this->hook();
        this->instance = this;
    }

    mediakey::~mediakey() {
        this->unhook();
        this->instance = NULL;
    }

    void mediakey::hook() {
        if (!this->hHook) {
            this->hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
        }
    }

    void mediakey::unhook() {
        if (this->hHook) {
            UnhookWindowsHookEx(hHook);
            this->hHook = NULL;
        }
    }

    void mediakey::sendKey(WORD key){
        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = key;
        SendInput(1, &input, sizeof(input));
        input.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(input));
    }

    void mediakey::ppTimer() {
        switch(this->click_stacks){
            case 1:
                this->ignore_pp = true;
                this->sendKey(VK_MEDIA_PLAY_PAUSE);
                break;
            case 2:
                this->sendKey(VK_MEDIA_NEXT_TRACK);
                break;
            case 3:
                this->sendKey(VK_MEDIA_PREV_TRACK);
                break;
        }
        this->click_stacks = 0;
    }

    void mediakey::ppPressed() {
        this->click_stacks++;

        if (this->timer_id)
            KillTimer(NULL, this->timer_id);
        this->timer_id = SetTimer(NULL, this->timer_id, this->delay_ms, this->TimerProc);
    }

    mediakey* mediakey::instance = nullptr;

    LRESULT CALLBACK mediakey::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
            KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
            switch (p->vkCode) {
                case VK_MEDIA_PLAY_PAUSE:
                    if (instance && !instance->ignore_pp){
                        instance->ppPressed();
                        return 1;
                    }
                    instance->ignore_pp = false;
                    break;
            }
        }
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

    void CALLBACK mediakey::TimerProc(HWND, UINT, UINT_PTR idEvent, DWORD) {
        if (instance)
            instance->ppTimer();
        KillTimer(NULL, idEvent);
    }
}
