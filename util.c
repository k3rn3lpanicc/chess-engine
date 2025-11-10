#include "util.h"
#include <stdio.h>
#include <string.h>

void position_key(Board *b, char color_to_move, char *out, size_t out_sz)
{
    // Serialize board + side + castling flags
    char *p = out;
    size_t rem = out_sz;
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            Cell c = b->cells[i][j];
            char color = (c.state == 'W' || c.state == 'B') ? c.state : '.';
            char piece = c.piece ? c.piece : '.';
            int n = snprintf(p, rem, "%c%c", color, piece);
            p += n;
            rem -= (size_t)n;
            if (rem <= 1)
                goto finish;
        }
    }
    snprintf(p, rem, "|%c|W%sQ%sB%sq%s",
             color_to_move,
             b->castling_W_K ? "K" : "-",
             b->castling_W_Q ? "Q" : "-",
             b->castling_B_K ? "K" : "-",
             b->castling_B_Q ? "Q" : "-");
finish:
    out[out_sz - 1] = 0;
}

void history_increment(Board *b, const char *key)
{
    for (int i = 0; i < b->history_size; i++)
    {
        if (strcmp(b->history[i].key, key) == 0)
        {
            b->history[i].count += 1;
            return;
        }
    }
    if (b->history_size < (int)(sizeof(b->history) / sizeof(b->history[0])))
    {
        strncpy(b->history[b->history_size].key, key, sizeof(b->history[0].key) - 1);
        b->history[b->history_size].key[sizeof(b->history[0].key) - 1] = 0;
        b->history[b->history_size].count = 1;
        b->history_size++;
    }
}

char piece_symbol(char piece, char color)
{
    // Unicode chess glyphs
    if (color == 'W')
    {
        switch (piece)
        {
        case 'P':
            return 'P'; // we will print Unicode via string for alignment in draw
        case 'R':
            return 'R';
        case 'N':
            return 'N';
        case 'B':
            return 'B';
        case 'Q':
            return 'Q';
        case 'K':
            return 'K';
        }
    }
    else if (color == 'B')
    {
        switch (piece)
        {
        case 'P':
            return 'p';
        case 'R':
            return 'r';
        case 'N':
            return 'n';
        case 'B':
            return 'b';
        case 'Q':
            return 'q';
        case 'K':
            return 'k';
        }
    }
    return ' ';
}

const char *piece_unicode(char piece, char color)
{
    // Use UTF-8 glyphs to match Python output feel
    if (color == 'W')
    {
        switch (piece)
        {
        case 'P':
            return "♙";
        case 'R':
            return "♖";
        case 'N':
            return "♘";
        case 'B':
            return "♗";
        case 'Q':
            return "♕";
        case 'K':
            return "♔";
        }
    }
    else if (color == 'B')
    {
        switch (piece)
        {
        case 'P':
            return "♟";
        case 'R':
            return "♜";
        case 'N':
            return "♞";
        case 'B':
            return "♝";
        case 'Q':
            return "♛";
        case 'K':
            return "♚";
        }
    }
    return " ";
}

int pos_in_list(Pos *list, int n, int x, int y)
{
    for (int i = 0; i < n; i++)
    {
        if (list[i].x == x && list[i].y == y)
            return 1;
    }
    return 0;
}

// ===================== Drawing =====================

