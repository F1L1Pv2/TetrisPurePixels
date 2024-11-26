set -xe

clang -g -D DEBUG -D WINDOWS_PLATFORM -o tetris.exe src/main.c src/windows_platform.c -luser32 -lgdi32