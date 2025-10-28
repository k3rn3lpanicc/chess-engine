#include "ai.h"
#include "move_gen.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

int board_threefold(Board *b, char color_to_move)
{
    char key[512];
    position_key(b, color_to_move, key, sizeof(key));
    for (int i = 0; i < b->history_size; i++)
    {
        if (strcmp(b->history[i].key, key) == 0)
        {
            return b->history[i].count >= 3;
        }
    }
    return 0;
}

double evaluate_board(Board *b, char color_to_move)
{
    const double values[128] = {['P'] = 1, ['N'] = 3, ['B'] = 3, ['R'] = 5, ['Q'] = 9, ['K'] = 100};
    double score = 0.0;

    if (board_threefold(b, color_to_move))
        return 0.0;

    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            Cell c = b->cells[i][j];
            if (c.state == 'E' || !c.piece)
                continue;
            double v = values[(int)c.piece];
            if (c.state == 'W')
                score += v;
            else
                score -= v;
        }
    }

    if (board_is_in_check(b, 'B'))
        score += 0.5;
    if (board_is_in_check(b, 'W'))
        score -= 0.5;

    if (board_is_checkmate(b, 'B'))
        score += 1e10;
    if (board_is_checkmate(b, 'W'))
        score -= 1e10;

    return score;
}

void collect_legal_moves(Board *b, char color, Move *out, int *out_n)
{
    *out_n = 0;
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (b->cells[i][j].state != color)
                continue;
            Pos ps[64];
            int pn = 0, ln = 0;
            get_available_moves(b, i, j, 1, ps, &pn);
            Pos leg[64];
            filter_legal_moves(b, i, j, ps, pn, color, leg, &ln);
            for (int k = 0; k < ln; k++)
            {
                out[*out_n] = (Move){i, j, leg[k].x, leg[k].y};
                (*out_n)++;
            }
        }
    }
}

void order_moves(Board *b, Move *moves, int n, char color)
{
    // Simple MVV-LVA: 10*victim - attacker
    int val[128] = {['P'] = 1, ['N'] = 3, ['B'] = 3, ['R'] = 5, ['Q'] = 9, ['K'] = 1000};
    int *scores = (int *)malloc(sizeof(int) * n);
    for (int i = 0; i < n; i++)
    {
        Cell tgt = b->cells[moves[i].to_x][moves[i].to_y];
        int cap = 0;
        if (tgt.state != 'E' && tgt.state != color)
        {
            char attacker = b->cells[moves[i].from_x][moves[i].from_y].piece;
            cap = 10 * val[(int)tgt.piece] - val[(int)attacker];
        }
        scores[i] = cap;
    }
    // sort desc by scores
    for (int i = 0; i < n; i++)
    {
        for (int j = i + 1; j < n; j++)
        {
            if (scores[j] > scores[i])
            {
                int ts = scores[i];
                scores[i] = scores[j];
                scores[j] = ts;
                Move tm = moves[i];
                moves[i] = moves[j];
                moves[j] = tm;
            }
        }
    }
    free(scores);
}

void make_move(Board *b, int from_x, int from_y, int to_x, int to_y, Snapshot *snap)
{
    snap->from = b->cells[from_x][from_y];
    snap->to = b->cells[to_x][to_y];
    b->cells[to_x][to_y] = b->cells[from_x][from_y];
    b->cells[from_x][from_y] = (Cell){'E', 0};

    if (b->cells[to_x][to_y].piece == 'P')
    {
        if ((b->cells[to_x][to_y].state == 'W' && to_x == 0) ||
            (b->cells[to_x][to_y].state == 'B' && to_x == 7))
        {
            b->cells[to_x][to_y].piece = 'Q';
        }
    }
    // NOTE: As in Python AI, this does NOT update castling rook movement or rights.
}

void undo_move(Board *b, int from_x, int from_y, int to_x, int to_y, Snapshot *snap)
{
    b->cells[from_x][from_y] = snap->from;
    b->cells[to_x][to_y] = snap->to;
}

double minimax(Board *b, int depth, double alpha, double beta, int maximizing, char color_to_move, Move *best)
{
    if (board_threefold(b, color_to_move))
        return 0.0;
    if (depth == 0 || board_is_checkmate(b, 'W') || board_is_checkmate(b, 'B') || board_is_stalemate(b, 'W') || board_is_stalemate(b, 'B'))
    {
        return evaluate_board(b, color_to_move);
    }

    char color = maximizing ? 'W' : 'B';
    Move moves[256];
    int n = 0;
    collect_legal_moves(b, color, moves, &n);
    order_moves(b, moves, n, color);
    if (n == 0)
        return evaluate_board(b, color_to_move);

    double best_eval = maximizing ? -INFINITY : INFINITY;
    Move best_local = moves[0];

    for (int i = 0; i < n; i++)
    {
        Snapshot snap;
        make_move(b, moves[i].from_x, moves[i].from_y, moves[i].to_x, moves[i].to_y, &snap);

        double val = minimax(b, depth - 1, alpha, beta, !maximizing, opposite_color(color_to_move), NULL);

        undo_move(b, moves[i].from_x, moves[i].from_y, moves[i].to_x, moves[i].to_y, &snap);

        if (maximizing)
        {
            if (val > best_eval)
            {
                best_eval = val;
                best_local = moves[i];
            }
            if (val > alpha)
                alpha = val;
            if (beta <= alpha)
                break;
        }
        else
        {
            if (val < best_eval)
            {
                best_eval = val;
                best_local = moves[i];
            }
            if (val < beta)
                beta = val;
            if (beta <= alpha)
                break;
        }
    }
    if (best)
        *best = best_local;
    return best_eval;
}

int engine(Board *b, char color, int depth)
{
    // Gather legal moves
    Move moves[256];
    int n = 0;
    collect_legal_moves(b, color, moves, &n);
    if (n == 0)
    {
        if (board_is_checkmate(b, color))
        {
            printf("Checkmate! %s wins.\n", (color == 'B') ? "White" : "Black");
        }
        else
        {
            printf("Stalemate! Draw.\n");
        }
        return 0; // game_over
    }

    // Evaluate each by calling minimax from the next side's perspective
    Move best;
    double score;
    if (color == 'W')
    {
        score = minimax(b, depth, -INFINITY, INFINITY, 1, 'W', &best);
    }
    else
    {
        score = minimax(b, depth, -INFINITY, INFINITY, 0, 'B', &best);
    }

    // Apply best
    Cell cap = b->cells[best.to_x][best.to_y];
    board_apply_move(b, best.from_x, best.from_y, best.to_x, best.to_y);
    char from_file = 'a' + best.from_y;
    int from_rank = 8 - best.from_x;
    char to_file = 'a' + best.to_y;
    int to_rank = 8 - best.to_x;

    if (cap.state != 'E')
    {
        printf("%s captures %c at %c%d (%c%d \342\206\222 %c%d)\n",
               (color == 'B') ? "Black" : "White",
               cap.piece,
               to_file, to_rank,
               from_file, from_rank, to_file, to_rank);
    }
    else
    {
        printf("%s moves %c%d \342\206\222 %c%d [eval=%.2f]\n",
               (color == 'B') ? "Black" : "White",
               from_file, from_rank, to_file, to_rank, score);
    }
    return 1; // moved
}
