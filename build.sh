set -xe

clang -g -D DEBUG -o tetris.exe src/main.c -luser32 -lgdi32