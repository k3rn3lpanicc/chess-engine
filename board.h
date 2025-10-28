#ifndef BOARD_H
#define BOARD_H

#include <stddef.h>

typedef struct
{
    char state; // 'W', 'B', or 'E'
    char piece; // 'P','R','N','B','Q','K' or 0
} Cell;

typedef struct
{
    Cell cells[8][8];
    int castling_W_K, castling_W_Q, castling_B_K, castling_B_Q;
    struct
    {
        char key[512];
        int count;
    } history[8192];
    int history_size;
} Board;

typedef struct
{
    int x, y;
} Pos;
typedef struct
{
    int from_x, from_y, to_x, to_y;
} Move;

static inline char opposite_color(char c) { return c == 'W' ? 'B' : 'W'; }

void board_init(Board *b);
void board_set_piece(Board *b, int x, int y, char piece, char color);
void board_apply_move(Board *b, int from_x, int from_y, int to_x, int to_y);
int board_find_king(Board *b, char color, int *outx, int *outy);
int board_is_in_check(Board *b, char color);
int board_is_checkmate(Board *b, char color);
int board_is_stalemate(Board *b, char color);
int board_can_castle(Board *b, char color, char side);
void position_key(Board *b, char color_to_move, char *out, size_t out_sz);
void history_increment(Board *b, const char *key);
int board_threefold(Board *b, char color_to_move);
void get_attack_squares(Board *b, int x, int y, Pos *out, int *out_count);
#endif