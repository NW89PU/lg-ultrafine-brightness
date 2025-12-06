#include <Windows.h>
#include "app.h"

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
    // Prevent multiple instances
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"LGUltrafineBrightnessMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // Another instance is running, find it and show its window
        HWND existingWindow = FindWindowW(L"LGUltrafineBrightnessClass", nullptr);
        if (existingWindow) {
            ShowWindow(existingWindow, SW_SHOW);
            SetForegroundWindow(existingWindow);
        }
        return 0;
    }

    // Create and run application
    app::Application application;

    if (!application.initialize(hInstance)) {
        MessageBoxW(nullptr, L"Failed to initialize application", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    int result = application.run();

    application.shutdown();

    if (hMutex) {
        CloseHandle(hMutex);
    }

    return result;
}
