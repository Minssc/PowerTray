#include "app/app.h"

#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <string>

namespace api::popup {

bool IsSystemInLightTheme() {
    DWORD value = 1;
    DWORD dataSize = sizeof(value);
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr, (LPBYTE)&value, &dataSize);
        RegCloseKey(hKey);
    }
    return value != 0;
}

LRESULT CALLBACK PopupSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                   UINT_PTR, DWORD_PTR) {
    if (msg == WM_TIMER && wParam == 1) {
        KillTimer(hwnd, 1);
        DestroyWindow(hwnd);
        return 0;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK PopupWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static std::wstring text;
    switch (msg) {
    case WM_CREATE:
        text = (LPCWSTR)((LPCREATESTRUCT)lParam)->lpCreateParams;
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);

        bool isLight = IsSystemInLightTheme();
        COLORREF bg = isLight ? RGB(245, 245, 245) : RGB(32, 32, 32);
        COLORREF fg = isLight ? RGB(0, 0, 0) : RGB(255, 255, 255);

        HBRUSH bgBrush = CreateSolidBrush(bg);
        FillRect(hdc, &rc, bgBrush);
        DeleteObject(bgBrush);

        UINT dpi = GetDpiForWindow(hwnd);
        int fontHeight = -MulDiv(12, dpi, 72);
        HFONT hFont = CreateFontW(fontHeight, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Segoe UI");
        HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

        SetTextColor(hdc, fg);
        SetBkMode(hdc, TRANSPARENT);
        DrawTextW(hdc, text.c_str(), -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        SelectObject(hdc, oldFont);
        DeleteObject(hFont);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_NCHITTEST:
        return HTTRANSPARENT;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void PositionPopupAtBottomCenter(HWND hwnd, int width, int height) {
    RECT rcScreen;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rcScreen, 0);

    int x = rcScreen.left + (rcScreen.right - rcScreen.left - width) / 2;
    int y = rcScreen.bottom - height - 10;

    SetWindowPos(hwnd, HWND_TOPMOST, x, y, width, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void showPopup(const wchar_t* text, int duration_ms = 3000) {
    HWND parent = app::handle;
    bool ownsParent = false;

    HINSTANCE hInstance = nullptr;

    if (!parent) {
        // Create a dummy invisible parent window to own popup
        hInstance = GetModuleHandleW(nullptr);
        const wchar_t* dummyClass = L"PopupOwner";

        static bool dummyRegistered = false;
        if (!dummyRegistered) {
            WNDCLASSW wc = {};
            wc.lpfnWndProc = DefWindowProcW;
            wc.hInstance = hInstance;
            wc.lpszClassName = dummyClass;
            RegisterClassW(&wc);
            dummyRegistered = true;
        }

        parent = CreateWindowExW(0, dummyClass, L"", WS_OVERLAPPED, 0, 0, 0, 0,
                                 nullptr, nullptr, hInstance, nullptr);
        ownsParent = true;
    } else {
        hInstance = (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE);
    }

    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW;
        wc.lpfnWndProc = PopupWndProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = L"StyledPopup";
        RegisterClassExW(&wc);
        registered = true;
    }

    UINT dpi = GetDpiForWindow(parent);
    int fontHeight = -MulDiv(12, dpi, 72);

    HDC hdc = GetDC(nullptr);
    HFONT font = CreateFontW(fontHeight, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Segoe UI");
    HFONT old = (HFONT)SelectObject(hdc, font);
    RECT rcText = { 0, 0, 0, 0 };
    DrawTextW(hdc, text, -1, &rcText, DT_CALCRECT | DT_SINGLELINE);
    SelectObject(hdc, old);
    DeleteObject(font);
    ReleaseDC(nullptr, hdc);

    int padding = MulDiv(10, dpi, 96);
    int w = rcText.right + padding * 1.75;
    int h = rcText.bottom + padding * 1.25;

    HWND hwnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        L"StyledPopup",
        nullptr,
        WS_POPUP,
        0, 0, w, h,
        parent, nullptr, hInstance, (LPVOID)text);

    if (!hwnd) {
        if (ownsParent && parent)
            DestroyWindow(parent);
        return;
    }

    SetWindowSubclass(hwnd, PopupSubclassProc, 1, 0);
    PositionPopupAtBottomCenter(hwnd, w, h);

    SetTimer(hwnd, 1, duration_ms, nullptr);
    ShowWindow(hwnd, SW_SHOWNOACTIVATE);
    UpdateWindow(hwnd);

    if (ownsParent){
        MSG msg;
        while (IsWindow(hwnd)) {
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            Sleep(100); 
        }
        
        if (parent) {
            DestroyWindow(parent);
        }
        
        ExitProcess(0);
    }
}


} // namespace api::popup
