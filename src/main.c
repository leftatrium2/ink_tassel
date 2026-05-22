#include "editor.h"
#include <locale.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    wint_t ch;
    int rc;

    (void)argc;
    (void)argv;

    /* Set locale for CJK display and input */
    setlocale(LC_ALL, "");

    /* ncurses initialization */
    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(1);
    set_escdelay(25);

    /* Editor initialization */
    editor_init();

    /* Load file from command line argument */
    if (argc > 1)
    {
        if (file_read(argv[1]) == 0)
        {
            strncpy(g_editor.filename, argv[1], FILENAME_SIZE - 1);
            g_editor.filename[FILENAME_SIZE - 1] = '\0';
            g_editor.has_filename = 1;
        }
    }

    /* Main loop */
    while (g_editor.running)
    {
        display_refresh();

        rc = get_wch(&ch);

        if (rc == ERR)
        {
            continue;
        }

        if (ch == KEY_RESIZE)
        {
            int visible_rows;

            /* Terminal resize: clamp cursor and scroll position, redraw */
            visible_rows = LINES - 2;
            if (visible_rows < 1)
            {
                visible_rows = 1;
            }

            if (g_editor.cur_row >= g_editor.num_lines)
            {
                g_editor.cur_row = g_editor.num_lines - 1;
            }
            if (g_editor.cur_row < 0)
            {
                g_editor.cur_row = 0;
            }

            if (g_editor.cur_row < g_editor.top_row)
            {
                g_editor.top_row = g_editor.cur_row;
            }
            if (g_editor.cur_row >= g_editor.top_row + visible_rows)
            {
                g_editor.top_row = g_editor.cur_row - visible_rows + 1;
            }
            if (g_editor.top_row < 0)
            {
                g_editor.top_row = 0;
            }

            clear();
            continue;
        }

        if (g_editor.menu_active)
        {
            menu_handle_key((int)ch);
        }
        else
        {
            input_process_key(ch);
        }
    }

    /* Cleanup and exit */
    editor_cleanup();
    endwin();

    return 0;
}
