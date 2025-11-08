#include "board.h"
#include "move_gen.h"
#include "ai.h"
#include "util.h"

int main()
{
#ifdef _WIN32
    // Enable UTF-8 on Windows console
    SetConsoleOutputCP(CP_UTF8);
#endif

    Board board;
    board_init(&board);

    // pre-record initial position for threefold next side move ('W' begins)
    char init_key[512];
    position_key(&board, 'W', init_key, sizeof(init_key));
    history_increment(&board, init_key);
    
    // Ask player to choose color
    char player_color = 'W';
    printf("Welcome to Chess Engine!\n");
    printf("Choose your color (W for White, B for Black): ");
    char color_buf[64];
    if (input_line(color_buf, sizeof(color_buf)))
    {
        if (color_buf[0] == 'B' || color_buf[0] == 'b')
        {
            player_color = 'B';
            printf("You are playing as Black.\n");
        }
        else
        {
            player_color = 'W';
            printf("You are playing as White.\n");
        }
    }
    
    board_draw(&board, NULL, 0);
    
    // If player is Black, let AI play first
    if (player_color == 'B')
    {
        printf("AI (White) is making the first move...\n");
        int depth = adaptive_depth(&board);
        int status = engine(&board, 'W', depth);
        if (!status)
            return 0;
        board_draw(&board, NULL, 0);
    }

    while (1)
    {
        // ---------- Player's turn ----------

        if (board_is_checkmate(&board, player_color))
        {
            printf("Checkmate! %s wins.\n", (player_color == 'W') ? "Black" : "White");
            break;
        }
        if (board_is_stalemate(&board, player_color))
        {
            printf("Stalemate! Draw.\n");
            break;
        }
        if (board_is_in_check(&board, player_color))
        {
            printf("%s is in check!\n", (player_color == 'W') ? "White" : "Black");
        }

        // ask for a piece with at least one legal move
        while (1)
        {
            double ev = evaluate_board(&board, player_color);
            printf("Board evaluation of %s: %.2f\n", (player_color == 'W') ? "White" : "Black", ev);
            char ai_color = opposite_color(player_color);
            int depthW = adaptive_depth(&board);
            printf("[AI] Using adaptive depth = %d (moves: %d, phase:%.2f)\n",
                   depthW, count_legal_moves(&board, ai_color), phase_score(&board));
            printf("Select %s piece to move (e.g., e2): ", (player_color == 'W') ? "white" : "black");
            char buf[64];
            if (!input_line(buf, sizeof(buf)))
                return 0;
            int from_x, from_y;
            if (!parse_square(buf, &from_x, &from_y))
            {
                printf("Invalid input. Try again.\n");
                continue;
            }
            if (board.cells[from_x][from_y].state != player_color)
            {
                printf("That's not a %s piece. Try again.\n", (player_color == 'W') ? "white" : "black");
                continue;
            }
            Pos ps[64];
            int pn = 0, ln = 0;
            get_available_moves(&board, from_x, from_y, 1, ps, &pn);
            Pos leg[64];
            filter_legal_moves(&board, from_x, from_y, ps, pn, player_color, leg, &ln);
            if (ln == 0)
            {
                printf("No legal moves for that piece. Try another.\n");
#ifdef _WIN32
                Sleep(400);
#else
                struct timespec ts = {0, 400 * 1000000};
                nanosleep(&ts, NULL);
#endif
                continue;
            }

            // Show highlights
            board_draw(&board, leg, ln);
            printf("Select destination from highlighted options:\n");
            // list allowed
            char opts[64][3];
            int on = 0;
            for (int k = 0; k < ln; k++)
            {
                char sq[3];
                format_square(leg[k].x, leg[k].y, sq);
                strcpy(opts[on++], sq);
            }
            for (int k = 0; k < on; k++)
            {
                printf("%s", opts[k]);
                if (k < on - 1)
                    printf(" | ");
            }
            printf("\n");
            char dest[64];
            if (!input_line(dest, sizeof(dest)))
                return 0;

            int ok = 0;
            for (int k = 0; k < on; k++)
            {
                if (strcasecmp(dest, opts[k]) == 0)
                {
                    ok = 1;
                    break;
                }
            }
            if (!ok)
            {
                printf("Invalid move. Try again.\n");
#ifdef _WIN32
                Sleep(400);
#else
                struct timespec ts = {0, 400 * 1000000};
                nanosleep(&ts, NULL);
#endif
                continue;
            }
            int to_x, to_y;
            parse_square(dest, &to_x, &to_y);
            board_apply_move(&board, from_x, from_y, to_x, to_y);
            board_draw(&board, NULL, 0);
            break;
        }

        // ---------- AI's turn ----------
        char ai_color = opposite_color(player_color);
        int depth = adaptive_depth(&board);
        int status = engine(&board, ai_color, depth);
        if (!status)
            break;

        // Post-move checks on player
        board_draw(&board, NULL, 0);
        if (board_is_checkmate(&board, player_color))
        {
            printf("Checkmate! %s wins.\n", (player_color == 'W') ? "Black" : "White");
            break;
        }
        if (board_is_stalemate(&board, player_color))
        {
            printf("Stalemate! Draw.\n");
            break;
        }
        if (board_is_in_check(&board, player_color))
        {
            printf("%s is in check!\n", (player_color == 'W') ? "White" : "Black");
        }
    }

    return 0;
}
