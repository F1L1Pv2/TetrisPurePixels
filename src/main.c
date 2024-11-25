//  ___________     __         .__        
//  \__    ___/____/  |________|__| ______
//    |    |_/ __ \   __\_  __ \  |/  ___/
//    |    |\  ___/|  |  |  | \/  |\___ \ 
//    |____| \___  >__|  |__|  |__/____  >
//               \/        PURE PIXELS \/ 
//      made by F1L1Pv2


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

int mouse_x;
int mouse_y;

#define CELL_SIZE 25
#define NUMBER_OF_CELLS 1000
#define MAX_ACC 400.0
#define PLAYFIELD_COLS 10
#define PLAYFIELD_ROWS 16
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
uint8_t PLAYFIELD[PLAYFIELD_COLS * PLAYFIELD_ROWS] = {0};

typedef enum {
    SHAPE_NONE = 0,
    SHAPE_Z,
    SHAPE_S,
    SHAPE_L_R,
    SHAPE_L,
    SHAPE_T,
    SHAPE_Block,
    SHAPE_I,
    SHAPES_COUNT,
} SHAPE;


const uint16_t shapeEncoders[] = {

    /*
       
       Shape Format

      0bKJIHGFEDCBA
        
          | |
          | |
         \   /
          \ /

        |A|B|C|
        |D|E|F|J|
        |G|H|I|
          |K|
    
      'E' is origin

    */


    0b00000110011, // - Z     shape - rotated 0   deg
    0b00000011110, // - S     shape - rotated 0   deg
    0b00000111001, // - |__   shape - rotated 0   deg
    0b00000111100, // - __|   shape - rotated 0   deg
    0b00000111010, // - T     shape - rotated 0   deg
    0b00000011011, // - Block shape - rotated 0   deg
    0b01000111000, // - I     shape - rotated 0   deg

    0b00010110100, // - Z     shape - rotated 90  deg
    0b00010011001, // - S     shape - rotated 90  deg
    0b00010010110, // - |__   shape - rotated 90  deg
    0b00110010010, // - __|   shape - rotated 90  deg
    0b00010110010, // - T     shape - rotated 90  deg
    0b00000011011, // - Block shape - rotated 90  deg
    0b10010010010, // - I     shape - rotated 90  deg

    0b00000110011, // - Z     shape - rotated 180 deg
    0b00000011110, // - S     shape - rotated 180 deg
    0b00100111000, // - |__   shape - rotated 180 deg
    0b00001111000, // - __|   shape - rotated 180 deg
    0b00010111000, // - T     shape - rotated 180 deg
    0b00000011011, // - Block shape - rotated 180 deg
    0b01000111000, // - I     shape - rotated 180 deg

    0b00010110100, // - Z     shape - rotated 270 deg
    0b00010011001, // - S     shape - rotated 270 deg
    0b00011010010, // - |__   shape - rotated 270 deg
    0b00010010011, // - __|   shape - rotated 270 deg
    0b00010011010, // - T     shape - rotated 270 deg
    0b00000011011, // - Block shape - rotated 270 deg
    0b10010010010, // - I     shape - rotated 270 deg
};

const uint32_t colors[] = {
    0xFF181818, // background
    0xFFFF0000, // red     - Z shape
    0xFF00FF00, // green   - S shape
    0xFF0000FF, // blue    -  |__ shape
    0xFFFF5500, // orange  -  __| shape
    0xFFFF00FF, // purple  -  T shape
    0xFFFFFF00, // yellow  - Block shape
    0xFF00FFFF, // cyan    - I shape
};

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
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    r = (uint8_t)((r * lightness) / 255);
    g = (uint8_t)((g * lightness) / 255);
    b = (uint8_t)((b * lightness) / 255);

    return 0xFF000000 | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

void drawCell(int64_t x, int64_t y, int64_t width, int64_t height, uint32_t tint){


    drawRectangle(x,y,width,height, COLOR_WITH_TINT(0x7F, tint));

    double diffX = (double)width * 0.2;
    double diffY = (double)height * 0.2;

    drawRectangle(x + (int64_t)(diffX * 0.5), y + (int64_t)(diffY * 0.5), width - (int64_t)diffX, height - (int64_t)diffY, COLOR_WITH_TINT(0x70, tint));
    drawRectangle(x + (int64_t)(diffX), y + (int64_t)(diffY), width - (int64_t)(diffX * 2), height - (int64_t)(diffY * 2), COLOR_WITH_TINT(0xAA, tint));
}

