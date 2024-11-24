#include <Windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "time.h"


#ifndef DEBUG
int main();

int WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR     lpCmdLine,
  int       nShowCmd
){
    return main();
}
#endif

const uint64_t FPS = 120;
char* className = NULL;

HBITMAP hBitmap;
HDC memDC; // Persistent memory DC
void* pPixels;
int bitmapWidth = 640;
int bitmapHeight = 480;
double Time = 0.0;

#define CELL_SIZE 40
#define NUMBER_OF_CELLS 1000
#define MAX_ACC 400.0

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

        case WM_SETCURSOR: {
            // Set the cursor to a normal arrow
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return TRUE;
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

        // Set the default cursor to an arrow
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

// Handle Window Messages
bool HandleWindowMessages(MSG* msg) {
    while (PeekMessage(msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg->message == WM_QUIT) return false;
        TranslateMessage(msg);
        DispatchMessage(msg);
    }
    return true;
}

// Initialize Bitmap
void initializeBitmap(HDC hdc) {
    BITMAPINFO bmpInfo = {0};
    bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfo.bmiHeader.biWidth = bitmapWidth;
    bmpInfo.bmiHeader.biHeight = -bitmapHeight; // Negative to use a top-down DIB
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biBitCount = 32; // 32-bit color (RGBA)
    bmpInfo.bmiHeader.biCompression = BI_RGB;

    hBitmap = CreateDIBSection(hdc, &bmpInfo, DIB_RGB_COLORS, &pPixels, NULL, 0);

    if (!hBitmap || !pPixels) {
        MessageBoxA(NULL, "Failed to create DIB section!", "Error", MB_ICONERROR);
        exit(1);
    }

    // Initialize bitmap with some color (e.g., black)
    memset(pPixels, 0, bitmapWidth * bitmapHeight * 4);

    // Create a persistent memory DC
    memDC = CreateCompatibleDC(hdc);
    SelectObject(memDC, hBitmap);
}

// Update Pixels (Example)
void updatePixels() {
    uint32_t* pixels = (uint32_t*)pPixels;

    double offset = Time * 20;

    // for (int y = 0; y < bitmapHeight; ++y) {
    //     for (int x = 0; x < bitmapWidth; ++x) {
    //         // Example: Gradient pattern
    //         uint8_t value = (uint8_t)((x - (int)offset) ^ (y + (int)(offset * 0.3)));
    //         pixels[y * bitmapWidth + x] = value; // RGB

    //         // pixels[y * bitmapWidth + x] &= (uint8_t)((sin((float)y / 5) + cos((float)x / 5)) * 255);
    //     }
    // }
}

void drawRectangle(int64_t x, int64_t y, int64_t width, int64_t height, uint32_t color){
    for (int64_t i = y; i < y+height; i++){
        if (i < 0 || i >= bitmapHeight) continue;
        for(int64_t j = x; j < x+width; j++){
            if (j < 0 || j >= bitmapWidth) continue;

            ((uint32_t*)pPixels)[i * bitmapWidth + j] = color;
        }
    }
}

uint32_t COLOR_FROM_LIGHTNESS(uint8_t lightness){
    return 0xFF000000 | ((uint32_t)lightness) << 8 * 2 | ((uint32_t)lightness) << 8 * 1 | ((uint32_t)lightness) << 8 * 0;
}

uint32_t COLOR_WITH_TINT(uint8_t lightness, uint32_t color) {
    // Extract the RGB components
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    // Apply the lightness tint
    r = (uint8_t)((r * lightness) / 255);
    g = (uint8_t)((g * lightness) / 255);
    b = (uint8_t)((b * lightness) / 255);

    // Reconstruct the color with the tinted values
    return 0xFF000000 | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}



void drawCell(int64_t x, int64_t y, int64_t width, int64_t height, uint32_t tint){


    drawRectangle(x,y,width,height, COLOR_WITH_TINT(0x7F, tint));

    double diffX = (double)width * 0.2;
    double diffY = (double)height * 0.2;

    drawRectangle(x + (int64_t)(diffX * 0.5), y + (int64_t)(diffY * 0.5), width - (int64_t)diffX, height - (int64_t)diffY, COLOR_WITH_TINT(0x70, tint));
    drawRectangle(x + (int64_t)(diffX), y + (int64_t)(diffY), width - (int64_t)(diffX * 2), height - (int64_t)(diffY * 2), COLOR_WITH_TINT(0xAA, tint));
}


typedef enum {
    SHAPE_Z = 0,
    SHAPE_S,
    SHAPE_L_R,
    SHAPE_L,
    SHAPE_T,
    SHAPE_Block,
    SHAPE_I,
    SHAPES_COUNT,
} SHAPE;


double x = 0.0;
double y = 0.0;

const uint32_t colors[] = {
    0xFFFF0000, // red     - Z shape
    0xFF00FF00, // green   - S shape
    0xFF0000FF, // blue    -  |__ shape
    0xFFFF5500, // orange  -  __| shape
    0xFFFF00FF, // purple  -  T shape
    0xFFFFFF00, // yellow  - Block shape
    0xFF00FFFF, // cyan    - I shape
};

void drawShape(double x, double y, SHAPE shape, int rotation) {
    uint32_t color = colors[shape];

    // Normalize the rotation to one of the four states: 0, 1, 2, 3
    rotation = rotation % 4;

    switch (shape) {
        case SHAPE_Z:
            if (rotation == 0 || rotation == 2) { // Horizontal Z
                drawCell(x,                y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE,    y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE,    y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE * 2,y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
            } else { // Vertical Z
                drawCell(x + CELL_SIZE,    y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x,                y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE,    y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
                drawCell(x,                y + CELL_SIZE * 2, CELL_SIZE, CELL_SIZE, color);
            }
            break;
        case SHAPE_S:
            if (rotation == 0 || rotation == 2) { // Horizontal S
                drawCell(x + CELL_SIZE * 2,y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE,    y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE,    y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
                drawCell(x,                y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
            } else { // Vertical S
                drawCell(x,                y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x,                y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE,    y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE,    y + CELL_SIZE * 2, CELL_SIZE, CELL_SIZE, color);
            }
            break;
        case SHAPE_T:
            if (rotation == 0) { // "T" facing up
                drawCell(x,                y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE,    y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE,    y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE * 2,y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
            } else if (rotation == 1) { // "T" facing right
                drawCell(x,                y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x,                y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE,    y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
                drawCell(x,                y + CELL_SIZE * 2, CELL_SIZE, CELL_SIZE, color);
            } else if (rotation == 2) { // "T" facing down
                drawCell(x,                y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE,    y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE * 2,y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE,    y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
            } else { // "T" facing left
                drawCell(x,                y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE,    y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE,    y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE,    y + CELL_SIZE * 2, CELL_SIZE, CELL_SIZE, color);
            }
            break;
        case SHAPE_Block: // A square doesn't rotate
            drawCell(x,                y,            CELL_SIZE, CELL_SIZE, color);
            drawCell(x + CELL_SIZE,    y,            CELL_SIZE, CELL_SIZE, color);
            drawCell(x + CELL_SIZE,    y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
            drawCell(x,                y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
            break;
        case SHAPE_L_R:
            if (rotation == 0) { // "L" facing right
                drawCell(x,                y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x,                y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE,    y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE * 2,y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
            } else if (rotation == 1) { // "L" facing up
                drawCell(x,                y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE,    y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x,                y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
                drawCell(x,                y + CELL_SIZE * 2, CELL_SIZE, CELL_SIZE, color);
            } else if (rotation == 2) { // "L" facing left
                drawCell(x,                y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE,    y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE * 2,y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE * 2,y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
            } else { // "L" facing down
                drawCell(x + CELL_SIZE * 2,y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE * 2,y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE,    y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
                drawCell(x,                y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
            }
            break;
        case SHAPE_L:
            if (rotation == 0) { // "L" facing right
                drawCell(x + CELL_SIZE * 2, y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x,                 y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE,     y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE * 2, y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
            } else if (rotation == 1) { // "L" facing up
                drawCell(x,                 y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x,                 y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
                drawCell(x,                 y + CELL_SIZE * 2, CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE,     y,            CELL_SIZE, CELL_SIZE, color);
            } else if (rotation == 2) { // "L" facing left
                drawCell(x,                 y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE,     y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE * 2, y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x,                 y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
            } else { // "L" facing down
                drawCell(x,                 y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE,     y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE,     y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE,     y + CELL_SIZE * 2, CELL_SIZE, CELL_SIZE, color);
            }
            break;
        case SHAPE_I:
            if (rotation == 0 || rotation == 2) { // Horizontal "I"
                drawCell(x,                y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE,    y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE * 2,y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x + CELL_SIZE * 3,y,            CELL_SIZE, CELL_SIZE, color);
            } else { // Vertical "I"
                drawCell(x,                y,            CELL_SIZE, CELL_SIZE, color);
                drawCell(x,                y + CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
                drawCell(x,                y + CELL_SIZE * 2, CELL_SIZE, CELL_SIZE, color);
                drawCell(x,                y + CELL_SIZE * 3, CELL_SIZE, CELL_SIZE, color);
            }
            break;
        default:
            drawCell(x, y, CELL_SIZE, CELL_SIZE, color);
    }
}

const double gravity = -9.8 * 10.0;

typedef struct{
    double x;
    double y;
    double accx;
    double accy;
    SHAPE shape;
    int rotation;
} Cell;

Cell cells[NUMBER_OF_CELLS] = {0};

void randomizeCell(Cell* cell){
    cell->x = (double)bitmapWidth / 2 + ((double)rand() / RAND_MAX * 2.0 - 1.0 ) * 100;
    cell->y = (double)bitmapHeight / 2 + ((double)rand() / RAND_MAX * 2.0 - 1.0 ) * 100;
    cell->accx = ((double)rand() / RAND_MAX * 2.0 - 1.0 ) * MAX_ACC;
    cell->accy = ((double)rand() / RAND_MAX * 2.0 - 1.0 ) * MAX_ACC * 1.2 * -1.0;

    cell->shape = rand() % SHAPES_COUNT; //colors[rand() % (sizeof(colors)/sizeof(colors[0]))];
    cell->rotation = rand() % 4; //colors[rand() % (sizeof(colors)/sizeof(colors[0]))];
}

void game(double deltaTime){
    drawRectangle(0,0,bitmapWidth, bitmapHeight, 0xFF181818);

    // x += deltaTime*100;
    // y += deltaTime*100*0.6;

    // drawCell(x,y,64,64,0xFFFF0000);

    for(uint64_t i = 0; i < NUMBER_OF_CELLS; i++){
        Cell* cell = cells + i;

        cell->x += cell->accx * deltaTime;        
        cell->y += cell->accy * deltaTime;

        cell->accy -= gravity * deltaTime;
        // drawCell(cell->x - (double)CELL_SIZE / 2,cell->y - (double)CELL_SIZE / 2,CELL_SIZE,CELL_SIZE,cell->color);
        drawShape(cell->x - (double)CELL_SIZE / 2,cell->y - (double)CELL_SIZE / 2,cell->shape, cell->rotation);

        if(cell->x - (double)CELL_SIZE * 3 >= bitmapWidth || cell->x + (double)CELL_SIZE * 3 < 0 || cell->y - (double)CELL_SIZE * 3 >= bitmapHeight || cell->y + (double)CELL_SIZE * 3 < 0){
            randomizeCell(cell);
        }
    }
}

// Main Function
int main() {
    srand(time(NULL));

    HWND window = createWindow("Tetris", bitmapWidth, bitmapHeight);
    if (!window) return 1;

    HDC hdc = GetDC(window);
    initializeBitmap(hdc);
    ReleaseDC(window, hdc);

    MSG msg = {0};

    double deltaTime = 0.0;
    uint64_t frequency;
    LARGE_INTEGER perfCounter, lastPerfCounter;
    QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
    QueryPerformanceCounter(&lastPerfCounter);

    for(uint64_t i = 0; i < NUMBER_OF_CELLS; i++){
        Cell* cell = cells+i;

        randomizeCell(cell);
        cell->x = (double)rand() / RAND_MAX * bitmapWidth;
        cell->y = (double)rand() / RAND_MAX * bitmapHeight;
    }


    while (1) {
        QueryPerformanceCounter(&perfCounter);
        deltaTime = (double)(perfCounter.QuadPart - lastPerfCounter.QuadPart) / frequency;
        lastPerfCounter = perfCounter;

        if(deltaTime > 2.0 / FPS) deltaTime = 2.0 / FPS;

        if (!HandleWindowMessages(&msg)) break;

        game(deltaTime);

        // Trigger immediate redraw
        HDC hdc = GetDC(window);
        BitBlt(hdc, 0, 0, bitmapWidth, bitmapHeight, memDC, 0, 0, SRCCOPY);
        ReleaseDC(window, hdc);

        Time += deltaTime;

        // Cap the frame rate
        double frameTime = 1.0 / FPS;
        if (deltaTime < frameTime) {
            Sleep((DWORD)((frameTime - deltaTime) * 1000));
        }
    }

    DeleteDC(memDC);
    DeleteObject(hBitmap);

    return 0;
}
