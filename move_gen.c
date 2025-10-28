#include "move_gen.h"

void append_pos(Pos *out, int *n, int x, int y)
{
    if (x >= 0 && x < 8 && y >= 0 && y < 8)
    {
        out[*n].x = x;
        out[*n].y = y;
        (*n)++;
    }
}

void get_available_moves(Board *b, int x, int y, int include_castling, Pos *out, int *out_count)
{
    *out_count = 0;
    Cell c = b->cells[x][y];
    char color = c.state;
    char piece = c.piece;
    if (piece == 0 || color == 'E')
        return;

    if (piece == 'P')
    {
        if (color == 'W')
        {
            if (x > 0 && b->cells[x - 1][y].state == 'E')
                append_pos(out, out_count, x - 1, y);
            if (x == 6 && b->cells[x - 2][y].state == 'E' && b->cells[x - 1][y].state == 'E')
                append_pos(out, out_count, x - 2, y);
            if (x > 0 && y > 0 && b->cells[x - 1][y - 1].state == 'B')
                append_pos(out, out_count, x - 1, y - 1);
            if (x > 0 && y < 7 && b->cells[x - 1][y + 1].state == 'B')
                append_pos(out, out_count, x - 1, y + 1);
        }
        else
        {
            if (x < 7 && b->cells[x + 1][y].state == 'E')
                append_pos(out, out_count, x + 1, y);
            if (x == 1 && b->cells[x + 2][y].state == 'E' && b->cells[x + 1][y].state == 'E')
                append_pos(out, out_count, x + 2, y);
            if (x < 7 && y > 0 && b->cells[x + 1][y - 1].state == 'W')
                append_pos(out, out_count, x + 1, y - 1);
            if (x < 7 && y < 7 && b->cells[x + 1][y + 1].state == 'W')
                append_pos(out, out_count, x + 1, y + 1);
        }
        return;
    }

    if (piece == 'R' || piece == 'Q')
    {
        int dirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
        for (int d = 0; d < 4; d++)
        {
            int dx = dirs[d][0], dy = dirs[d][1];
            int nx = x + dx, ny = y + dy;
            while (nx >= 0 && nx < 8 && ny >= 0 && ny < 8)
            {
                if (b->cells[nx][ny].state == 'E')
                {
                    append_pos(out, out_count, nx, ny);
                }
                else if (b->cells[nx][ny].state != color)
                {
                    append_pos(out, out_count, nx, ny);
                    break;
                }
                else
                    break;
                nx += dx;
                ny += dy;
            }
        }
        if (piece == 'R')
            goto maybe_castle;
    }

    if (piece == 'B' || piece == 'Q')
    {
        int dirs[4][2] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
        for (int d = 0; d < 4; d++)
        {
            int dx = dirs[d][0], dy = dirs[d][1];
            int nx = x + dx, ny = y + dy;
            while (nx >= 0 && nx < 8 && ny >= 0 && ny < 8)
            {
                if (b->cells[nx][ny].state == 'E')
                {
                    append_pos(out, out_count, nx, ny);
                }
                else if (b->cells[nx][ny].state != color)
                {
                    append_pos(out, out_count, nx, ny);
                    break;
                }
                else
                    break;
                nx += dx;
                ny += dy;
            }
        }
        if (piece == 'B')
            goto done_piece;
    }

    if (piece == 'N')
    {
        int jumps[8][2] = {{2, 1}, {2, -1}, {-2, 1}, {-2, -1}, {1, 2}, {1, -2}, {-1, 2}, {-1, -2}};
        for (int k = 0; k < 8; k++)
        {
            int nx = x + jumps[k][0], ny = y + jumps[k][1];
            if (nx >= 0 && nx < 8 && ny >= 0 && ny < 8)
            {
                char st = b->cells[nx][ny].state;
                if (st == 'E' || st != color)
                    append_pos(out, out_count, nx, ny);
            }
        }
        goto done_piece;
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
                {
                    char st = b->cells[nx][ny].state;
                    if (st == 'E' || st != color)
                        append_pos(out, out_count, nx, ny);
                }
            }
        }
    maybe_castle:
        if (include_castling)
        {
            if (board_can_castle(b, color, 'K'))
                append_pos(out, out_count, x, y + 2);
            if (board_can_castle(b, color, 'Q'))
                append_pos(out, out_count, x, y - 2);
        }
    }

done_piece:
    return;
}

void filter_legal_moves(Board *b, int x, int y, Pos *moves, int moves_count, char color, Pos *out, int *out_count)
{
    *out_count = 0;
    for (int i = 0; i < moves_count; i++)
    {
        int nx = moves[i].x, ny = moves[i].y;
        Cell saved_from = b->cells[x][y];
        Cell saved_to = b->cells[nx][ny];

        b->cells[nx][ny] = saved_from;
        b->cells[x][y] = (Cell){'E', 0};

        int ok = !board_is_in_check(b, color);

        b->cells[x][y] = saved_from;
        b->cells[nx][ny] = saved_to;

        if (ok)
        {
            out[*out_count] = (Pos){nx, ny};
            (*out_count)++;
        }
    }
}
