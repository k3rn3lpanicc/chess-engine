#include "board.h"
#include "move_gen.h"
#include <string.h>
#include <stdlib.h>

void board_init(Board *b)
{
    memset(b, 0, sizeof(*b));
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            b->cells[i][j].state = 'E';
            b->cells[i][j].piece = 0;
        }
    }
    b->castling_W_K = 1;
    b->castling_W_Q = 1;
    b->castling_B_K = 1;
    b->castling_B_Q = 1;
    b->history_size = 0;

    // Place pieces (same as Python create_empty_board)
    const char order[8] = {'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R'};
    for (int i = 0; i < 8; i++)
    {
        // Black back + pawns
        board_set_piece(b, 0, i, order[i], 'B');
        board_set_piece(b, 1, i, 'P', 'B');
        // White back + pawns
        board_set_piece(b, 7, i, order[i], 'W');
        board_set_piece(b, 6, i, 'P', 'W');
    }
}

void board_set_piece(Board *b, int x, int y, char piece, char color)
{
    b->cells[x][y].piece = piece;
    b->cells[x][y].state = color;
}

int board_find_king(Board *b, char color, int *outx, int *outy)
{
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            Cell c = b->cells[i][j];
            if (c.piece == 'K' && c.state == color)
            {
                *outx = i;
                *outy = j;
                return 1;
            }
        }
    }
    return 0;
}

int board_is_in_check(Board *b, char color)
{
    int kx, ky;
    if (!board_find_king(b, color, &kx, &ky))
        return 0;
    char enemy = opposite_color(color);
    Pos att[64];
    int att_n = 0;
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (b->cells[i][j].state == enemy)
            {
                // include_castling = 0 for checks
                Pos tmp[64];
                int tn = 0;
                get_available_moves(b, i, j, 0, tmp, &tn);
                for (int t = 0; t < tn; t++)
                {
                    if (tmp[t].x == kx && tmp[t].y == ky)
                        return 1;
                }
            }
        }
    }
    return 0;
}

int board_is_checkmate(Board *b, char color)
{
    if (!board_is_in_check(b, color))
        return 0;
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
            if (ln > 0)
                return 0;
        }
    }
    return 1;
}

int board_is_stalemate(Board *b, char color)
{
    if (board_is_in_check(b, color))
        return 0;
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
            if (ln > 0)
                return 0;
        }
    }
    return 1;
}

int board_can_castle(Board *b, char color, char side)
{
    int row = (color == 'W') ? 7 : 0;
    int king_y = 4;
    int rook_y = (side == 'K') ? 7 : 0;

    // Check rights
    if (color == 'W')
    {
        if (side == 'K' && !b->castling_W_K)
            return 0;
        if (side == 'Q' && !b->castling_W_Q)
            return 0;
    }
    else
    {
        if (side == 'K' && !b->castling_B_K)
            return 0;
        if (side == 'Q' && !b->castling_B_Q)
            return 0;
    }

    // Rook present
    Cell rook = b->cells[row][rook_y];
    if (!(rook.piece == 'R' && rook.state == color))
        return 0;

    // Empty between & pass-through not in check
    int empty_ok = 1;
    if (side == 'K')
    {
        int squares[2][2] = {{row, 5}, {row, 6}};
        for (int k = 0; k < 2; k++)
        {
            if (b->cells[squares[k][0]][squares[k][1]].state != 'E')
            {
                empty_ok = 0;
                break;
            }
        }
        if (!empty_ok)
            return 0;
        if (board_is_in_check(b, color))
            return 0;
        // simulate king pass squares
        for (int k = 0; k < 2; k++)
        {
            Cell saved_from = b->cells[row][4];
            Cell saved_to = b->cells[squares[k][0]][squares[k][1]];
            b->cells[squares[k][0]][squares[k][1]] = (Cell){color, 'K'};
            b->cells[row][4] = (Cell){'E', 0};
            int under = board_is_in_check(b, color);
            b->cells[row][4] = saved_from;
            b->cells[squares[k][0]][squares[k][1]] = saved_to;
            if (under)
                return 0;
        }
    }
    else
    { // Queenside
        int sqs[3][2] = {{row, 3}, {row, 2}, {row, 1}};
        for (int k = 0; k < 3; k++)
        {
            if (b->cells[sqs[k][0]][sqs[k][1]].state != 'E')
            {
                empty_ok = 0;
                break;
            }
        }
        if (!empty_ok)
            return 0;
        if (board_is_in_check(b, color))
            return 0;
        int path[2][2] = {{row, 3}, {row, 2}};
        for (int k = 0; k < 2; k++)
        {
            Cell saved_from = b->cells[row][4];
            Cell saved_to = b->cells[path[k][0]][path[k][1]];
            b->cells[path[k][0]][path[k][1]] = (Cell){color, 'K'};
            b->cells[row][4] = (Cell){'E', 0};
            int under = board_is_in_check(b, color);
            b->cells[row][4] = saved_from;
            b->cells[path[k][0]][path[k][1]] = saved_to;
            if (under)
                return 0;
        }
    }
    return 1;
}

