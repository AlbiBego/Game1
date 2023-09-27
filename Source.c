//2023 Albi Bego "albibego01@gmal.com"
//First game project in c
//Helped and mentored by Ryan Ries playlist
//Youtube playlist name: "Making a Video Game From Scratch" 

#pragma warning (push,3)

#include <stdio.h>

//#include<processthreadsapi.h> maybe it was needed for GetProcessHandleCount()

#include <windows.h>

#include <psapi.h>

#include <emmintrin.h>

#pragma warning (pop)

#include <stdint.h>

#include "Main.h"

#pragma comment(lib, "Winmm.lib")

HWND gGameWindow;

BOOL gGameIsRunning;

GAMEBITMAP gBackBuffer;

GAMEPERFDATA gPerformanceData;

HERO gPlayer;

BOOL gWindowHasFocus;

int __stdcall WinMain(HINSTANCE Instance, HINSTANCE PreviousInstance, PSTR CommandLine, int CmdShow)
{
    UNREFERENCED_PARAMETER(Instance);

    UNREFERENCED_PARAMETER(PreviousInstance);

    UNREFERENCED_PARAMETER(CommandLine); 

    UNREFERENCED_PARAMETER(CmdShow);

    MSG Message = { 0 };

    int64_t FrameStart = 0;

    int64_t FrameEnd = 0;

    int64_t ElapsedMicroseconds = 0; //per frame

    int64_t ElapsedMicrosecondsAccumulatorRaw = 0;

    int64_t ElapsedMicrosecondsAccumulatorCooked = 0;

    HMODULE NtDllModuleHandle = NULL;

    FILETIME ProcessCreationTime = { 0 };

    FILETIME ProcessExitTime = { 0 };

    int64_t CurrentUserCPUTime = 0;

    int64_t CurrentKernelCPUTime = 0;

    int64_t PreviousUserCPUTime = 0;

    int64_t PreviousKernelCPUTime = 0;

    HANDLE ProcessHandle = GetCurrentProcess();

    if ((NtDllModuleHandle = GetModuleHandleA("ntdll.dll")) == NULL)
    {
        MessageBoxA(NULL, "Couldn't load ntdll.dll!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    if ((NtQueryTimerResolution = (_NtQueryTimerResolution)GetProcAddress(NtDllModuleHandle, "NtQueryTimerResolution")) == NULL)
    {
        MessageBoxA(NULL, "Couldn't find the NtQueryTimerResolution function in ntdll.dll!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    NtQueryTimerResolution(&gPerformanceData.MinimumTimerResolution, &gPerformanceData.MaximumTimerResolution, &gPerformanceData.CurrentTimerResolution);

    GetSystemInfo(&gPerformanceData.SystemInfo);

    GetSystemTimeAsFileTime(&gPerformanceData.PreviousSystemTime);

    if (GameIsAlreadyRunning() == TRUE)
    {
        MessageBoxA(NULL, "Another instance of this programming is already running!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        
        goto Exit;
    }

    if (timeBeginPeriod(1) == TIMERR_NOCANDO)
    {
        MessageBoxA(NULL, "Failed to set the global timer resolution!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    if (SetPriorityClass(ProcessHandle, HIGH_PRIORITY_CLASS) == 0)
    {
        MessageBoxA(NULL, "Failed to set process priority!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    if (SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST) == 0)
    {
        MessageBoxA(NULL, "Failed to set thread priority!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    if (CreateMainGameWindow() != ERROR_SUCCESS)
    {
        goto Exit;
    }


    QueryPerformanceFrequency((LARGE_INTEGER *) &gPerformanceData.PerfFrequency);

    gPerformanceData.DisplayDebugInfo = TRUE;

    gBackBuffer.BitmapInfo.bmiHeader.biSize = sizeof(gBackBuffer.BitmapInfo.bmiHeader);

    gBackBuffer.BitmapInfo.bmiHeader.biWidth = GAME_RES_WIDTH;

    gBackBuffer.BitmapInfo.bmiHeader.biHeight = GAME_RES_HEIGHT;

    gBackBuffer.BitmapInfo.bmiHeader.biBitCount = GAME_BPP;

    gBackBuffer.BitmapInfo.bmiHeader.biCompression = BI_RGB;

    gBackBuffer.BitmapInfo.bmiHeader.biPlanes = 1;

    gBackBuffer.Memory = VirtualAlloc(NULL, GAME_DRAWING_AREA_MEMORY_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    if (gBackBuffer.Memory == NULL)
    {
        MessageBoxA(NULL, "Failed to allocate memory for drawing surface!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        
        goto Exit;
    }

    memset(gBackBuffer.Memory, 0x7f, GAME_DRAWING_AREA_MEMORY_SIZE);

    if (IniatializeHero() != ERROR_SUCCESS)
    {
        MessageBoxA(NULL, "Failed to iniatialize hero!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    gGameIsRunning = TRUE;

    while (gGameIsRunning == TRUE)
    {
        QueryPerformanceCounter((LARGE_INTEGER*)&FrameStart);

        while (PeekMessageA(&Message, gGameWindow, 0, 0, PM_REMOVE))
        {
            DispatchMessageA(&Message);
        }

        ProcessPlayerInput();

        RenderFrameGraphics();

        QueryPerformanceCounter((LARGE_INTEGER*)&FrameEnd);

        ElapsedMicroseconds = FrameEnd - FrameStart;

        ElapsedMicroseconds *= 1000000;

        ElapsedMicroseconds /= gPerformanceData.PerfFrequency;

        gPerformanceData.TotalFramesRendered ++;

        ElapsedMicrosecondsAccumulatorRaw += ElapsedMicroseconds;

        while (ElapsedMicroseconds < TARGET_MICROSECONDS_PER_FRAME)
        {
            ElapsedMicroseconds = FrameEnd - FrameStart;

            ElapsedMicroseconds *= 1000000;

            ElapsedMicroseconds /= gPerformanceData.PerfFrequency;

            QueryPerformanceCounter((LARGE_INTEGER*)&FrameEnd);

            if (ElapsedMicroseconds < (TARGET_MICROSECONDS_PER_FRAME * 0.75f))
            {
               Sleep(1); //could be anywhere from 1 ms to a full system timer tick? (~15.635 ms)
            }
        }

        ElapsedMicrosecondsAccumulatorCooked += ElapsedMicroseconds;



        if ((gPerformanceData.TotalFramesRendered % CALCULATE_AVG_FPS_EVERY_X_FRAMES) == 0)
        {
            //int64_t AverageMicrosecondsPerFrameRaw = ElapsedMicrosecondsPerFrameAccumulatorRaw / CALCULATE_AVG_FPS_EVERY_X_FRAMES;

            //int64_t AverageMicrosecondsPerFrameCooked = ElapsedMicrosecondsPerFrameAccumulatorCooked / CALCULATE_AVG_FPS_EVERY_X_FRAMES;

            GetSystemTimeAsFileTime(&gPerformanceData.CurrentSystemTime);

            GetProcessTimes(ProcessHandle, &ProcessCreationTime,
            &ProcessExitTime, (FILETIME*) & CurrentKernelCPUTime,
            (FILETIME*) & CurrentUserCPUTime);

            gPerformanceData.CPUPercent = (CurrentKernelCPUTime - PreviousKernelCPUTime) \
            + (CurrentUserCPUTime - PreviousUserCPUTime);

            gPerformanceData.CPUPercent /= (gPerformanceData.CurrentSystemTime - gPerformanceData.PreviousSystemTime);

            gPerformanceData.CPUPercent /= gPerformanceData.SystemInfo.dwNumberOfProcessors;

            gPerformanceData.CPUPercent *= 100;

            GetProcessHandleCount(ProcessHandle, &gPerformanceData.HandleCount);

            K32GetProcessMemoryInfo(ProcessHandle, (PROCESS_MEMORY_COUNTERS*) & gPerformanceData.MemInfo, sizeof(gPerformanceData.MemInfo));

            gPerformanceData.RawFPSAverage = 1.0f / ((ElapsedMicrosecondsAccumulatorRaw / CALCULATE_AVG_FPS_EVERY_X_FRAMES) * 0.000001f);

            gPerformanceData.CookedFPSAverage = 1.0f / ((ElapsedMicrosecondsAccumulatorCooked / CALCULATE_AVG_FPS_EVERY_X_FRAMES) * 0.000001f);

            //no longer necessary since we are printing our debug info directly on our app 

            //char FrameStats[256] = { 0 };

            //_snprintf_s(FrameStats, _countof(FrameStats), _TRUNCATE,
            //    "Avg microseconds/frame raw: %lld\tAvg FPS Cooked: %.01f\tAvg FPS Raw: %.01f\n",
            //    AverageMicrosecondsPerFrameRaw,
            //    gPerformanceData.CookedFPSAverage,
            //    gPerformanceData.RawFPSAverage);

            //OutputDebugStringA(FrameStats);

            ElapsedMicrosecondsAccumulatorRaw = 0;

            ElapsedMicrosecondsAccumulatorCooked = 0;

            PreviousKernelCPUTime = CurrentKernelCPUTime;

            PreviousUserCPUTime = CurrentUserCPUTime;

            gPerformanceData.PreviousSystemTime = gPerformanceData.CurrentSystemTime;
        }
    }

Exit:

    return 0;
}

LRESULT CALLBACK MainWindowProc(_In_ HWND WindowHandle, _In_ UINT Message, _In_ WPARAM WParam, _In_ LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message)
    {
        case WM_CLOSE:
        {
            gGameIsRunning = FALSE;

            PostQuitMessage(0);

            break;
        }

        case WM_ACTIVATE:
        {
            if (WParam == 0)
            {
                //our window has lost focus

                gWindowHasFocus = FALSE;
            }
            else
            {
                //our window has gained focus

                gWindowHasFocus = TRUE;

                ShowCursor(FALSE);
            }

            ShowCursor(FALSE);

            break;
        }

        default:
             Result = DefWindowProcA(WindowHandle, Message, WParam, LParam);
    }
    return (Result);
}

DWORD CreateMainGameWindow(void)
{
    DWORD Result = ERROR_SUCCESS;

    WNDCLASSEXA WindowClass = { 0 };

    WindowClass.cbSize = sizeof(WNDCLASSEXA);

    WindowClass.style = 0;

    WindowClass.lpfnWndProc = MainWindowProc;

    WindowClass.cbClsExtra = 0;

    WindowClass.cbWndExtra = 0;

    WindowClass.hInstance = GetModuleHandleA(NULL);

    WindowClass.hIcon = LoadIconA(NULL, IDI_APPLICATION);

    WindowClass.hIconSm = LoadIconA(NULL, IDI_APPLICATION);

    WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);

    WindowClass.hbrBackground = CreateSolidBrush(RGB(255, 0, 255));

    WindowClass.lpszMenuName = NULL;

    WindowClass.lpszClassName = GAME_NAME "_WINDOWCLASS";

    //SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    if (!RegisterClassEx(&WindowClass))
    {
        Result = GetLastError();

        MessageBoxA(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    gGameWindow = CreateWindowExA(0, WindowClass.lpszClassName, "Game 1", WS_VISIBLE,
    CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, NULL, NULL, GetModuleHandle(NULL), NULL);

    if (gGameWindow == NULL)
    {
        Result = GetLastError();

        MessageBoxA(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    gPerformanceData.MonitorInfo.cbSize = sizeof(MONITORINFO); 

    if (GetMonitorInfoA(MonitorFromWindow(gGameWindow, MONITOR_DEFAULTTOPRIMARY), 
    &gPerformanceData.MonitorInfo) == 0)
    {
        Result = ERROR_MONITOR_NO_DESCRIPTOR;

        goto Exit;
    }

    gPerformanceData.MonitorWidth = gPerformanceData.MonitorInfo.rcMonitor.right - gPerformanceData.MonitorInfo.rcMonitor.left;

    gPerformanceData.MonitorHeight = gPerformanceData.MonitorInfo.rcMonitor.bottom - gPerformanceData.MonitorInfo.rcMonitor.top;

    if (SetWindowLongPtrA(gGameWindow, GWL_STYLE, WS_VISIBLE) == 0)
    {
        Result = GetLastError();

        goto Exit;
    }

    if (SetWindowPos(gGameWindow, HWND_TOP, gPerformanceData.MonitorInfo.rcMonitor.left, gPerformanceData.MonitorInfo.rcMonitor.top,
    gPerformanceData.MonitorWidth, gPerformanceData.MonitorHeight,SWP_NOOWNERZORDER | SWP_FRAMECHANGED) == 0)
    {
        Result = GetLastError();

        goto Exit;
    }



Exit:

    return Result;
}

BOOL GameIsAlreadyRunning(void)
{
    HANDLE Mutex = NULL;
    
    Mutex = CreateMutexA(NULL, FALSE, GAME_NAME "_GameMutex");

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        return TRUE; 
    }
    else
    {
        return FALSE;
    }
}

void ProcessPlayerInput(void)
{
    if (gWindowHasFocus == FALSE)
    {
        return;
    }

    int16_t EscapeKeyIsDown = GetAsyncKeyState(VK_ESCAPE);

    int16_t DebugKeyIsDown = GetAsyncKeyState(VK_F1);

    int16_t LeftKeyIsDown = GetAsyncKeyState(VK_LEFT) | GetAsyncKeyState('A');

    int16_t RightKeyIsDown = GetAsyncKeyState(VK_RIGHT) | GetAsyncKeyState('D');

    int16_t UpKeyIsDown = GetAsyncKeyState(VK_UP) | GetAsyncKeyState('W');

    int16_t DownKeyIsDown = GetAsyncKeyState(VK_DOWN) | GetAsyncKeyState('S');

    static int16_t DebugKeyWasDown;

    static int16_t LeftKeyWasDown;

    static int16_t RightKeyWasDown;

    static int16_t UpKeyWasDown;

    static int16_t DownKeyWasDown;

    if (EscapeKeyIsDown)
    {
        SendMessageA(gGameWindow, WM_CLOSE, 0, 0);
    }

    if (DebugKeyIsDown && !DebugKeyWasDown)
    {
        gPerformanceData.DisplayDebugInfo = !gPerformanceData.DisplayDebugInfo;
    }

    if (RightKeyIsDown)
    {
        if (gPlayer.ScreenPosX < GAME_RES_WIDTH - 16)
        {
            gPlayer.ScreenPosX++;
        }
    }

    if (LeftKeyIsDown)
    {
        if (gPlayer.ScreenPosX > 0)
        {
            gPlayer.ScreenPosX--;
        }
    }

    if (UpKeyIsDown)
    {
        if (gPlayer.ScreenPosY > 0)
        {
            gPlayer.ScreenPosY--;
        }
    }

    if (DownKeyIsDown)
    {
        if(gPlayer.ScreenPosY < GAME_RES_HEIGHT - 16)
        {
            gPlayer.ScreenPosY++;
        }
    }

    DebugKeyWasDown = DebugKeyIsDown;

    LeftKeyWasDown = LeftKeyIsDown;

    RightKeyWasDown = RightKeyIsDown;

    UpKeyWasDown = UpKeyIsDown;

    DownKeyWasDown = DownKeyIsDown;
}

DWORD Load32BppBitmapFromFile(_In_ char* FileName, _Inout_ GAMEBITMAP* GameBitmap)
{
    DWORD Error = ERROR_SUCCESS;

    HANDLE FileHandle = INVALID_HANDLE_VALUE;

    WORD BitmapHeader = 0;

    DWORD PixelDataOffset = 0;

    DWORD NumberOfBytesRead = 2;

    FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        Error = GetLastError();

        goto Exit;
    }

    if (ReadFile(FileHandle, &BitmapHeader, 2, &NumberOfBytesRead, NULL) == 0)
    {
        Error = GetLastError();

        goto Exit;
    }

    if (BitmapHeader != 0x4d42) // BM backwords
    {
        Error = ERROR_FILE_INVALID;

        goto Exit;
    }

    if (SetFilePointer(FileHandle, 0xA, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        Error = GetLastError();

        goto Exit;
    }

    if (ReadFile(FileHandle, &PixelDataOffset, sizeof(DWORD), &NumberOfBytesRead, NULL) == 0)
    {
        Error = GetLastError();

        goto Exit;
    }

    if (SetFilePointer(FileHandle, 0xE, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        Error = GetLastError();

        goto Exit;
    }

    if (ReadFile(FileHandle, &GameBitmap->BitmapInfo.bmiHeader, sizeof(BITMAPINFOHEADER), &NumberOfBytesRead, NULL) == 0)
    {
        Error = GetLastError();

        goto Exit;
    }

    if ((GameBitmap->Memory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, GameBitmap->BitmapInfo.bmiHeader.biSizeImage)) == NULL)
    {
        Error = ERROR_NOT_ENOUGH_MEMORY;

        goto Exit;
    }

    if (SetFilePointer(FileHandle, PixelDataOffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        Error = GetLastError();

        goto Exit;
    }

    if (ReadFile(FileHandle, GameBitmap->Memory, GameBitmap->BitmapInfo.bmiHeader.biSizeImage, &NumberOfBytesRead, NULL) == 0)
    {
        Error = GetLastError();

        goto Exit;
    }

Exit:

    if (FileHandle && (FileHandle != INVALID_HANDLE_VALUE))
    {
        CloseHandle(FileHandle);
    }

    return Error;
}

DWORD IniatializeHero(void)
{
    DWORD Error = ERROR_SUCCESS;

    gPlayer.ScreenPosX = 25;

    gPlayer.ScreenPosY = 25;
    
    Error = Load32BppBitmapFromFile("C:\\Users\\User\\Desktop\\Game1\\Assets\\Hero_Suit0_Down_Standing.bmpx", &gPlayer.Sprite[SUITE_0][FACING_DOWN_0]);

    if (Error != ERROR_SUCCESS)
    {
        MessageBoxA(NULL, "Load32BppBitmapFromFile failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    Error = Load32BppBitmapFromFile("C:\\Users\\User\\Desktop\\Game1\\Assets\\Hero_Suit0_Down_Walk1.bmpx", &gPlayer.Sprite[SUITE_0][FACING_DOWN_1]);

    if (Error != ERROR_SUCCESS)
    {
        MessageBoxA(NULL, "Load32BppBitmapFromFile failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    Error = Load32BppBitmapFromFile("C:\\Users\\User\\Desktop\\Game1\\Assets\\Hero_Suit0_Down_Walk2.bmpx", &gPlayer.Sprite[SUITE_0][FACING_DOWN_2]);

    if (Error != ERROR_SUCCESS)
    {
        MessageBoxA(NULL, "Load32BppBitmapFromFile failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

Exit:
    return Error;
}

void RenderFrameGraphics(void)
{
#ifdef SIMD
    __m128i  QuadPixel = {0x7f, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff};

    ClearScreen(&QuadPixel);
#else
    PIXEL32 Pixel = { 0x7f, 0x00, 0x00, 0xff };

    ClearScreen(&Pixel);
#endif

    //int32_t ScreenX = gPlayer.ScreenPosX;

    //int32_t ScreenY = gPlayer.ScreenPosY;

    //int32_t StartingScreenPixel = ((GAME_RES_HEIGHT * GAME_RES_WIDTH) - GAME_RES_WIDTH) - \
    //(GAME_RES_WIDTH * ScreenY) + ScreenX;

    //for (int x = 0; x < 16; x++)
    //{
    //    for (int y = 0; y < 16; y++)
    //    {
    //        memset((PIXEL32*)gBackBuffer.Memory + (uintptr_t)StartingScreenPixel + x - \
    //        ((uintptr_t)GAME_RES_WIDTH * y), 0xFF, sizeof(PIXEL32));
    //    }
    //}

    Blit32BppBitmapToBuffer(&gPlayer.Sprite[SUITE_0][FACING_DOWN_0], gPlayer.ScreenPosX, gPlayer.ScreenPosY);

    HDC DeviceContext = GetDC(gGameWindow);

    StretchDIBits(DeviceContext, 0, 0, gPerformanceData.MonitorWidth, gPerformanceData.MonitorHeight, 0, 0,
    GAME_RES_WIDTH, GAME_RES_HEIGHT, gBackBuffer.Memory, &gBackBuffer.BitmapInfo, DIB_RGB_COLORS, SRCCOPY);

    if (gPerformanceData.DisplayDebugInfo == TRUE)
    {
        SelectObject(DeviceContext, (HFONT)GetStockObject(ANSI_FIXED_FONT));

        char DebugTextBuffer[64] = { 0 };

        sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "FPS Raw:   %.01f", gPerformanceData.RawFPSAverage);

        TextOutA(DeviceContext, 0, 0, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "FPS Cooked:%.01f", gPerformanceData.CookedFPSAverage);

        TextOutA(DeviceContext, 0, 13, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "Min Timer Res: %.02f", gPerformanceData.MinimumTimerResolution / 10000.0f);

        TextOutA(DeviceContext, 0, 26, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "Max Timer Res: %.02f", gPerformanceData.MaximumTimerResolution / 10000.0f);

        TextOutA(DeviceContext, 0, 39, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "Cur Timer Res: %.02f", gPerformanceData.CurrentTimerResolution / 10000.0f);

        TextOutA(DeviceContext, 0, 52, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "Handles: %lu", gPerformanceData.HandleCount);

        TextOutA(DeviceContext, 0, 65, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "Memory: %llu KB", gPerformanceData.MemInfo.PrivateUsage / 1024);

        TextOutA(DeviceContext, 0, 78, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "CPU:    %.02f%%", gPerformanceData.CPUPercent);

        TextOutA(DeviceContext, 0, 91, DebugTextBuffer, (int)strlen(DebugTextBuffer));

    }
    ReleaseDC(gGameWindow, DeviceContext);
}

#ifdef SIMD
__forceinline void ClearScreen(_In_ __m128i* Color)
{
    for (int x = 0; x < GAME_RES_WIDTH * GAME_RES_HEIGHT; x += 4)
    {
        _mm_store_si128((PIXEL32*)gBackBuffer.Memory + x, *Color);

        //memcpy_s((PIXEL32 *)gBackBuffer.Memory + x,sizeof(PIXEL32), & Pixel, sizeof(PIXEL32));
    }
#else
__forceinline void ClearScreen(_In_ PIXEL32 * Pixel)
{
    for (int x = 0; x < GAME_RES_WIDTH * GAME_RES_HEIGHT; x ++)
    {
        //_mm_store_si128((PIXEL32*)gBackBuffer.Memory + x, Color);

        memcpy_s((PIXEL32 *)gBackBuffer.Memory + x,sizeof(PIXEL32), Pixel, sizeof(PIXEL32));
    }
#endif

}

void Blit32BppBitmapToBuffer(_In_ GAMEBITMAP* GameBitmap, _In_ uint16_t x, _In_ uint16_t y)
{
    int32_t StartingScreenPixel = ((GAME_RES_HEIGHT * GAME_RES_WIDTH) - GAME_RES_WIDTH) - \
        (GAME_RES_WIDTH * y) + x;

    int32_t StartingBitmapPixel = ((GameBitmap->BitmapInfo.bmiHeader.biWidth * \
        GameBitmap->BitmapInfo.bmiHeader.biHeight) - GameBitmap->BitmapInfo.bmiHeader.biWidth);

    int32_t MemoryOffset = 0;

    int32_t BitmapOffset = 0;

    PIXEL32 BitmapPixel = { 0 };

    PIXEL32 BackgroundPixel = { 0 };

    for (int16_t YPixel = 0; YPixel < GameBitmap->BitmapInfo.bmiHeader.biHeight; YPixel++)
    {
        for (int16_t XPixel = 0; XPixel < GameBitmap->BitmapInfo.bmiHeader.biWidth; XPixel++)
        {
            MemoryOffset = StartingScreenPixel + XPixel - (GAME_RES_WIDTH * YPixel);

            BitmapOffset = StartingBitmapPixel + XPixel - (GameBitmap->BitmapInfo.bmiHeader.biWidth * YPixel);

            memcpy_s(&BitmapPixel, sizeof(PIXEL32), (PIXEL32*)GameBitmap->Memory + BitmapOffset, sizeof(PIXEL32));

            if (BitmapPixel.Alpha == 255)
            {
                memcpy_s((PIXEL32*)gBackBuffer.Memory + MemoryOffset, sizeof(PIXEL32), &BitmapPixel, sizeof(PIXEL32));
            }
        }
    }
}