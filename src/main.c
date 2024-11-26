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
#include <string.h>


#ifndef DEBUG
int main();

int WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR     lpCmdLine,
  int       nShowCmd
){
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nShowCmd;
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
#define DEFAULT_FALL_SPEED 2.0
double fallSpeed = DEFAULT_FALL_SPEED;
bool outOfBounds = false;

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

const uint16_t counterShapes[] = {

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


    0b00111101111,
    0b00010010010,
    0b00110010011,
    0b00111011111,
    0b00110011101,
    0b00011010110,
    0b00111111001,
    0b00001010111,
    0b00111111111,
    0b00100111111,
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

bool checkCell(int x, int y, int moveDir){

    bool wall = false;
    if(moveDir == 0) wall = y >= PLAYFIELD_ROWS;
    else if(moveDir == 1) wall = x >=PLAYFIELD_COLS;
    else if(moveDir == 2) wall = x < 0;
    else return false;

    return wall || (PLAYFIELD[y*PLAYFIELD_COLS + x] != 0);
}

bool shapeColide(int x, int y, SHAPE shape, int rotation, int moveDir){
    // moveDir 0 is down
    // moveDir 1 is right
    // moveDir 2 is left
    // up is useless in tetris

    if (shape <= 0 || shape >= SHAPES_COUNT) return false; //invalid case
    
    rotation = rotation % 4;
    
    uint16_t shapeForm = shapeEncoders[7*rotation + (shape - 1)];

    int offX = x;
    int offY = y;

    if(moveDir != -1){
        offX += (moveDir == 0 ? 0 : moveDir == 1 ? 1 : -1);
        offY += (moveDir == 0 ? 1 : 0);
    }

    bool touched = false;

    if((shapeForm & 0b00000000001) != 0) touched |= checkCell(offX - 1, offY - 1, moveDir);
    if((shapeForm & 0b00000000010) != 0) touched |= checkCell(offX    , offY - 1, moveDir);
    if((shapeForm & 0b00000000100) != 0) touched |= checkCell(offX + 1, offY - 1, moveDir);
    if((shapeForm & 0b00000001000) != 0) touched |= checkCell(offX - 1, offY    , moveDir);
    if((shapeForm & 0b00000010000) != 0) touched |= checkCell(offX    , offY    , moveDir);
    if((shapeForm & 0b00000100000) != 0) touched |= checkCell(offX + 1, offY    , moveDir);
    if((shapeForm & 0b00001000000) != 0) touched |= checkCell(offX - 1, offY + 1, moveDir);
    if((shapeForm & 0b00010000000) != 0) touched |= checkCell(offX    , offY + 1, moveDir);
    if((shapeForm & 0b00100000000) != 0) touched |= checkCell(offX + 1, offY + 1, moveDir);
    if((shapeForm & 0b01000000000) != 0) touched |= checkCell(offX + 2, offY    , moveDir);
    if((shapeForm & 0b10000000000) != 0) touched |= checkCell(offX    , offY + 2, moveDir);

    return touched;
}

void setCell(int x, int y, uint8_t cellType){
    if(y < 0) outOfBounds = true;
    if(x < 0 || x >= PLAYFIELD_COLS || y < 0 || y >= PLAYFIELD_ROWS) return;

    PLAYFIELD[y*PLAYFIELD_COLS + x] = cellType;
}

void placeShape(int x, int y, SHAPE shape, int rotation){
    if(shape <= 0 || shape >= SHAPES_COUNT) return;
    rotation = rotation % 4;

    uint16_t shapeForm = shapeEncoders[7*rotation + (shape - 1)];

    if((shapeForm & 0b00000000001) != 0) setCell(x - 1,y - 1, shape);
    if((shapeForm & 0b00000000010) != 0) setCell(x    ,y - 1, shape);
    if((shapeForm & 0b00000000100) != 0) setCell(x + 1,y - 1, shape);

    if((shapeForm & 0b00000001000) != 0) setCell(x - 1,y    , shape);
    if((shapeForm & 0b00000010000) != 0) setCell(x    ,y    , shape);
    if((shapeForm & 0b00000100000) != 0) setCell(x + 1,y    , shape);

    if((shapeForm & 0b00001000000) != 0) setCell(x - 1,y + 1, shape);
    if((shapeForm & 0b00010000000) != 0) setCell(x    ,y + 1, shape);
    if((shapeForm & 0b00100000000) != 0) setCell(x + 1,y + 1, shape);

    if((shapeForm & 0b01000000000) != 0) setCell(x + 2,y    , shape);
    if((shapeForm & 0b10000000000) != 0) setCell(x    ,y + 2, shape);
}

void draw_encoded(double x, double y, uint16_t encoded, uint32_t color, double size){
    if((encoded & 0b00000000001) != 0) drawCell(x - size    , y - size    , size, size, color);
    if((encoded & 0b00000000010) != 0) drawCell(x           , y - size    , size, size, color);
    if((encoded & 0b00000000100) != 0) drawCell(x + size    , y - size    , size, size, color);

    if((encoded & 0b00000001000) != 0) drawCell(x - size    , y           , size, size, color);
    if((encoded & 0b00000010000) != 0) drawCell(x           , y           , size, size, color);
    if((encoded & 0b00000100000) != 0) drawCell(x + size    , y           , size, size, color);

    if((encoded & 0b00001000000) != 0) drawCell(x - size    , y + size    , size, size, color);
    if((encoded & 0b00010000000) != 0) drawCell(x           , y + size    , size, size, color);
    if((encoded & 0b00100000000) != 0) drawCell(x + size    , y + size    , size, size, color);

    if((encoded & 0b01000000000) != 0) drawCell(x + size * 2, y           , size, size, color);
    if((encoded & 0b10000000000) != 0) drawCell(x           , y + size * 2, size, size, color);
}

void drawShape(double x, double y, SHAPE shape, int rotation, uint8_t lightness) {
    uint32_t color = multiply_rgb(colors[shape], lightness);
    if(shape <= 0 || shape >= SHAPES_COUNT) {drawCell(x, y, CELL_SIZE, CELL_SIZE, color); return;}
    
    rotation = rotation % 4;

    uint16_t shapeForm = shapeEncoders[7*rotation + (shape - 1)];

    draw_encoded(x,y,shapeForm,color, CELL_SIZE);
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

int test_x = PLAYFIELD_COLS / 2;
int test_y = 0;
SHAPE shape = 1;
int rotation = 0;

float tick = 0.0;

#define MAX_PLACETIMER 2
int placeTimer = 0;

int rows_to_clear[4] = {0};
int n_rows_to_clear = 0;
int score = 0;

#define MAX_DIGITS 6

int digits[MAX_DIGITS];
int counter_num_digits;

void draw_counter(int x, int y, int number, double size){
    if(number == 0) {
        for(int i = 0; i < MAX_DIGITS; i++){
            draw_encoded(x+((size*3 + size*0.2)*i) - size*0.2 * 2,y,counterShapes[number],0xFFFFFFFF, size);
        }
    };
    if(number > pow(10,MAX_DIGITS)-1) number = pow(10,MAX_DIGITS)-1;
    counter_num_digits = 0;
    while(number / 10 != 0){
        digits[counter_num_digits++] = number % 10;
        number /= 10;
    }
    digits[counter_num_digits++] = number % 10;
    int to_fill = MAX_DIGITS - counter_num_digits;
    for(int i = counter_num_digits; i < MAX_DIGITS; i++){
        digits[i] = 0;
    }

    for(int i = 0; i < MAX_DIGITS; i++){
        draw_encoded(x+((size*3 + size*0.2)*i) - size*0.2 * 2,y,counterShapes[digits[MAX_DIGITS-1-i]],0xFFFFFF00 | (i * 30 + 10), size);
    }
}

void clear_rows(){
    //find how many and what rows to clear
    n_rows_to_clear = 0;
    for(int i = 0; i < PLAYFIELD_ROWS; i++){
        int filled = 0;
        for(int j = 0; j < PLAYFIELD_COLS; j++){
            if(PLAYFIELD[i*PLAYFIELD_COLS + j] != 0) filled++;
        }
        if(filled == PLAYFIELD_COLS){
            rows_to_clear[n_rows_to_clear++] = i;
        }
    }
    if(n_rows_to_clear == 0) return;
    for(int i = 0; i < n_rows_to_clear; i++){
        int row = rows_to_clear[i];
        memcpy(PLAYFIELD+PLAYFIELD_COLS,PLAYFIELD,PLAYFIELD_COLS*row);
        memset(PLAYFIELD,0,PLAYFIELD_COLS);
    }
    if(n_rows_to_clear == 4) score += 800;
    else score += n_rows_to_clear * 100;

}

void update_block(){
    if(placeTimer == 2) {
        if(!shapeColide(test_x,test_y,shape,rotation,0)){
            test_y++;
            placeTimer = 0;
            return;
        }

        placeShape(test_x,test_y,shape,rotation);
        test_x = PLAYFIELD_COLS / 2;
        test_y = 0;
        rotation = 0;
        shape = (rand() % (SHAPES_COUNT - 1) ) + 1;

        placeTimer = 0;
        clear_rows();
        return;
    }

    if(!shapeColide(test_x,test_y,shape,rotation,0)){
        test_y++;
        placeTimer = 0;
    }else{
        placeTimer++;
    }
}

void handleCol(){
    bool moved = shapeColide(test_x,test_y,shape,rotation,0);
    while(shapeColide(test_x,test_y,shape,rotation,0)) test_y--;
    if(moved) test_y++;

    moved = shapeColide(test_x,test_y,shape,rotation,1);
    while(shapeColide(test_x,test_y,shape,rotation,1)) test_x--;
    if(moved) test_x++;

    moved = shapeColide(test_x,test_y,shape,rotation,2);
    while(shapeColide(test_x,test_y,shape,rotation,2)) test_x++;
    if(moved) test_x--;
}

void game(double deltaTime){
    if(outOfBounds){
        for(int i = 0; i < PLAYFIELD_COLS * PLAYFIELD_ROWS; i++){
            PLAYFIELD[i] = 0;
        }
        score = 0;
        outOfBounds = false;
    }

    drawRectangle(0,0,bitmapWidth, bitmapHeight, 0xFF181818);

    lightness -= deltaTime / 10;
    if(lightness < 0.2) lightness = 0.2;

    if(just_pressed_keys['D'] && !shapeColide(test_x,test_y,shape,rotation,1)) test_x++;
    if(just_pressed_keys['A'] && !shapeColide(test_x,test_y,shape,rotation,2)) test_x--;

    if(just_pressed_keys['E']) {
        rotation = (rotation + 1) % 4;
        handleCol();
    }

    if(just_pressed_keys['Q']) {
        rotation = (rotation - 1) > 0 ? (rotation - 1) : 4 + (rotation - 1);
        handleCol();
    }

    if(just_pressed_keys[VK_SPACE]) {
        do {
            update_block();
        }while(test_y != 0);
    }

    fallSpeed = keys['S'] ? DEFAULT_FALL_SPEED * 8 : DEFAULT_FALL_SPEED;

    // if(just_pressed_keys['1']) shape = (shape + 1) % 7;
    // if(just_pressed_keys['2']) shape = (shape - 1) > 0 ? (shape - 1) : 7 + (shape - 1);

    tick += deltaTime * fallSpeed;

    if(tick >= 1.0) {
        tick = 0.0;
        update_block();
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
    drawShape(offX,offY, shape, rotation, (1.0 - (double)placeTimer/(MAX_PLACETIMER+1)) * 255);

    int number_of_counters = MAX_DIGITS;
    double scoreBoardCell = (double)CELL_SIZE * 0.3;
    double scoreBoardWidth = scoreBoardCell*(number_of_counters*3 + 2);
    double scoreBoardHeight = scoreBoardCell*(3+2);
    double scoreBoardX = ((double)bitmapWidth/2 + (double)(PLAYFIELD_COLS+1) * (double)CELL_SIZE / 2.0) + ((double)CELL_SIZE)/16.0;
    double scoreBoardY = (double)bitmapHeight/2 - scoreBoardHeight / 2;

    drawRectangle(scoreBoardX,scoreBoardY, scoreBoardWidth,scoreBoardHeight, 0xFF181818);
    draw_counter(scoreBoardX+scoreBoardCell*2,scoreBoardY + scoreBoardHeight/2 - scoreBoardCell/2, score, scoreBoardCell);
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

    shape = (rand() % (SHAPES_COUNT - 1) ) + 1;

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