void board_draw(Board *b, Pos *highlights, int n_highlights)
{
    clear_console();

    // Top border
    printf("\n   ╔══════════════════════════════════════╗\n");
    printf("   ║    a   b   c   d   e   f   g   h     ║\n");
    printf("   ╠══════════════════════════════════════╣\n");

    for (int i = 0; i < 8; i++)
    {
        printf("   ║ %d ", 8 - i);

        for (int j = 0; j < 8; j++)
        {
            Cell c = b->cells[i][j];
            int hl = pos_in_list(highlights, n_highlights, i, j);

            // Determine if square is light or dark
            int is_light_square = (i + j) % 2 == 0;

#ifdef _WIN32
            HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
            CONSOLE_SCREEN_BUFFER_INFO info;
            GetConsoleScreenBufferInfo(h, &info);
            WORD orig = info.wAttributes;

            WORD bg_color;
            if (hl)
            {
                // Yellow/gold background for highlights
                bg_color = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY;
            }
            else if (is_light_square)
            {
                // White/light gray background
                bg_color = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY;
            }
            else
            {
                // Blue/dark background
                bg_color = BACKGROUND_BLUE;
            }

            // Set text color for good contrast
            WORD fg_color;
            if (c.state == 'W')
            {
                if (is_light_square)
                {
                    // White piece on light square: medium gray for contrast
                    fg_color = FOREGROUND_INTENSITY;
                }
                else
                {
                    // White piece on dark square: bright white
                    fg_color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
                }
            }
            else if (c.state == 'B')
            {
                if (is_light_square)
                {
                    // Black piece on light square: black
                    fg_color = 0;
                }
                else
                {
                    // Black piece on dark square: light gray for contrast
                    fg_color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
                }
            }
            else
            {
                // Empty square: use square color for text (won't be visible anyway)
                fg_color = is_light_square ? 0 : FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
            }

            SetConsoleTextAttribute(h, bg_color | fg_color);

            // 4 characters wide to balance with 2 line height (roughly square)
            if (c.state == 'E')
                printf("    ");
            else
                printf("  %s ", piece_unicode(c.piece, c.state));

            SetConsoleTextAttribute(h, orig);
#else
            // ANSI escape codes for Unix/Linux
            // Set background color
            if (hl)
            {
                // Yellow background for highlights
                printf("\x1b[48;5;226m"); // yellow bg
            }
            else if (is_light_square)
            {
                // White/light gray background
                printf("\x1b[48;5;255m"); // white bg
            }
            else
            {
                // Blue/dark background
                printf("\x1b[48;5;18m"); // dark blue bg
            }

            // Set text color for good contrast
            if (c.state == 'W')
            {
                if (is_light_square)
                {
                    // White piece on light square: medium gray
                    printf("\x1b[38;5;240m");
                }
                else
                {
                    // White piece on dark square: bright white
                    printf("\x1b[38;5;231m");
                }
            }
            else if (c.state == 'B')
            {
                if (is_light_square)
                {
                    // Black piece on light square: black
                    printf("\x1b[38;5;16m");
                }
                else
                {
                    // Black piece on dark square: light gray
                    printf("\x1b[38;5;250m");
                }
            }

            // 4 characters wide to balance with 2 line height (roughly square)
            if (c.state == 'E')
                printf("    ");
            else
                printf("  %s ", piece_unicode(c.piece, c.state));

            printf("\x1b[0m"); // Reset colors
#endif
        }

        printf(" %d ║\n", 8 - i);

        // Second line for each row (empty) to make squares taller
        printf("   ║   ");
        for (int j = 0; j < 8; j++)
        {
            int is_light_square = (i + j) % 2 == 0;
            int hl = pos_in_list(highlights, n_highlights, i, j);

#ifdef _WIN32
            HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
            CONSOLE_SCREEN_BUFFER_INFO info;
            GetConsoleScreenBufferInfo(h, &info);
            WORD orig = info.wAttributes;

            WORD bg_color;
            if (hl)
                bg_color = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY;
            else if (is_light_square)
                bg_color = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY;
            else
                bg_color = BACKGROUND_BLUE;

            SetConsoleTextAttribute(h, bg_color);
            printf("    ");
            SetConsoleTextAttribute(h, orig);
#else
            if (hl)
                printf("\x1b[48;5;226m");
            else if (is_light_square)
                printf("\x1b[48;5;255m");
            else
                printf("\x1b[48;5;18m");

            printf("    ");
            printf("\x1b[0m");
#endif
        }
        printf("   ║\n");
    }

    // Bottom border
    printf("   ╠══════════════════════════════════════╣\n");
    printf("   ║    a   b   c   d   e   f   g   h     ║\n");
    printf("   ╚══════════════════════════════════════╝\n\n");
}

// ===================== Main (CLI) =====================

int parse_square(const char *s, int *out_x, int *out_y)
{
    // expects like "e2"
    if (!s || strlen(s) < 2)
        return 0;
    char file = tolower((unsigned char)s[0]);
    char rank = s[1];
    if (file < 'a' || file > 'h')
        return 0;
    if (rank < '1' || rank > '8')
        return 0;
    *out_y = file - 'a';
    *out_x = 8 - (rank - '0');
    if (*out_x < 0 || *out_x > 7 || *out_y < 0 || *out_y > 7)
        return 0;
    return 1;
}

void format_square(int x, int y, char *out)
{
    out[0] = 'a' + y;
    out[1] = '0' + (8 - x);
    out[2] = 0;
}

int input_line(char *buf, size_t n)
{
    if (!fgets(buf, (int)n, stdin))
        return 0;
    size_t L = strlen(buf);
    if (L > 0 && (buf[L - 1] == '\n' || buf[L - 1] == '\r'))
        buf[L - 1] = 0;
    return 1;
}

void clear_console()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}