#pragma once

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

// #define FLASH_STRING(str) ((const char[]){str})
#define FLASH_STRING(str) ({ static const char flash_str[] = str; flash_str; })
#define FS(str) FLASH_STRING(str)

#define ESC     "\x1B["

#define CLEAR_SCREEN        ESC "2J"
#define CLEAR_LINE          ESC "2K"

#define CURSOR_HOME         ESC "H"
#define CURSOR_UP(n)        ESC #n "A"
#define CURSOR_DOWN(n)      ESC #n "B"
#define CURSOR_FORWARD(n)   ESC #n "C"
#define CURSOR_BACK(n)      ESC #n "D"
#define CURSOR_SET(x,y)     ESC #x ";" #y "H"
#define CURSOR_SHOW         ESC "?25h"
#define CURSOR_HIDE         ESC "?25l"

#define MOUSE_ON            ESC "?1003h"
#define MOUSE_OFF           ESC "?1003l"

#define SOUND_BEEP          "\a"

#define TEXT_RESET      ESC "0m"
#define TEXT_BOLD       ESC "1m"
#define TEXT_UNDERLINE  ESC "4m"
#define TEXT_BLINK      ESC "5m"
#define TEXT_REVERSE    ESC "7m"

#define TEXT_BLACK   ESC "30m"
#define TEXT_RED     ESC "31m"
#define TEXT_GREEN   ESC "32m"
#define TEXT_YELLOW  ESC "33m"
#define TEXT_BLUE    ESC "34m"
#define TEXT_MAGENTA ESC "35m"
#define TEXT_CYAN    ESC "36m"
#define TEXT_WHITE   ESC "37m"

#define BG_BLACK   ESC "40m"
#define BG_RED     ESC "41m"
#define BG_GREEN   ESC "42m"
#define BG_YELLOW  ESC "43m"
#define BG_BLUE    ESC "44m"
#define BG_MAGENTA ESC "45m"
#define BG_CYAN    ESC "46m"
#define BG_WHITE   ESC "47m"

#define TEXT_BRIGHT_BLACK   ESC "90m"
#define TEXT_BRIGHT_RED     ESC "91m"
#define TEXT_BRIGHT_GREEN   ESC "92m"
#define TEXT_BRIGHT_YELLOW  ESC "93m"
#define TEXT_BRIGHT_BLUE    ESC "94m"
#define TEXT_BRIGHT_MAGENTA ESC "95m"
#define TEXT_BRIGHT_CYAN    ESC "96m"
#define TEXT_BRIGHT_WHITE   ESC "97m"

#define BG_BRIGHT_BLACK   ESC "100m"
#define BG_BRIGHT_RED     ESC "101m"
#define BG_BRIGHT_GREEN   ESC "102m"
#define BG_BRIGHT_YELLOW  ESC "103m"
#define BG_BRIGHT_BLUE    ESC "104m"
#define BG_BRIGHT_MAGENTA ESC "105m"
#define BG_BRIGHT_CYAN    ESC "106m"
#define BG_BRIGHT_WHITE   ESC "107m"

#define ASCII_NULL        (0x00)  /* Null terminator (end of the string) */
#define ASCII_NEWLINE     (0x0A)  /* Line Feed (LF) (\n) */
#define ASCII_CARRIAGE    (0x0D)  /* Carriage Return (CR) (\r) */
#define ASCII_TAB         (0x09)  /* Horizontal Tab (\t) */
#define ASCII_BACKSPACE   (0x08)  /* Backspace */
#define ASCII_BELL        (0x07)  /* Bell (sound signal) */
#define ASCII_ESCAPE      (0x1B)  /* Escape (ESC) */
#define STAB              "  "

/*
#include "terminal_tools.h"
*/
#define RED(text)       TEXT_RED    text    TEXT_RESET
#define GREEN(text)     TEXT_GREEN  text    TEXT_RESET
#define YELLOW(text)    TEXT_YELLOW text    TEXT_RESET

#if 1
// DEBUG > 0
    #include <stdio.h>
    #define LLOG(...)    printf(__VA_ARGS__);                /* Lazy log */
    #define LOG(...)     LLOG(__VA_ARGS__); fflush(stdout);  /* Strong log */
#else
    #define LLOG(...)
    #define LOG(...)
#endif
    #define WARNING(...)    LLOG(TEXT_YELLOW __VA_ARGS__); LOG(TEXT_RESET "\n")
    #define ERROR(...)      LLOG(TEXT_RED __VA_ARGS__);    LOG(TEXT_RESET "\n")
    #define HALT(...)       ERROR(__VA_ARGS__);  while (1) {}
