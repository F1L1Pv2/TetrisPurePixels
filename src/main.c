//  ___________     __         .__        
//  \__    ___/____/  |________|__| ______
//    |    |_/ __ \   __\_  __ \  |/  ___/
//    |    |\  ___/|  |  |  | \/  |\___ \ 
//    |____| \___  >__|  |__|  |__/____  >
//               \/        PURE PIXELS \/ 
//      made by F1L1Pv2


#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "time.h"
#include <string.h>
#include "platform.h"

#define RAND_MAX 0x7fff

#ifdef WINDOWS_PLATFORM
#include "Windows.h"
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
#endif

const uint64_t FPS = 120;

#define CELL_SIZE (platform_screen_width()/25)
#define NUMBER_OF_CELLS 1000
#define MAX_ACC (platform_screen_width()*0.75)
#define PLAYFIELD_COLS 10
#define PLAYFIELD_ROWS 16
#define gravity (-9.8 * platform_screen_height()/40)
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
    0b00000000000, //a
    0b00000000000, //b
    0b00000000000, //c
    0b00000000000, //d
    0b00000000000, //e
    0b00001011111, //f
    0b00000000000, //g
    0b00000000000, //h
    0b00111010111, //i
    0b00000000000, //j
    0b00000000000, //k
    0b00111001001, //l
    0b00000000000, //m
    0b00000000000, //n
    0b00000000000, //o
    0b00001111111, //p
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
    platform_draw_rectangle(x,y,width,height, COLOR_WITH_TINT(0x7F, tint));

    double diffX = (double)width * 0.2;
    double diffY = (double)height * 0.2;

    platform_draw_rectangle(x + (int64_t)(diffX * 0.5), y + (int64_t)(diffY * 0.5), width - (int64_t)diffX, height - (int64_t)diffY, COLOR_WITH_TINT(0x70, tint));
    platform_draw_rectangle(x + (int64_t)(diffX), y + (int64_t)(diffY), width - (int64_t)(diffX * 2), height - (int64_t)(diffY * 2), COLOR_WITH_TINT(0xAA, tint));
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
    cell->x = (double)platform_screen_width() / 2 + ((double)rand() / RAND_MAX * 2.0 - 1.0 ) * 100;
    cell->y = (double)platform_screen_width() / 2 + ((double)rand() / RAND_MAX * 2.0 - 1.0 ) * 100;
    cell->accx = ((double)rand() / RAND_MAX * 2.0 - 1.0 ) * MAX_ACC;
    cell->accy = ((double)rand() / RAND_MAX * 2.0 - 1.0 ) * MAX_ACC * 1.2 * -1.0;

    cell->shape = (rand() % (SHAPES_COUNT - 1) ) + 1;
    cell->rotation = rand() % 4;
}

double lightness = 0.8;

int player_x = PLAYFIELD_COLS / 2;
int player_y = 0;
SHAPE player_shape = 1;
int player_rotation = 0;

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
        if(!shapeColide(player_x,player_y,player_shape,player_rotation,0)){
            player_y++;
            placeTimer = 0;
            return;
        }

        placeShape(player_x,player_y,player_shape,player_rotation);
        player_x = PLAYFIELD_COLS / 2;
        player_y = 0;
        player_rotation = 0;
        player_shape = (rand() % (SHAPES_COUNT - 1) ) + 1;

        placeTimer = 0;
        clear_rows();
        return;
    }

    if(!shapeColide(player_x,player_y,player_shape,player_rotation,0)){
        player_y++;
        placeTimer = 0;
    }else{
        placeTimer++;
    }
}

void handleCol(){
    bool moved = shapeColide(player_x,player_y,player_shape,player_rotation,0);
    while(shapeColide(player_x,player_y,player_shape,player_rotation,0)) player_y--;
    if(moved) player_y++;

    moved = shapeColide(player_x,player_y,player_shape,player_rotation,1);
    while(shapeColide(player_x,player_y,player_shape,player_rotation,1)) player_x--;
    if(moved) player_x++;

    moved = shapeColide(player_x,player_y,player_shape,player_rotation,2);
    while(shapeColide(player_x,player_y,player_shape,player_rotation,2)) player_x++;
    if(moved) player_x--;
}

