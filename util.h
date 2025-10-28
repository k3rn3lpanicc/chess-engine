#ifndef UTIL_H
#define UTIL_H
#include "board.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif
void position_key(Board *b, char color_to_move, char *out, size_t out_sz);
void history_increment(Board *b, const char *key);
int input_line(char *buf, size_t n);
void format_square(int x, int y, char *out);
int parse_square(const char *s, int *out_x, int *out_y);
void board_draw(Board *b, Pos *highlights, int n_highlights);
int pos_in_list(Pos *list, int n, int x, int y);
const char *piece_unicode(char piece, char color);
char piece_symbol(char piece, char color);
void clear_console();

#endif