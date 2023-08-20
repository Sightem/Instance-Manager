#include <windows.h>
#include <vector>
#include <stdexcept>

float GetDPIScalingFactor() {
    HDC screen = GetDC(NULL);
    float dpiScaling = static_cast<float>(GetDeviceCaps(screen, LOGPIXELSX)) / 96.0f;
    ReleaseDC(NULL, screen);
    return dpiScaling;
}

class MouseController {
public:
    MouseController() {
        ntUserSendInputBytes.resize(30); // Reserve space for 30 bytes

        LPVOID NtUserSendInput_Addr = GetProcAddress(GetModuleHandle("win32u"), "NtUserSendInput");
        if (!NtUserSendInput_Addr) {
            NtUserSendInput_Addr = GetProcAddress(GetModuleHandle("user32"), "NtUserSendInput");
            if (!NtUserSendInput_Addr) {
                NtUserSendInput_Addr = GetProcAddress(GetModuleHandle("user32"), "SendInput");
                if (!NtUserSendInput_Addr) {
                    throw std::runtime_error("Failed to locate the NtUserSendInput or equivalent function.");
                }
            }
        }

        memcpy(ntUserSendInputBytes.data(), NtUserSendInput_Addr, 30);
    }

    BOOLEAN MoveMouse(int x, int y) {
        float dpiScaling = GetDPIScalingFactor();
        x = static_cast<int>(x * dpiScaling);
        y = static_cast<int>(y * dpiScaling);

        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.mouseData = 0;
        input.mi.time = 0;
        input.mi.dx = x * (65536 / GetSystemMetrics(SM_CXVIRTUALSCREEN));
        input.mi.dy = y * (65536 / GetSystemMetrics(SM_CYVIRTUALSCREEN));
        input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_VIRTUALDESK | MOUSEEVENTF_ABSOLUTE;

        return SpoofedSendInput(1, &input, sizeof(input));
    }


    BOOLEAN ClickMouse() {
        INPUT input[2] = {};

        // Left button down
        input[0].type = INPUT_MOUSE;
        input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

        // Left button up
        input[1].type = INPUT_MOUSE;
        input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

        return SpoofedSendInput(2, input, sizeof(INPUT));
    }

private:
    std::vector<BYTE> ntUserSendInputBytes;

    BOOLEAN SpoofedSendInput(UINT cInputs, LPINPUT pInputs, int cbSize) {
        if (ntUserSendInputBytes.size() != 30)
            return FALSE;

        LPVOID NtUserSendInput_Spoof = VirtualAlloc(0, 0x1000, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        if (!NtUserSendInput_Spoof)
            return FALSE;

        memcpy(NtUserSendInput_Spoof, ntUserSendInputBytes.data(), 30);
        NTSTATUS Result = reinterpret_cast<NTSTATUS(NTAPI*)(UINT, LPINPUT, int)>(NtUserSendInput_Spoof)(cInputs, pInputs, cbSize);

        ZeroMemory(NtUserSendInput_Spoof, 0x1000);
        VirtualFree(NtUserSendInput_Spoof, 0, MEM_RELEASE);

        return (Result > 0);
    }
};