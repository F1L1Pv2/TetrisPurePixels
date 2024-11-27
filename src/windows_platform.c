#include "platform.h"
#include "Windows.h"
#include <stdbool.h>
#include <stdint.h>

HBITMAP hBitmap;
HDC memDC; // Persistent memory DC
void* pPixels;
int bitmapWidth = 640;
int bitmapHeight = 480;

bool keys[0xFE];
bool old_keys[0xFE];
bool just_pressed_keys[0xFE];
bool just_unpressed_keys[0xFE];

bool mouseKeys[3];
bool old_mouseKeys[3];
bool just_pressed_mouseKeys[3];
bool doublePress_mouseKeys[3];
bool just_unpressed_mouseKeys[3];
int scroll;
char* className = NULL;

int mouse_x;
int mouse_y;

LRESULT window_proc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    switch (Msg) {
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // Render the DIB to the window using the persistent memory DC
            BitBlt(hdc, 0, 0, bitmapWidth, bitmapHeight, memDC, 0, 0, SRCCOPY);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_KEYDOWN:
        {
            keys[wParam] = 1;

            if (old_keys[wParam] == 0) just_pressed_keys[wParam] = 1;
            old_keys[wParam] = 1;

            break;
        }

        case WM_KEYUP:
        {
            keys[wParam] = 0;

            if (old_keys[wParam] == 1) just_unpressed_keys[wParam] = 1;
            old_keys[wParam] = 0;

            break;
        }

        case WM_LBUTTONDOWN:
        {
            mouseKeys[0] = 1;
            if(old_mouseKeys[0] == 0) just_pressed_mouseKeys[0] = 1;
            old_mouseKeys[0] = 1;
            break;
        }

        case WM_LBUTTONUP:
        {
            mouseKeys[0] = 0;
            if(old_mouseKeys[0] == 1) just_unpressed_mouseKeys[0] = 0;
            old_mouseKeys[0] = 0;
            break;
        }

        case WM_LBUTTONDBLCLK:
        {
            doublePress_mouseKeys[0] = 1;
            break;
        }

        case WM_RBUTTONDOWN:
        {
            mouseKeys[2] = 1;
            if(old_mouseKeys[2] == 0) just_pressed_mouseKeys[2] = 1;
            old_mouseKeys[2] = 1;
            break;
        }

        case WM_RBUTTONUP:
        {
            mouseKeys[2] = 0;
            if(old_mouseKeys[2] == 1) just_pressed_mouseKeys[2] = 0;
            old_mouseKeys[2] = 0;
            break;
        }

        case WM_RBUTTONDBLCLK:
        {
            doublePress_mouseKeys[2] = 1;
            break;
        }

        case WM_MBUTTONDOWN:
        {
            mouseKeys[1] = 1;
            if(old_mouseKeys[1] == 0) just_pressed_mouseKeys[1] = 1;
            old_mouseKeys[1] = 1;
            break;
        }

        case WM_MBUTTONUP:
        {
            mouseKeys[1] = 0;
            if(old_mouseKeys[1] == 1) just_pressed_mouseKeys[1] = 0;
            old_mouseKeys[1] = 0;
            break;
        }

        case WM_MBUTTONDBLCLK:
        {
            doublePress_mouseKeys[1] = 1;
            break;
        }

        case WM_MOUSEMOVE:
        {
            mouse_x = LOWORD(lParam);
            mouse_y = HIWORD(lParam);
            break;
        }

        case WM_MOUSEWHEEL:{

            int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);

            scroll = zDelta;

            break;
        }
    }
    return DefWindowProcA(hwnd, Msg, wParam, lParam);
}

HWND createWindow(const char* windowName, uint32_t width, uint32_t height) {
    if (className == NULL) {
        className = "TETRISOUHMCLASS";

        WNDCLASSA class = {0};
        class.hInstance = GetModuleHandleA(NULL);
        class.lpfnWndProc = window_proc;
        class.lpszClassName = className;

        class.hCursor = LoadCursor(NULL, IDC_ARROW);

        RegisterClassA(&class);
    }

    HWND window = CreateWindowA(className, windowName, WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT, width, height,
                                NULL, NULL, GetModuleHandleA(NULL), NULL);

    if (!window) return NULL;

    ShowWindow(window, SW_SHOW);
    return window;
}

