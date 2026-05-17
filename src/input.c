#include "editor.h"
#include <string.h>
#include <ctype.h>

/* ---- 光标移动辅助函数 ---- */

static void cursor_move_up(void)
{
    if (g_editor.cur_row > 0)
    {
        g_editor.cur_row--;
        {
            LINE *line;
            line = &g_editor.lines[g_editor.cur_row];
            if (g_editor.cur_col > line->len)
            {
                g_editor.cur_col = line->len;
            }
        }
    }

    if (g_editor.cur_row < g_editor.top_row)
    {
        g_editor.top_row = g_editor.cur_row;
    }
}

static void cursor_move_down(void)
{
    if (g_editor.cur_row < g_editor.num_lines - 1)
    {
        g_editor.cur_row++;
        {
            LINE *line;
            line = &g_editor.lines[g_editor.cur_row];
            if (g_editor.cur_col > line->len)
            {
                g_editor.cur_col = line->len;
            }
        }
    }

    {
        int visible_rows;
        visible_rows = LINES - 2;
        if (g_editor.cur_row >= g_editor.top_row + visible_rows)
        {
            g_editor.top_row = g_editor.cur_row - visible_rows + 1;
        }
    }
}

static void cursor_move_left(void)
{
    if (g_editor.cur_col > 0)
    {
        LINE *line;
        line = &g_editor.lines[g_editor.cur_row];
        g_editor.cur_col = utf8_prev_char_start(line->data, g_editor.cur_col);
    }
    else if (g_editor.cur_row > 0)
    {
        g_editor.cur_row--;
        g_editor.cur_col = g_editor.lines[g_editor.cur_row].len;
    }
}

static void cursor_move_right(void)
{
    LINE *line;
    line = &g_editor.lines[g_editor.cur_row];

    if (g_editor.cur_col < line->len)
    {
        g_editor.cur_col += utf8_char_bytes(line->data, g_editor.cur_col);
    }
    else if (g_editor.cur_row < g_editor.num_lines - 1)
    {
        g_editor.cur_row++;
        g_editor.cur_col = 0;
    }
}

static void cursor_home(void)
{
    g_editor.cur_col = 0;
}

static void cursor_end(void)
{
    LINE *line;
    line = &g_editor.lines[g_editor.cur_row];
    g_editor.cur_col = line->len;
}

static void cursor_page_up(void)
{
    int page_size;
    int i;

    page_size = LINES - 2;
    if (page_size < 1)
    {
        page_size = 1;
    }

    for (i = 0; i < page_size && g_editor.cur_row > 0; i++)
    {
        g_editor.cur_row--;
    }

    g_editor.top_row -= page_size;
    if (g_editor.top_row < 0)
    {
        g_editor.top_row = 0;
    }
}

static void cursor_page_down(void)
{
    int page_size;
    int i;

    page_size = LINES - 2;
    if (page_size < 1)
    {
        page_size = 1;
    }

    for (i = 0; i < page_size && g_editor.cur_row < g_editor.num_lines - 1; i++)
    {
        g_editor.cur_row++;
    }

    g_editor.top_row += page_size;
    if (g_editor.top_row > g_editor.num_lines - 1)
    {
        g_editor.top_row = g_editor.num_lines - 1;
    }
    if (g_editor.top_row < 0)
    {
        g_editor.top_row = 0;
    }
}

/* ---- 打开文件对话框 ---- */