uint32_t multiply_rgb(uint32_t argb, uint8_t multiplier) {
    uint8_t alpha = (argb >> 24) & 0xFF;
    uint8_t red   = (argb >> 16) & 0xFF;
    uint8_t green = (argb >> 8) & 0xFF;
    uint8_t blue  = argb & 0xFF;

    red   = (red * multiplier) / 255;
    green = (green * multiplier) / 255;
    blue  = (blue * multiplier) / 255;

    return (alpha << 24) | (red << 16) | (green << 8) | blue;
}

void drawShape(double x, double y, SHAPE shape, int rotation, uint8_t lightness) {
    uint32_t color = multiply_rgb(colors[shape], lightness);
    rotation = rotation % 4;

    if(shape == 0 || shape >= SHAPES_COUNT) {drawCell(x, y, CELL_SIZE, CELL_SIZE, color); return;}

    uint16_t shapeForm = shapeEncoders[7*rotation + (shape - 1)];

    if((shapeForm & 0b00000000001) != 0) drawCell(x - CELL_SIZE    , y - CELL_SIZE    , CELL_SIZE, CELL_SIZE, color);
    if((shapeForm & 0b00000000010) != 0) drawCell(x                , y - CELL_SIZE    , CELL_SIZE, CELL_SIZE, color);
    if((shapeForm & 0b00000000100) != 0) drawCell(x + CELL_SIZE    , y - CELL_SIZE    , CELL_SIZE, CELL_SIZE, color);

    if((shapeForm & 0b00000001000) != 0) drawCell(x - CELL_SIZE    , y                , CELL_SIZE, CELL_SIZE, color);
    if((shapeForm & 0b00000010000) != 0) drawCell(x                , y                , CELL_SIZE, CELL_SIZE, color);
    if((shapeForm & 0b00000100000) != 0) drawCell(x + CELL_SIZE    , y                , CELL_SIZE, CELL_SIZE, color);

    if((shapeForm & 0b00001000000) != 0) drawCell(x - CELL_SIZE    , y + CELL_SIZE    , CELL_SIZE, CELL_SIZE, color);
    if((shapeForm & 0b00010000000) != 0) drawCell(x                , y + CELL_SIZE    , CELL_SIZE, CELL_SIZE, color);
    if((shapeForm & 0b00100000000) != 0) drawCell(x + CELL_SIZE    , y + CELL_SIZE    , CELL_SIZE, CELL_SIZE, color);

    if((shapeForm & 0b01000000000) != 0) drawCell(x + CELL_SIZE * 2, y                , CELL_SIZE, CELL_SIZE, color);
    if((shapeForm & 0b10000000000) != 0) drawCell(x                , y + CELL_SIZE * 2, CELL_SIZE, CELL_SIZE, color);
}

void randomizeCell(Cell* cell){
    cell->x = (double)bitmapWidth / 2 + ((double)rand() / RAND_MAX * 2.0 - 1.0 ) * 100;
    cell->y = (double)bitmapHeight / 2 + ((double)rand() / RAND_MAX * 2.0 - 1.0 ) * 100;
    cell->accx = ((double)rand() / RAND_MAX * 2.0 - 1.0 ) * MAX_ACC;
    cell->accy = ((double)rand() / RAND_MAX * 2.0 - 1.0 ) * MAX_ACC * 1.2 * -1.0;

    cell->shape = (rand() % (SHAPES_COUNT - 1) ) + 1;
    cell->rotation = rand() % 4;
}

double lightness = 0.8;

int test_x = 0;
int test_y = 0;
SHAPE shape = 1;
int rotation = 0;

float tick = 0.0;

void game_tick(){
    test_y++;
}

