#ifndef MOVE_GEN_H
#define MOVE_GEN_H
#include "board.h"

void append_pos(Pos *out, int *n, int x, int y);
void get_available_moves(Board *b, int x, int y, int include_castling, Pos *out, int *out_count);
void filter_legal_moves(Board *b, int x, int y, Pos *moves, int moves_count, char color, Pos *out, int *out_count);


#endif