bool HandleWindowMessages(MSG* msg) {
    while (PeekMessage(msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg->message == WM_QUIT) return false;
        TranslateMessage(msg);
        DispatchMessage(msg);
    }
    return true;
}

void initializeBitmap(HDC hdc) {
    BITMAPINFO bmpInfo = {0};
    bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfo.bmiHeader.biWidth = bitmapWidth;
    bmpInfo.bmiHeader.biHeight = -bitmapHeight;
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biBitCount = 32;
    bmpInfo.bmiHeader.biCompression = BI_RGB;

    hBitmap = CreateDIBSection(hdc, &bmpInfo, DIB_RGB_COLORS, &pPixels, NULL, 0);

    if (!hBitmap || !pPixels) {
        MessageBoxA(NULL, "Failed to create DIB section!", "Error", MB_ICONERROR);
        exit(1);
    }

    memset(pPixels, 0, bitmapWidth * bitmapHeight * 4);

    memDC = CreateCompatibleDC(hdc);
    SelectObject(memDC, hBitmap);
}

void platform_draw_rectangle(double x, double y, double width, double height, uint32_t color){
    for (int64_t i = (int64_t)y; i < (int64_t)y+(int64_t)height; i++){
        if (i < 0 || i >= bitmapHeight) continue;
        for(int64_t j = (int64_t)x; j < (int64_t)x+(int64_t)width; j++){
            if (j < 0 || j >= bitmapWidth) continue;

            ((uint32_t*)pPixels)[i * bitmapWidth + j] = color;
        }
    }
}

uint64_t frequency;
LARGE_INTEGER perfCounter, lastPerfCounter;
MSG msg = {0};
HWND window;
double deltaTime = 0.0;

void platform_init(){
    window = createWindow("Tetris", bitmapWidth, bitmapHeight);
    if (!window) exit(1);

    HDC hdc = GetDC(window);
    initializeBitmap(hdc);
    ReleaseDC(window, hdc);


    QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
    QueryPerformanceCounter(&lastPerfCounter);
}

bool platform_pre_game_update(){
    QueryPerformanceCounter(&perfCounter);
    deltaTime = (double)(perfCounter.QuadPart - lastPerfCounter.QuadPart) / frequency;
    lastPerfCounter = perfCounter;
    if (!HandleWindowMessages(&msg)) return false;
    return true;
}

void platform_post_game_update(){
    HDC hdc = GetDC(window);
    BitBlt(hdc, 0, 0, bitmapWidth, bitmapHeight, memDC, 0, 0, SRCCOPY);
    ReleaseDC(window, hdc);

    for(int i = 0; i<0xFE; i++){
        just_pressed_keys[i] = false;
        just_unpressed_keys[i] = false;
    }

    for(int i = 0; i<3; i++){
        just_pressed_mouseKeys[i] = false;
        just_unpressed_mouseKeys[i] = false;
        doublePress_mouseKeys[i] = false;
    }
}

double platform_get_deltaTime(){
    return deltaTime;
}
void platform_set_deltaTime(double time){
    deltaTime = time;
}

void platform_cleanup(){
    DeleteDC(memDC);
    DeleteObject(hBitmap);
}

int platform_screen_width(){
    return bitmapWidth;
}

int platform_screen_height(){
    return bitmapHeight;
}

bool platform_move_right(){
    return just_pressed_keys['D'];
}

bool platform_move_left(){
    return just_pressed_keys['A'];
}

bool platform_move_down(){
    return keys['S'];
}

bool platform_move_jump_down(){
    return just_pressed_keys[VK_SPACE];
}

bool platform_rot_right(){
    return just_pressed_keys['E'];
}

bool platform_rot_left(){
    return just_pressed_keys['Q'];
}