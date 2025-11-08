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
    const double values[128] = {['P'] = 100, ['N'] = 320, ['B'] = 330, ['R'] = 500, ['Q'] = 900, ['K'] = 20000};
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
        score += 50;
    if (board_is_in_check(b, 'W'))
        score -= 50;

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

void collect_capture_moves(Board *b, char color, Move *out, int *out_n)
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
            get_available_moves(b, i, j, 0, ps, &pn);
            Pos leg[64];
            filter_legal_moves(b, i, j, ps, pn, color, leg, &ln);
            for (int k = 0; k < ln; k++)
            {
                // Only include captures
                Cell target = b->cells[leg[k].x][leg[k].y];
                if (target.state != 'E' && target.state != color)
                {
                    out[*out_n] = (Move){i, j, leg[k].x, leg[k].y};
                    (*out_n)++;
                }
            }
        }
    }
}

void order_moves(Board *b, Move *moves, int n, char color)
{
    // Simple MVV-LVA: 10*victim - attacker
    int val[128] = {['P'] = 100, ['N'] = 320, ['B'] = 330, ['R'] = 500, ['Q'] = 900, ['K'] = 20000};
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

void make_move(Board *b, int fx, int fy, int tx, int ty, Snapshot *s)
{
    s->from = b->cells[fx][fy];
    s->to = b->cells[tx][ty];

    // Save castling rights
    s->castling_W_K = b->castling_W_K;
    s->castling_W_Q = b->castling_W_Q;
    s->castling_B_K = b->castling_B_K;
    s->castling_B_Q = b->castling_B_Q;

    s->did_castle = 0;
    s->did_promo = 0;

    char piece = b->cells[fx][fy].piece;
    char color = b->cells[fx][fy].state;

    // Move the piece
    b->cells[tx][ty] = b->cells[fx][fy];
    b->cells[fx][fy] = (Cell){'E', 0};

    // Handle castling (king moved 2 files)
    if (piece == 'K' && abs(ty - fy) == 2)
    {
        int row = fx;
        if (ty > fy)
        { // kingside
            // rook h -> f
            b->cells[row][5] = b->cells[row][7];
            b->cells[row][7] = (Cell){'E', 0};
            s->did_castle = 1;
            s->rook_fx = row;
            s->rook_fy = 7;
            s->rook_tx = row;
            s->rook_ty = 5;
        }
        else
        { // queenside
            // rook a -> d
            b->cells[row][3] = b->cells[row][0];
            b->cells[row][0] = (Cell){'E', 0};
            s->did_castle = -1;
            s->rook_fx = row;
            s->rook_fy = 0;
            s->rook_tx = row;
            s->rook_ty = 3;
        }
        // Revoke castling rights for that side
        if (color == 'W')
        {
            b->castling_W_K = 0;
            b->castling_W_Q = 0;
        }
        else
        {
            b->castling_B_K = 0;
            b->castling_B_Q = 0;
        }
    }

    // Revoke rights if king/rook moved (non-castling moves)
    if (piece == 'K')
    {
        if (color == 'W')
        {
            b->castling_W_K = 0;
            b->castling_W_Q = 0;
        }
        else
        {
            b->castling_B_K = 0;
            b->castling_B_Q = 0;
        }
    }
    else if (piece == 'R')
    {
        if (color == 'W')
        {
            if (fx == 7 && fy == 0)
                b->castling_W_Q = 0;
            else if (fx == 7 && fy == 7)
                b->castling_W_K = 0;
        }
        else
        {
            if (fx == 0 && fy == 0)
                b->castling_B_Q = 0;
            else if (fx == 0 && fy == 7)
                b->castling_B_K = 0;
        }
    }

    // Handle promotion
    if (piece == 'P')
    {
        if ((color == 'W' && tx == 0) || (color == 'B' && tx == 7))
        {
            s->did_promo = 1;
            s->promo_prev_piece = 'P';
            b->cells[tx][ty].piece = 'Q';
        }
    }
}

void undo_move(Board *b, int fx, int fy, int tx, int ty, Snapshot *s)
{
    // Undo promotion
    if (s->did_promo)
    {
        b->cells[tx][ty].piece = s->promo_prev_piece; // back to pawn
    }

    // Undo castling rook move if any
    if (s->did_castle != 0)
    {
        b->cells[s->rook_fx][s->rook_fy] = b->cells[s->rook_tx][s->rook_ty];
        b->cells[s->rook_tx][s->rook_ty] = (Cell){'E', 0};
    }

    // Restore the piece move
    b->cells[fx][fy] = s->from;
    b->cells[tx][ty] = s->to;

    // Restore castling rights
    b->castling_W_K = s->castling_W_K;
    b->castling_W_Q = s->castling_W_Q;
    b->castling_B_K = s->castling_B_K;
    b->castling_B_Q = s->castling_B_Q;
}

// Quiescence search with delta pruning to handle tactical positions
double quiescence(Board *b, double alpha, double beta, int maximizing, char color_to_move, int ply_from_root)
{
    const int MAX_QUIESCE_DEPTH = 10;
    
    // Penalize repetitions heavily to avoid tempo moves
    if (board_threefold(b, color_to_move))
        return 0.0;
    
    // Check for two-fold repetition and discourage it
    char key[512];
    position_key(b, color_to_move, key, sizeof(key));
    int rep_count = 0;
    for (int i = 0; i < b->history_size; i++)
    {
        if (strcmp(b->history[i].key, key) == 0)
        {
            rep_count = b->history[i].count;
            break;
        }
    }
    if (rep_count >= 2)
        return maximizing ? -50.0 : 50.0; // Penalize repetition
    
    // Stand pat evaluation
    double stand_pat = evaluate_board(b, color_to_move);
    
    // Depth-dependent scoring: reduce score as we go deeper to prefer shorter mates
    if (stand_pat > 1e9) // Checkmate for white
        stand_pat -= ply_from_root * 10;
    else if (stand_pat < -1e9) // Checkmate for black
        stand_pat += ply_from_root * 10;
    
    if (ply_from_root >= MAX_QUIESCE_DEPTH)
        return stand_pat;
    
    if (maximizing)
    {
        if (stand_pat >= beta)
            return beta;
        if (alpha < stand_pat)
            alpha = stand_pat;
    }
    else
    {
        if (stand_pat <= alpha)
            return alpha;
        if (beta > stand_pat)
            beta = stand_pat;
    }
    
    // Only search captures
    char color = maximizing ? 'W' : 'B';
    Move moves[256];
    int n = 0;
    collect_capture_moves(b, color, moves, &n);
    order_moves(b, moves, n, color);
    
    // Delta pruning: skip if no capture can improve position
    const double DELTA_MARGIN = 900.0; // Queen value
    if (maximizing)
    {
        if (stand_pat + DELTA_MARGIN < alpha && n > 0)
        {
            // Check if even best capture can't reach alpha
            Cell target = b->cells[moves[0].to_x][moves[0].to_y];
            const double values[128] = {['P'] = 100, ['N'] = 320, ['B'] = 330, ['R'] = 500, ['Q'] = 900};
            double best_capture_value = (target.state != 'E') ? values[(int)target.piece] : 0;
            if (stand_pat + best_capture_value + DELTA_MARGIN < alpha)
                return alpha;
        }
    }
    else
    {
        if (stand_pat - DELTA_MARGIN > beta && n > 0)
        {
            Cell target = b->cells[moves[0].to_x][moves[0].to_y];
            const double values[128] = {['P'] = 100, ['N'] = 320, ['B'] = 330, ['R'] = 500, ['Q'] = 900};
            double best_capture_value = (target.state != 'E') ? values[(int)target.piece] : 0;
            if (stand_pat - best_capture_value - DELTA_MARGIN > beta)
                return beta;
        }
    }
    
    double best_eval = stand_pat;
    
    for (int i = 0; i < n; i++)
    {
        Snapshot snap;
        make_move(b, moves[i].from_x, moves[i].from_y, moves[i].to_x, moves[i].to_y, &snap);
        
        double val = quiescence(b, alpha, beta, !maximizing, opposite_color(color_to_move), ply_from_root + 1);
        
        undo_move(b, moves[i].from_x, moves[i].from_y, moves[i].to_x, moves[i].to_y, &snap);
        
        if (maximizing)
        {
            if (val > best_eval)
                best_eval = val;
            if (val > alpha)
                alpha = val;
            if (beta <= alpha)
                break;
        }
        else
        {
            if (val < best_eval)
                best_eval = val;
            if (val < beta)
                beta = val;
            if (beta <= alpha)
                break;
        }
    }
    
    return best_eval;
}

double minimax(Board *b, int depth, double alpha, double beta, int maximizing, char color_to_move, Move *best, int ply_from_root)
{
    // Penalize three-fold repetition
    if (board_threefold(b, color_to_move))
        return 0.0;
    
    // Detect and heavily penalize two-fold repetition to prevent tempo moves
    char key[512];
    position_key(b, color_to_move, key, sizeof(key));
    int rep_count = 0;
    for (int i = 0; i < b->history_size; i++)
    {
        if (strcmp(b->history[i].key, key) == 0)
        {
            rep_count = b->history[i].count;
            break;
        }
    }
    
    // Strong penalty for repeating positions
    if (rep_count >= 2)
    {
        double penalty = maximizing ? -200.0 : 200.0;
        return penalty;
    }
    
    if (depth == 0 || board_is_checkmate(b, 'W') || board_is_checkmate(b, 'B') || board_is_stalemate(b, 'W') || board_is_stalemate(b, 'B'))
    {
        // Use quiescence search instead of static evaluation
        return quiescence(b, alpha, beta, maximizing, color_to_move, ply_from_root);
    }

    char color = maximizing ? 'W' : 'B';
    Move moves[256];
    int n = 0;
    collect_legal_moves(b, color, moves, &n);
    order_moves(b, moves, n, color);
    if (n == 0)
        return quiescence(b, alpha, beta, maximizing, color_to_move, ply_from_root);

    double best_eval = maximizing ? -INFINITY : INFINITY;
    Move best_local = moves[0];

    for (int i = 0; i < n; i++)
    {
        // Heavily penalize moves that undo the last move (tempo moves)
        if (ply_from_root == 0 && b->has_last_move &&
            moves[i].from_x == b->last_to_x && moves[i].from_y == b->last_to_y &&
            moves[i].to_x == b->last_from_x && moves[i].to_y == b->last_from_y)
        {
            // This is an immediate undo move - skip it at root
            continue;
        }
        
        Snapshot snap;
        make_move(b, moves[i].from_x, moves[i].from_y, moves[i].to_x, moves[i].to_y, &snap);

        double val = minimax(b, depth - 1, alpha, beta, !maximizing, opposite_color(color_to_move), NULL, ply_from_root + 1);

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
        score = minimax(b, depth, -INFINITY, INFINITY, 1, 'W', &best, 0);
    }
    else
    {
        score = minimax(b, depth, -INFINITY, INFINITY, 0, 'B', &best, 0);
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

int count_legal_moves(Board *b, char color)
{
    Move moves[256];
    int n = 0;
    collect_legal_moves(b, color, moves, &n);
    return n;
}

int adaptive_depth_by_moves(Board *b, char color)
{
    int n = count_legal_moves(b, color);
    if (n >= 40)
        return 4;
    else if (n >= 19)
        return 5;
    else if (n >= 8)
        return 6;
    else if (n >= 4)
        return 7;
    else
        return 8;
}

double phase_score(Board *b)
{
    const double val[128] = {['P'] = 1, ['N'] = 3, ['B'] = 3, ['R'] = 5, ['Q'] = 9};
    double total = 0;
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
            if (b->cells[i][j].state != 'E')
                total += val[(int)b->cells[i][j].piece];
    // Normalize: 78 = typical full material (both sides except kings)
    return total / 78.0; // 1.0 = opening, 0.0 = empty board (endgame)
}

int adaptive_depth_combined(Board *b, char color)
{
    int n = count_legal_moves(b, color);
    double phase = phase_score(b); // 1.0 opening → 0.0 endgame

    // Base depth determined by branching
    double base = 8.0 - log2(n + 1); // fewer moves → higher base
    // Adjust by phase: deeper search in endgames
    double depth = base + (1.5 * (1.0 - phase)); // up to +1.5 ply deeper

    // Clamp and round
    if (depth < 5)
        depth = 5;
    if (depth > 8)
        depth = 8;
    return (int)round(depth);
}