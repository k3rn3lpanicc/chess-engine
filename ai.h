#ifndef AI_H
#define AI_H
#include "board.h"
typedef struct
{
    Cell to;
    Cell from;
    // Extra for full reversibility:
    int castling_W_K, castling_W_Q, castling_B_K, castling_B_Q;
    int did_castle;        // 0 no, 1 kingside, -1 queenside
    int rook_fx, rook_fy;  // rook from (for castling)
    int rook_tx, rook_ty;  // rook to   (for castling)
    int did_promo;         // 1 if we promoted a pawn
    char promo_prev_piece; // original piece before promotion (should be 'P')
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