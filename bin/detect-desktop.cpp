#include <windows.h>
#include <iostream>
#include <string>
#include <fstream>

HWINEVENTHOOK g_hHook = nullptr;
bool g_desktopIsForeground = false;
bool g_quiet = false;
bool g_log = false;
std::wofstream g_logFile;

void Log(const std::wstring& msg) {
    if (!g_quiet)
        std::wcout << msg << std::endl;
    if (g_log && g_logFile.is_open())
        g_logFile << msg << std::endl;
}

void CALLBACK ForegroundChanged(HWINEVENTHOOK, DWORD, HWND hwnd, LONG, LONG, DWORD, DWORD) {
    bool nowDesktop = (hwnd == GetShellWindow());
    if (nowDesktop != g_desktopIsForeground) {
        g_desktopIsForeground = nowDesktop;
        std::wstring status = L"*** Desktop is now ";
        status += (g_desktopIsForeground ? L"FOREGROUND (shown)" : L"BACKGROUND (apps shown)");
        Log(status);
    }
}

void ShowHelp() {
    std::wcout << L"Usage: detect-desktop.exe [--quiet] [--log] [--help]\n"
        << L"  --quiet   Suppress console output\n"
        << L"  --log     Log output to detect-desktop.log\n"
        << L"  --help    Show this help message\n";
}

int wmain(int argc, wchar_t* argv[]) {
    // Parse CLI args
    for (int i = 1; i < argc; ++i) {
        std::wstring arg = argv[i];
        if (arg == L"--quiet") {
            g_quiet = true;
        }
        else if (arg == L"--log") {
            g_log = true;
        }
        else if (arg == L"--help") {
            ShowHelp();
            return 0;
        }
    }

    if (g_log) {
        g_logFile.open(L"detect-desktop.log", std::ios::app);
        if (!g_logFile) {
            std::wcerr << L"Error: Failed to open log file.\n";
            return 1;
        }
    }

    HWND shellDesktop = GetShellWindow();
    if (!shellDesktop) {
        std::wcerr << L"Error: could not get shell (desktop) window handle.\n";
        return 1;
    }

    g_desktopIsForeground = (GetForegroundWindow() == shellDesktop);
    Log(L"Initial state: Desktop is " + std::wstring(g_desktopIsForeground ? L"FOREGROUND" : L"BACKGROUND"));
    Log(L"Listening for foreground changes; press Ctrl+C to exit.");

    g_hHook = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
        nullptr,
        ForegroundChanged,
        0, 0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
    );
    if (!g_hHook) {
        std::wcerr << L"Error: SetWinEventHook failed.\n";
        return 1;
    }

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWinEvent(g_hHook);
    if (g_logFile.is_open()) g_logFile.close();

    return 0;
}
