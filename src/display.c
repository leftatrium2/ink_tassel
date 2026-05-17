#include "editor.h"
#include <string.h>
#include <stdio.h>

/* ---- 内部光标位置（由 display_text_area 设置） ---- */
static int g_cursor_row = -1;
static int g_cursor_col = -1;

void display_status_bar(void)
{
    int max_x;
    int display_col;
    char status[COLS + 1];

    max_x = COLS;

    /* 计算光标的显示列（非字节偏移） */
    if (g_editor.cur_row >= 0 && g_editor.cur_row < g_editor.num_lines)
    {
        LINE *line = &g_editor.lines[g_editor.cur_row];
        display_col = utf8_byte_offset_to_display_col(
            line->data, g_editor.cur_col);
    }
    else
    {
        display_col = 0;
    }

    attrset(A_REVERSE);

    if (g_editor.has_filename)
    {
        snprintf(status, sizeof(status),
                 "文件: %s  行: %d/%d  列: %d %s",
                 g_editor.filename,
                 g_editor.cur_row + 1,
                 g_editor.num_lines,
                 display_col + 1,
                 g_editor.modified ? "[已修改]" : "");
    }
    else
    {
        snprintf(status, sizeof(status),
                 "文件: [未命名]  行: %d/%d  列: %d %s",
                 g_editor.cur_row + 1,
                 g_editor.num_lines,
                 display_col + 1,
                 g_editor.modified ? "[已修改]" : "");
    }

    /* 填充空白至行尾 */
    {
        int len = (int)strlen(status);
        while (len < max_x)
        {
            status[len++] = ' ';
        }
        status[len] = '\0';
    }

    mvaddstr(0, 0, status);
    attrset(A_NORMAL);
}

void display_text_area(void)
{
    int i;
    int screen_row;
    int max_y;

    max_y = LINES;

    g_cursor_row = -1;
    g_cursor_col = -1;

    for (i = 0; i < max_y - 2; i++)
    {
        screen_row = i + 1;
        move(screen_row, 0);
        clrtoeol();

        {
            int line_idx = g_editor.top_row + i;
            if (line_idx < g_editor.num_lines)
            {
                LINE *line = &g_editor.lines[line_idx];
                int cur_disp = 0;
                int start_disp = 0;
                int x = 0;

                /* 计算当前行的水平滚动偏移 */
                if (g_editor.cur_row == line_idx)
                {
                    int cursor_disp = utf8_byte_offset_to_display_col(
                        line->data, g_editor.cur_col);
                    if (cursor_disp >= COLS - 1)
                    {
                        start_disp = cursor_disp - COLS + 1;
                    }
                }

                /* 逐字符渲染 */
                while (x < line->len)
                {
                    unsigned char ch;
                    int char_disp;
                    int bytes;

                    ch = (unsigned char)line->data[x];

                    if (ch < 0x80)
                    {
                        bytes = 1;
                        char_disp = (ch == '\t') ? 4 : 1;
                    }
                    else
                    {
                        if ((ch & 0xE0) == 0xC0)
                            bytes = 2;
                        else if ((ch & 0xF0) == 0xE0)
                            bytes = 3;
                        else if ((ch & 0xF8) == 0xF0)
                            bytes = 4;
                        else
                            bytes = 1;
                        char_disp = 2;
                    }

                    if (cur_disp + char_disp <= start_disp)
                    {
                        cur_disp += char_disp;
                        x += bytes;
                        continue;
                    }

                    if (cur_disp < start_disp)
                    {
                        int visible = cur_disp + char_disp - start_disp;
                        int j;
                        for (j = 0; j < visible; j++)
                            addch(' ');
                        cur_disp += char_disp;
                        x += bytes;
                        continue;
                    }

                    if (cur_disp >= COLS)
                        break;

                    if (ch == '\t')
                    {
                        int j;
                        for (j = 0; j < 4; j++)
                        {
                            if (cur_disp + j >= COLS)
                                break;
                            addch(' ');
                        }
                    }
                    else if (bytes == 1)
                    {
                        addch(ch);
                    }
                    else
                    {
                        char buf[5];
                        int k;
                        for (k = 0; k < bytes; k++)
                            buf[k] = line->data[x + k];
                        buf[bytes] = '\0';
                        addstr(buf);
                    }

                    cur_disp += char_disp;
                    x += bytes;
                }

                /* 记录光标行位置 */
                if (line_idx == g_editor.cur_row)
                {
                    int cursor_disp = utf8_byte_offset_to_display_col(
                        line->data, g_editor.cur_col);
                    int line_start = 0;
                    if (cursor_disp >= COLS - 1)
                        line_start = cursor_disp - COLS + 1;
                    g_cursor_row = screen_row;
                    g_cursor_col = cursor_disp - line_start;
                    if (g_cursor_col >= COLS)
                        g_cursor_col = COLS - 1;
                }
            }
        }
    }
}

void display_message_line(void)
{
    int max_y;

    max_y = LINES;

    attrset(A_REVERSE);
    move(max_y - 1, 0);
    clrtoeol();
    mvaddstr(max_y - 1, 0, g_editor.msg);
    attrset(A_NORMAL);
}

void display_refresh(void)
{
    g_cursor_row = -1;
    g_cursor_col = -1;

    display_status_bar();

    if (g_editor.menu_active)
    {
        menu_draw();
    }
    else
    {
        display_text_area();
    }

    display_message_line();

    /* 最后放置光标，确保不被其他绘制覆盖 */
    if (g_cursor_row > 0)
    {
        move(g_cursor_row, g_cursor_col);
    }

    refresh();
}
