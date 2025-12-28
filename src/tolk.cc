#include "tolk.h"

#include "debug.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace fallout {

#ifdef _WIN32

// Tolk function pointer types
typedef void(__stdcall* Tolk_Load_t)();
typedef void(__stdcall* Tolk_Unload_t)();
typedef bool(__stdcall* Tolk_Output_t)(const wchar_t* str, bool interrupt);
typedef const wchar_t*(__stdcall* Tolk_DetectScreenReader_t)();
typedef void(__stdcall* Tolk_TrySAPI_t)(bool trySAPI);

// Function pointers
static Tolk_Load_t pTolk_Load = nullptr;
static Tolk_Unload_t pTolk_Unload = nullptr;
static Tolk_Output_t pTolk_Output = nullptr;
static Tolk_DetectScreenReader_t pTolk_DetectScreenReader = nullptr;
static Tolk_TrySAPI_t pTolk_TrySAPI = nullptr;

// DLL handle
static HMODULE gTolkModule = nullptr;
static bool gTolkLoaded = false;

int tolkInit()
{
    // Try to load Tolk.dll from the game directory
    gTolkModule = LoadLibraryA("Tolk.dll");
    if (gTolkModule == nullptr) {
        debugPrint("Tolk: Tolk.dll not found, screen reader support disabled\n");
        return 0; // Not an error - graceful degradation
    }

    // Get function pointers
    pTolk_Load = (Tolk_Load_t)GetProcAddress(gTolkModule, "Tolk_Load");
    pTolk_Unload = (Tolk_Unload_t)GetProcAddress(gTolkModule, "Tolk_Unload");
    pTolk_Output = (Tolk_Output_t)GetProcAddress(gTolkModule, "Tolk_Output");
    pTolk_DetectScreenReader = (Tolk_DetectScreenReader_t)GetProcAddress(gTolkModule, "Tolk_DetectScreenReader");
    pTolk_TrySAPI = (Tolk_TrySAPI_t)GetProcAddress(gTolkModule, "Tolk_TrySAPI");

    if (pTolk_Load == nullptr || pTolk_Unload == nullptr || pTolk_Output == nullptr) {
        debugPrint("Tolk: Failed to get required function pointers\n");
        FreeLibrary(gTolkModule);
        gTolkModule = nullptr;
        return -1;
    }

    // Initialize Tolk
    pTolk_Load();
    gTolkLoaded = true;

    // Enable SAPI as fallback if no screen reader is running
    if (pTolk_TrySAPI != nullptr) {
        pTolk_TrySAPI(true);
    }

    // Log which screen reader was detected
    if (pTolk_DetectScreenReader != nullptr) {
        const wchar_t* screenReader = pTolk_DetectScreenReader();
        if (screenReader != nullptr) {
            debugPrint("Tolk: Detected screen reader: %ls\n", screenReader);
        } else {
            debugPrint("Tolk: No screen reader detected, using SAPI\n");
        }
    }

    debugPrint("Tolk: Initialized successfully\n");
    return 0;
}

void tolkExit()
{
    if (gTolkLoaded && pTolk_Unload != nullptr) {
        pTolk_Unload();
        gTolkLoaded = false;
    }

    if (gTolkModule != nullptr) {
        FreeLibrary(gTolkModule);
        gTolkModule = nullptr;
    }

    pTolk_Load = nullptr;
    pTolk_Unload = nullptr;
    pTolk_Output = nullptr;
    pTolk_DetectScreenReader = nullptr;
    pTolk_TrySAPI = nullptr;
}

void tolkSpeak(const char* text, bool interrupt)
{
    if (!gTolkLoaded || pTolk_Output == nullptr || text == nullptr) {
        return;
    }

    // Convert UTF-8 to wide string
    int wideLen = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
    if (wideLen <= 0) {
        return;
    }

    wchar_t* wideText = new wchar_t[wideLen];
    if (MultiByteToWideChar(CP_UTF8, 0, text, -1, wideText, wideLen) > 0) {
        pTolk_Output(wideText, interrupt);
    }
    delete[] wideText;
}

bool tolkIsActive()
{
    return gTolkLoaded;
}

#else // Non-Windows platforms

int tolkInit()
{
    // Tolk is Windows-only
    return 0;
}

void tolkExit()
{
}

void tolkSpeak(const char* text, bool interrupt)
{
    (void)text;
    (void)interrupt;
}

bool tolkIsActive()
{
    return false;
}

#endif // _WIN32

} // namespace fallout
