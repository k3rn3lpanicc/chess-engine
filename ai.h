#ifndef AI_H
#define AI_H
#include "board.h"
typedef struct
{
    Cell to;
    Cell from;
} Snapshot;
int board_threefold(Board *b, char color_to_move);
double evaluate_board(Board *b, char color_to_move);
double minimax(Board *b, int depth, double alpha, double beta, int maximizing_player, char color_to_move, Move *best);
void order_moves(Board *b, Move *moves, int n, char color);
void get_attack_squares(Board *b, int x, int y, Pos *out, int *out_count);
void collect_legal_moves(Board *b, char color, Move *out, int *out_n);
void make_move(Board *b, int from_x, int from_y, int to_x, int to_y, Snapshot *snap);
void undo_move(Board *b, int from_x, int from_y, int to_x, int to_y, Snapshot *snap);
int engine(Board *b, char color, int depth);
int count_legal_moves(Board *b, char color);
int adaptive_depth_by_moves(Board *b, char color);
double phase_score(Board *b);
int adaptive_depth_combined(Board *b, char color);
#endif