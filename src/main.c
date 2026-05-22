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

    /* 设置本地化以支持中文显示和输入 */
    setlocale(LC_ALL, "");

    /* ncurses 初始化 */
    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(1);
    set_escdelay(25);

    /* 编辑器初始化 */
    editor_init();

    /* 从命令行参数加载文件 */
    if (argc > 1)
    {
        if (file_read(argv[1]) == 0)
        {
            strncpy(g_editor.filename, argv[1], FILENAME_SIZE - 1);
            g_editor.filename[FILENAME_SIZE - 1] = '\0';
            g_editor.has_filename = 1;
        }
    }

    /* 主循环 */
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

            /* 终端尺寸变化: 钳制光标和滚动位置, 清屏后重绘 */
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

    /* 清理退出 */
    editor_cleanup();
    endwin();

    return 0;
}
