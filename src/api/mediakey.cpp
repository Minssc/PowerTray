#include "mediakey.h"

namespace api {
    mediakey::mediakey() {
        this->hook();
        this->instance = this;
    }

    mediakey::~mediakey() {
        this->unhook();
        this->instance = NULL;
        if (this->hTimer) {
            DeleteTimerQueueTimer(NULL, this->hTimer, NULL);
            this->hTimer = NULL;
        }
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
                this->ignore_pp = false;
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

        if (this->hTimer) {
            DeleteTimerQueueTimer(NULL, this->hTimer, NULL);
            this->hTimer = NULL;
        }

        CreateTimerQueueTimer(
            &this->hTimer,
            NULL,
            TimerProc,
            this,
            this->delay_ms,
            0,
            WT_EXECUTEONLYONCE
        );
    }

    mediakey* mediakey::instance = nullptr;

    LRESULT CALLBACK mediakey::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
            KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
            if (p->vkCode == VK_MEDIA_PLAY_PAUSE && instance){
                if (!instance->ignore_pp){
                    instance->ppPressed();
                    return 1;
                }
            }
        }
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

    VOID CALLBACK mediakey::TimerProc(PVOID lpParam, BOOLEAN) {
        auto* self = static_cast<mediakey*>(lpParam);
        if (self)
            self->ppTimer();
    }

}