void board_apply_move(Board *b, int from_x, int from_y, int to_x, int to_y)
{
    Cell moving = b->cells[from_x][from_y];
    char piece = moving.piece;
    char color = moving.state;

    // Castling move if king moves 2 columns
    if (piece == 'K' && abs(to_y - from_y) == 2)
    {
        // move rook
        int row = from_x;
        if (to_y > from_y)
        {
            // kingside
            b->cells[row][5] = b->cells[row][7];
            b->cells[row][7] = (Cell){'E', 0};
        }
        else
        {
            // queenside
            b->cells[row][3] = b->cells[row][0];
            b->cells[row][0] = (Cell){'E', 0};
        }
        // move king
        b->cells[to_x][to_y] = moving;
        b->cells[from_x][from_y] = (Cell){'E', 0};

        // revoke rights
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
    else
    {
        // Regular move
        b->cells[to_x][to_y] = moving;
        b->cells[from_x][from_y] = (Cell){'E', 0};

        // Update castling rights if king or rook moved
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
                if (from_x == 7 && from_y == 0)
                    b->castling_W_Q = 0;
                else if (from_x == 7 && from_y == 7)
                    b->castling_W_K = 0;
            }
            else
            {
                if (from_x == 0 && from_y == 0)
                    b->castling_B_Q = 0;
                else if (from_x == 0 && from_y == 7)
                    b->castling_B_K = 0;
            }
        }
    }

    if (piece == 'P')
    {
        if ((color == 'W' && to_x == 0) || (color == 'B' && to_x == 7))
        {
            b->cells[to_x][to_y].piece = 'Q';
        }
    }

    // Update repetition table with next side to move
    char next = opposite_color(color);
    char key[512];
    position_key(b, next, key, sizeof(key));
    history_increment(b, key);
}

void get_attack_squares(Board *b, int x, int y, Pos *out, int *out_count)
{
    *out_count = 0;
    Cell c = b->cells[x][y];
    char color = c.state;
    char piece = c.piece;
    if (piece == 0 || color == 'E')
        return;

    if (piece == 'P')
    {
        int dir = (color == 'W') ? -1 : 1;
        int nx = x + dir, ny = y - 1;
        if (nx >= 0 && nx < 8 && ny >= 0 && ny < 8)
            append_pos(out, out_count, nx, ny);
        ny = y + 1;
        if (nx >= 0 && nx < 8 && ny >= 0 && ny < 8)
            append_pos(out, out_count, nx, ny);
        return;
    }
    if (piece == 'N')
    {
        int jumps[8][2] = {{2, 1}, {2, -1}, {-2, 1}, {-2, -1}, {1, 2}, {1, -2}, {-1, 2}, {-1, -2}};
        for (int k = 0; k < 8; k++)
        {
            int nx = x + jumps[k][0], ny = y + jumps[k][1];
            if (nx >= 0 && nx < 8 && ny >= 0 && ny < 8)
                append_pos(out, out_count, nx, ny);
        }
        return;
    }
    if (piece == 'B' || piece == 'Q')
    {
        int dirs[4][2] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
        for (int d = 0; d < 4; d++)
        {
            int nx = x + dirs[d][0], ny = y + dirs[d][1];
            while (nx >= 0 && nx < 8 && ny >= 0 && ny < 8)
            {
                append_pos(out, out_count, nx, ny);
                if (b->cells[nx][ny].state != 'E')
                    break;
                nx += dirs[d][0];
                ny += dirs[d][1];
            }
        }
        if (piece == 'B')
            return;
    }
    if (piece == 'R' || piece == 'Q')
    {
        int dirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
        for (int d = 0; d < 4; d++)
        {
            int nx = x + dirs[d][0], ny = y + dirs[d][1];
            while (nx >= 0 && nx < 8 && ny >= 0 && ny < 8)
            {
                append_pos(out, out_count, nx, ny);
                if (b->cells[nx][ny].state != 'E')
                    break;
                nx += dirs[d][0];
                ny += dirs[d][1];
            }
        }
        if (piece == 'R')
            return;
    }
    if (piece == 'K')
    {
        for (int dx = -1; dx <= 1; dx++)
        {
            for (int dy = -1; dy <= 1; dy++)
            {
                if (dx == 0 && dy == 0)
                    continue;
                int nx = x + dx, ny = y + dy;
                if (nx >= 0 && nx < 8 && ny >= 0 && ny < 8)
                    append_pos(out, out_count, nx, ny);
            }
        }
        return;
    }
}

int count_pieces(Board *b)
{
    int count = 0;
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (b->cells[i][j].state != 'E')
                count++;
        }
    }
    return count;
}

int adaptive_depth(Board *b)
{
    int pieces = count_pieces(b);
    if (pieces >= 26)
        return 5;
    else if (pieces >= 18)
        return 6;
    else if (pieces >= 10)
        return 7;
    else if (pieces >= 7)
        return 8;
    else
        return 9;
}