void game(double deltaTime){
    drawRectangle(0,0,bitmapWidth, bitmapHeight, 0xFF181818);

    lightness -= deltaTime / 10;
    if(lightness < 0.2) lightness = 0.2;

    if(just_pressed_keys['D']) test_x++;
    if(just_pressed_keys['A']) test_x--;

    if(just_pressed_keys['W']) test_y--;
    if(just_pressed_keys['S']) test_y++;

    if(just_pressed_keys['E']) rotation = (rotation + 1) % 4;
    if(just_pressed_keys['Q']) rotation = (rotation - 1) > 0 ? (rotation - 1) : 4 + (rotation - 1);

    if(just_pressed_keys['1']) shape = (shape + 1) % 7;
    if(just_pressed_keys['2']) shape = (shape - 1) > 0 ? (shape - 1) : 7 + (shape - 1);

    tick += deltaTime * 2;

    if(tick >= 1.0) {
        tick = 0.0;
        game_tick();
    }

    for(uint64_t i = 0; i < NUMBER_OF_CELLS; i++){
        Cell* cell = cells + i;

        cell->x += cell->accx * deltaTime;        
        cell->y += cell->accy * deltaTime;

        cell->accy -= gravity * deltaTime;
        drawShape(cell->x - (double)CELL_SIZE / 2,cell->y - (double)CELL_SIZE / 2,cell->shape, cell->rotation, lightness*255);

        if(cell->x - (double)CELL_SIZE * 3 >= bitmapWidth || cell->x + (double)CELL_SIZE * 3 < 0 || cell->y - (double)CELL_SIZE * 3 >= bitmapHeight || cell->y + (double)CELL_SIZE * 3 < 0){
            randomizeCell(cell);
        }
    }

    drawRectangle(((double)bitmapWidth/2 - (double)(PLAYFIELD_COLS + 1)* CELL_SIZE / 2) - (double)CELL_SIZE / 2,((double)bitmapHeight/2 - (double)(PLAYFIELD_ROWS + 1)* CELL_SIZE / 2) - (double)CELL_SIZE / 2,(double)(PLAYFIELD_COLS + 1)* CELL_SIZE, (double)(PLAYFIELD_ROWS + 1)* CELL_SIZE, 0xFF181818);
    for(uint64_t i = 0; i < PLAYFIELD_ROWS; i++){
        double y = ((double)bitmapHeight / 2 - (double)PLAYFIELD_ROWS * CELL_SIZE / 2 + i * CELL_SIZE) - (double)CELL_SIZE / 2;
        for(uint64_t j = 0; j < PLAYFIELD_COLS; j++){
            double x = ((double)bitmapWidth / 2 - (double)PLAYFIELD_COLS * CELL_SIZE / 2 + j * CELL_SIZE) - (double)CELL_SIZE / 2;

            drawCell(x,y,CELL_SIZE,CELL_SIZE,colors[PLAYFIELD[i*PLAYFIELD_COLS+j]]);
        }
    }

    double offX = ((double)bitmapWidth / 2 - (double)PLAYFIELD_COLS * CELL_SIZE / 2 + test_x * CELL_SIZE) - (double)CELL_SIZE / 2;
    double offY = ((double)bitmapHeight / 2 - (double)PLAYFIELD_ROWS * CELL_SIZE / 2 + test_y * CELL_SIZE) - (double)CELL_SIZE / 2;
    drawShape(offX,offY, shape, rotation, 0xFF);
}

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

    // for(int i =0; i < PLAYFIELD_COLS * PLAYFIELD_ROWS; i++){
    //     PLAYFIELD[i] = rand() % (sizeof(colors)/sizeof(colors[0]));
    // }

    while (1) {
        QueryPerformanceCounter(&perfCounter);
        deltaTime = (double)(perfCounter.QuadPart - lastPerfCounter.QuadPart) / frequency;
        lastPerfCounter = perfCounter;

        if(deltaTime > 2.0 / FPS) deltaTime = 2.0 / FPS;

        if (!HandleWindowMessages(&msg)) break;

        game(deltaTime);

        HDC hdc = GetDC(window);
        BitBlt(hdc, 0, 0, bitmapWidth, bitmapHeight, memDC, 0, 0, SRCCOPY);
        ReleaseDC(window, hdc);

        Time += deltaTime;

        for(int i = 0; i<0xFE; i++){
            just_pressed_keys[i] = false;
            just_unpressed_keys[i] = false;
        }

        for(int i = 0; i<3; i++){
            just_pressed_mouseKeys[i] = false;
            just_unpressed_mouseKeys[i] = false;
            doublePress_mouseKeys[i] = false;
        }

        double frameTime = 1.0 / FPS;
        if (deltaTime < frameTime) {
            Sleep((DWORD)((frameTime - deltaTime) * 1000));
        }
    }

    DeleteDC(memDC);
    DeleteObject(hBitmap);

    return 0;
}