static void prompt_open_file(void)
{
    char input[FILENAME_SIZE];
    int pos;
    int done;
    wint_t wch;
    int prompt_width;

    curs_set(1);

    prompt_width = utf8_display_width("文件名: ");

    move(LINES - 1, 0);
    clrtoeol();
    mvaddstr(LINES - 1, 0, "文件名: ");
    refresh();

    pos = 0;
    done = 0;
    memset(input, 0, sizeof(input));

    while (!done)
    {
        int rc;
        rc = get_wch(&wch);

        if (rc == ERR)
        {
            continue;
        }

        if (wch == '\n' || wch == '\r')
        {
            done = 1;
        }
        else if (wch == 27)
        {
            input[0] = '\0';
            done = 1;
        }
        else if (wch == KEY_BACKSPACE || wch == 127 || wch == '\b')
        {
            if (pos > 0)
            {
                pos = utf8_prev_char_start(input, pos);
                input[pos] = '\0';

                /* 清空输入区并重新显示剩余内容 */
                move(LINES - 1, prompt_width);
                clrtoeol();
                addstr(input);
                move(LINES - 1, prompt_width + utf8_display_width(input));
                refresh();
            }
        }
        else if (wch >= 32 && pos < FILENAME_SIZE - 1)
        {
            char utf8[5];
            int bytes;
            int k;
            bytes = utf8_encode(wch, utf8);

            if (pos + bytes < FILENAME_SIZE)
            {
                for (k = 0; k < bytes; k++)
                {
                    input[pos++] = utf8[k];
                }
                input[pos] = '\0';

                /* 清空并重新显示整个输入区 */
                move(LINES - 1, prompt_width);
                clrtoeol();
                addstr(input);
                move(LINES - 1, prompt_width + utf8_display_width(input));
                refresh();
            }
        }
    }

    curs_set(1);

    if (input[0] != '\0')
    {
        if (file_read(input) == 0)
        {
            strncpy(g_editor.filename, input, FILENAME_SIZE - 1);
            g_editor.filename[FILENAME_SIZE - 1] = '\0';
            g_editor.has_filename = 1;
            g_editor.modified = 0;
            g_editor.cur_row = 0;
            g_editor.cur_col = 0;
            g_editor.top_row = 0;
            snprintf(g_editor.msg, MSG_SIZE, "已读取文件: %s", input);
        }
        else
        {
            snprintf(g_editor.msg, MSG_SIZE, "读取文件失败: %s", input);
        }
    }
    else
    {
        snprintf(g_editor.msg, MSG_SIZE, EDITOR_NAME " " EDITOR_VERSION " - ESC:菜单");
    }
}

/* ---- 主输入处理 ---- */

int input_process_key(wint_t ch)
{
    switch (ch)
    {
        /* ---- 导航键 ---- */
        case KEY_UP:
            cursor_move_up();
            break;

        case KEY_DOWN:
            cursor_move_down();
            break;

        case KEY_LEFT:
            cursor_move_left();
            break;

        case KEY_RIGHT:
            cursor_move_right();
            break;

        case KEY_HOME:
            cursor_home();
            break;

        case KEY_END:
            cursor_end();
            break;

        case KEY_PPAGE:
            cursor_page_up();
            break;

        case KEY_NPAGE:
            cursor_page_down();
            break;

        /* ---- 编辑键 ---- */
        case KEY_BACKSPACE:
        case 127:
        case '\b':
            buffer_delete_char();
            break;

        case KEY_DC:
            buffer_delete_char_forward();
            break;

        case '\n':
        case '\r':
        case KEY_ENTER:
            buffer_insert_newline();
            break;

        case '\t':
            buffer_insert_char(L'\t');
            break;

        /* ---- ESC 菜单 ---- */
        case 27:
            g_editor.menu_active = 1;
            g_editor.menu_sel = 0;
            break;

        /* ---- Ctrl 组合键 ---- */
        case 19:  /* Ctrl+S: 保存 */
            if (g_editor.has_filename)
            {
                if (file_write(g_editor.filename) == 0)
                {
                    snprintf(g_editor.msg, MSG_SIZE, "已保存: %s", g_editor.filename);
                    g_editor.modified = 0;
                }
                else
                {
                    snprintf(g_editor.msg, MSG_SIZE, "保存失败: %s", g_editor.filename);
                }
            }
            else
            {
                strncpy(g_editor.msg, "请使用菜单保存新文件", MSG_SIZE);
            }
            break;

        case 15:  /* Ctrl+O: 打开文件 */
            prompt_open_file();
            break;

        default:
            /* 可打印字符 (包括中文) */
            if (ch >= 32)
            {
                buffer_insert_char(ch);
            }
            break;
    }

    return 0;
}