void game(double deltaTime){
    if(outOfBounds){
        for(int i = 0; i < PLAYFIELD_COLS * PLAYFIELD_ROWS; i++){
            PLAYFIELD[i] = 0;
        }
        score = 0;
        outOfBounds = false;
    }

    platform_draw_rectangle(0,0,platform_screen_width(), platform_screen_height(), 0xFF181818);

    lightness -= deltaTime / 10;
    if(lightness < 0.2) lightness = 0.2;

    if(platform_move_right() && !shapeColide(player_x,player_y,player_shape,player_rotation,1)) player_x++;
    if(platform_move_left() && !shapeColide(player_x,player_y,player_shape,player_rotation,2)) player_x--;

    if(platform_rot_right()) {
        player_rotation = (player_rotation + 1) % 4;
        handleCol();
    }

    if(platform_rot_left()) {
        player_rotation = (player_rotation - 1) > 0 ? (player_rotation - 1) : 4 + (player_rotation - 1);
        handleCol();
    }

    if(platform_move_jump_down()) {
        do {
            update_block();
        }while(player_y != 0);
    }

    fallSpeed = platform_move_down() ? DEFAULT_FALL_SPEED * 8 : DEFAULT_FALL_SPEED;

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

        if(cell->x - (double)CELL_SIZE * 3 >= platform_screen_width() || cell->x + (double)CELL_SIZE * 3 < 0 || cell->y - (double)CELL_SIZE * 3 >= platform_screen_height() || cell->y + (double)CELL_SIZE * 3 < 0){
            randomizeCell(cell);
        }
    }

    platform_draw_rectangle(((double)platform_screen_width()/2 - (double)(PLAYFIELD_COLS + 1)* CELL_SIZE / 2) - (double)CELL_SIZE / 2,((double)platform_screen_height()/2 - (double)(PLAYFIELD_ROWS + 1)* CELL_SIZE / 2) - (double)CELL_SIZE / 2,(double)(PLAYFIELD_COLS + 1)* CELL_SIZE, (double)(PLAYFIELD_ROWS + 1)* CELL_SIZE, 0xFF181818);
    for(uint64_t i = 0; i < PLAYFIELD_ROWS; i++){
        double y = ((double)platform_screen_height() / 2 - (double)PLAYFIELD_ROWS * CELL_SIZE / 2 + i * CELL_SIZE) - (double)CELL_SIZE / 2;
        for(uint64_t j = 0; j < PLAYFIELD_COLS; j++){
            double x = ((double)platform_screen_width() / 2 - (double)PLAYFIELD_COLS * CELL_SIZE / 2 + j * CELL_SIZE) - (double)CELL_SIZE / 2;

            drawCell(x,y,CELL_SIZE,CELL_SIZE,colors[PLAYFIELD[i*PLAYFIELD_COLS+j]]);
        }
    }

    double offX = ((double)platform_screen_width() / 2 - (double)PLAYFIELD_COLS * CELL_SIZE / 2 + player_x * CELL_SIZE) - (double)CELL_SIZE / 2;
    double offY = ((double)platform_screen_height() / 2 - (double)PLAYFIELD_ROWS * CELL_SIZE / 2 + player_y * CELL_SIZE) - (double)CELL_SIZE / 2;
    drawShape(offX,offY, player_shape, player_rotation, (1.0 - (double)placeTimer/(MAX_PLACETIMER+1)) * 255);

    int number_of_counters = MAX_DIGITS;
    double scoreBoardCell = (double)CELL_SIZE * 0.3;
    double scoreBoardWidth = scoreBoardCell*(number_of_counters*3 + 2);
    double scoreBoardHeight = scoreBoardCell*(3+2);
    double scoreBoardX = ((double)platform_screen_width()/2 + (double)(PLAYFIELD_COLS+1) * (double)CELL_SIZE / 2.0) + ((double)CELL_SIZE)/16.0;
    double scoreBoardY = (double)platform_screen_height()/2 - scoreBoardHeight / 2;

    platform_draw_rectangle(scoreBoardX,scoreBoardY, scoreBoardWidth,scoreBoardHeight, 0xFF181818);
    draw_counter(scoreBoardX+scoreBoardCell*2,scoreBoardY + scoreBoardHeight/2 - scoreBoardCell/2, score, scoreBoardCell);

    double text_cell_size = (double)CELL_SIZE / 3;

    draw_encoded(text_cell_size*(1.5+3 * 0),platform_screen_height()-text_cell_size*8,counterShapes['F' - 'A' + 10],0xFF404040,text_cell_size);
    draw_encoded(text_cell_size*(1.5+3 * 1),platform_screen_height()-text_cell_size*8,counterShapes[1],0xFF404040,text_cell_size);
    draw_encoded(text_cell_size*(1.5+3 * 2),platform_screen_height()-text_cell_size*8,counterShapes['L' - 'A' + 10],0xFF404040,text_cell_size);
    draw_encoded(text_cell_size*(1.5+3 * 3),platform_screen_height()-text_cell_size*8,counterShapes[1],0xFF404040,text_cell_size);
    draw_encoded(text_cell_size*(1.5+3 * 4),platform_screen_height()-text_cell_size*8,counterShapes['P' - 'A' + 10],0xFF404040,text_cell_size);
}


double Time = 0.0;

int main() {
    srand(time(NULL));

    platform_init();

    for(uint64_t i = 0; i < NUMBER_OF_CELLS; i++){
        Cell* cell = cells+i;

        randomizeCell(cell);
        cell->x = (double)rand() / RAND_MAX * platform_screen_width();
        cell->y = (double)rand() / RAND_MAX * platform_screen_height();
    }

    player_shape = (rand() % (SHAPES_COUNT - 1) ) + 1;

    while (1) {
        if(!platform_pre_game_update()) break;

        if(platform_get_deltaTime() > 2.0 / FPS) platform_set_deltaTime(2.0 / FPS);

        game(platform_get_deltaTime());

        platform_post_game_update();

        double frameTime = 1.0 / FPS;
        if (platform_get_deltaTime() < frameTime) {
            Sleep((int)((frameTime - platform_get_deltaTime()) * 1000));
        }
    }

    platform_cleanup();

    return 0;
}
