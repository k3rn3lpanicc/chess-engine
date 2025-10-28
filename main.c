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

    while (1)
    {
        // ---------- White (human) ----------
        board_draw(&board, NULL, 0);

        if (board_is_checkmate(&board, 'W'))
        {
            printf("Checkmate! Black wins.\n");
            break;
        }
        if (board_is_stalemate(&board, 'W'))
        {
            printf("Stalemate! Draw.\n");
            break;
        }
        if (board_is_in_check(&board, 'W'))
        {
            printf("White is in check!\n");
        }

        // ask for a white piece with at least one legal move
        while (1)
        {
            double ev = evaluate_board(&board, 'W');
            printf("Board evaluation of White: %.2f\n", ev);
            printf("Select white piece to move (e.g., e2): ");
            char buf[64];
            if (!input_line(buf, sizeof(buf)))
                return 0;
            int from_x, from_y;
            if (!parse_square(buf, &from_x, &from_y))
            {
                printf("Invalid input. Try again.\n");
                continue;
            }
            if (board.cells[from_x][from_y].state != 'W')
            {
                printf("That's not a white piece. Try again.\n");
                continue;
            }
            Pos ps[64];
            int pn = 0, ln = 0;
            get_available_moves(&board, from_x, from_y, 1, ps, &pn);
            Pos leg[64];
            filter_legal_moves(&board, from_x, from_y, ps, pn, 'W', leg, &ln);
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

        // ---------- Black (AI) ----------
        int status = engine(&board, 'B', 5);
        if (!status)
            break;

        // Post-move checks on White
        board_draw(&board, NULL, 0);
        if (board_is_checkmate(&board, 'W'))
        {
            printf("Checkmate! Black wins.\n");
            break;
        }
        if (board_is_stalemate(&board, 'W'))
        {
            printf("Stalemate! Draw.\n");
            break;
        }
        if (board_is_in_check(&board, 'W'))
        {
            printf("White is in check!\n");
        }

#ifdef _WIN32
        Sleep(100);
#else
        struct timespec ts = {0, 100 * 1000000};
        nanosleep(&ts, NULL);
#endif
    }

    return 0;
}
