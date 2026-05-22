#include "editor.h"
#include <stdlib.h>
#include <string.h>

void buffer_init(void)
{
    g_editor.lines = (LINE *)malloc(sizeof(LINE) * LINE_CAP_INIT);
    g_editor.cap_lines = LINE_CAP_INIT;
    g_editor.num_lines = 0;
    g_editor.cur_row = 0;
    g_editor.cur_col = 0;
    g_editor.top_row = 0;

    buffer_ensure_line();
}

void buffer_free(void)
{
    int i;
    for (i = 0; i < g_editor.num_lines; i++)
    {
        free(g_editor.lines[i].data);
    }
    free(g_editor.lines);
    g_editor.lines = NULL;
    g_editor.num_lines = 0;
    g_editor.cap_lines = 0;
}

void buffer_ensure_line(void)
{
    if (g_editor.num_lines == 0)
    {
        LINE *line;
        g_editor.num_lines = 1;
        line = &g_editor.lines[0];
        line->size = LINE_DATA_INIT;
        line->data = (char *)malloc((size_t)line->size);
        line->data[0] = '\0';
        line->len = 0;
    }
}

static void line_ensure_capacity(LINE *line, int needed)
{
    if (needed >= line->size)
    {
        int new_size;
        new_size = needed + LINE_DATA_STEP;
        line->data = (char *)realloc(line->data, (size_t)new_size);
        line->size = new_size;
    }
}

static int buffer_expand_lines(void)
{
    if (g_editor.num_lines >= MAX_LINES)
    {
        return -1;
    }

    if (g_editor.num_lines >= g_editor.cap_lines)
    {
        int new_cap;
        LINE *new_lines;
        new_cap = g_editor.cap_lines + LINE_CAP_STEP;
        new_lines = (LINE *)realloc(g_editor.lines, sizeof(LINE) * (size_t)new_cap);
        if (new_lines == NULL)
        {
            return -1;
        }
        g_editor.lines = new_lines;
        g_editor.cap_lines = new_cap;
    }
    return 0;
}

int buffer_insert_char(wint_t ch)
{
    LINE *line;
    int i;
    char utf8[5];
    int bytes;

    buffer_ensure_line();

    if (g_editor.cur_row >= g_editor.num_lines)
    {
        g_editor.cur_row = g_editor.num_lines - 1;
    }

    /* Encode wide character to UTF-8 */
    bytes = utf8_encode(ch, utf8);

    line = &g_editor.lines[g_editor.cur_row];
    line_ensure_capacity(line, line->len + bytes);

    /* Shift characters after insertion point */
    for (i = line->len; i > g_editor.cur_col; i--)
    {
        line->data[i + bytes - 1] = line->data[i - 1];
    }

    /* Insert UTF-8 bytes */
    for (i = 0; i < bytes; i++)
    {
        line->data[g_editor.cur_col + i] = utf8[i];
    }

    line->len += bytes;
    line->data[line->len] = '\0';

    g_editor.cur_col += bytes;
    g_editor.modified = 1;
    return 0;
}

int buffer_delete_char(void)
{
    LINE *line;
    int i;
    int prev_start;
    int char_bytes;

    /* If cursor is at line start and not first line, merge with previous line */
    if (g_editor.cur_col == 0 && g_editor.cur_row > 0)
    {
        LINE *prev_line;
        int prev_len;

        prev_line = &g_editor.lines[g_editor.cur_row - 1];
        line = &g_editor.lines[g_editor.cur_row];
        prev_len = prev_line->len;

        line_ensure_capacity(prev_line, prev_len + line->len + 1);

        memcpy(prev_line->data + prev_len, line->data, (size_t)line->len);
        prev_line->len += line->len;
        prev_line->data[prev_line->len] = '\0';

        buffer_delete_line_at(g_editor.cur_row);
        g_editor.cur_row--;
        g_editor.cur_col = prev_len;
        g_editor.modified = 1;
        return 0;
    }

    /* Delete character before cursor within line (multibyte-aware) */
    if (g_editor.cur_col > 0)
    {
        line = &g_editor.lines[g_editor.cur_row];
        prev_start = utf8_prev_char_start(line->data, g_editor.cur_col);
        char_bytes = g_editor.cur_col - prev_start;

        /* Shift subsequent characters forward */
        for (i = prev_start; i < line->len - char_bytes; i++)
        {
            line->data[i] = line->data[i + char_bytes];
        }

        line->len -= char_bytes;
        line->data[line->len] = '\0';
        g_editor.cur_col = prev_start;
        g_editor.modified = 1;
    }

    return 0;
}

int buffer_delete_char_forward(void)
{
    LINE *line;
    int char_bytes;

    buffer_ensure_line();

    if (g_editor.cur_row >= g_editor.num_lines)
    {
        return 0;
    }

    line = &g_editor.lines[g_editor.cur_row];

    /* Cursor at end of line: merge next line into current */
    if (g_editor.cur_col >= line->len && g_editor.cur_row + 1 < g_editor.num_lines)
    {
        LINE *next_line;
        next_line = &g_editor.lines[g_editor.cur_row + 1];
        line_ensure_capacity(line, line->len + next_line->len + 1);
        memcpy(line->data + line->len, next_line->data, (size_t)next_line->len);
        line->len += next_line->len;
        line->data[line->len] = '\0';
        buffer_delete_line_at(g_editor.cur_row + 1);
        g_editor.modified = 1;
        return 0;
    }

    /* Delete character at cursor (multibyte-aware) */
    if (g_editor.cur_col < line->len)
    {
        int i;
        char_bytes = utf8_char_bytes(line->data, g_editor.cur_col);

        for (i = g_editor.cur_col; i < line->len - char_bytes; i++)
        {
            line->data[i] = line->data[i + char_bytes];
        }

        line->len -= char_bytes;
        g_editor.modified = 1;
    }

    return 0;
}

int buffer_insert_newline(void)
{
    LINE *line;
    LINE *new_line;
    int i;
    char *remainder;
    int remain_len;

    buffer_ensure_line();

    if (g_editor.cur_row >= g_editor.num_lines)
    {
        g_editor.cur_row = g_editor.num_lines - 1;
    }

    line = &g_editor.lines[g_editor.cur_row];

    if (buffer_expand_lines() != 0)
    {
        return -1;
    }

    /* Shift lines after current line down */
    for (i = g_editor.num_lines; i > g_editor.cur_row + 1; i--)
    {
        g_editor.lines[i] = g_editor.lines[i - 1];
    }

    g_editor.num_lines++;

    /* Initialize new line */
    {
        int new_idx = g_editor.cur_row + 1;
        g_editor.lines[new_idx].size = LINE_DATA_INIT;
        g_editor.lines[new_idx].data = (char *)malloc(LINE_DATA_INIT);
        g_editor.lines[new_idx].len = 0;
        g_editor.lines[new_idx].data[0] = '\0';
    }

    /* Move content after cursor to new line */
    remain_len = line->len - g_editor.cur_col;
    if (remain_len > 0)
    {
        remainder = (char *)malloc((size_t)(remain_len + 1));
        memcpy(remainder, line->data + g_editor.cur_col, (size_t)remain_len);
        remainder[remain_len] = '\0';

        {
            LINE *nl = &g_editor.lines[g_editor.cur_row + 1];
            line_ensure_capacity(nl, remain_len + 1);
            memcpy(nl->data, remainder, (size_t)(remain_len + 1));
            nl->len = remain_len;
        }

        free(remainder);
    }

    line->data[g_editor.cur_col] = '\0';
    line->len = g_editor.cur_col;

    g_editor.cur_row++;
    g_editor.cur_col = 0;
    g_editor.modified = 1;

    return 0;
}

int buffer_delete_line_at(int row)
{
    int i;

    if (g_editor.num_lines <= 1)
    {
        LINE *line;
        line = &g_editor.lines[0];
        line->len = 0;
        line->data[0] = '\0';
        g_editor.cur_col = 0;
        g_editor.modified = 1;
        return 0;
    }

    if (row < 0 || row >= g_editor.num_lines)
    {
        return -1;
    }

    free(g_editor.lines[row].data);

    for (i = row; i < g_editor.num_lines - 1; i++)
    {
        g_editor.lines[i] = g_editor.lines[i + 1];
    }

    g_editor.num_lines--;

    if (g_editor.cur_row >= g_editor.num_lines)
    {
        g_editor.cur_row = g_editor.num_lines - 1;
    }

    g_editor.modified = 1;
    return 0;
}

/* ---- UTF-8 helper functions ---- */

int utf8_char_bytes(const char *s, int pos)
{
    unsigned char c;
    c = (unsigned char)s[pos];

    if (c < 0x80)
    {
        return 1;
    }
    if ((c & 0xE0) == 0xC0)
    {
        return 2;
    }
    if ((c & 0xF0) == 0xE0)
    {
        return 3;
    }
    if ((c & 0xF8) == 0xF0)
    {
        return 4;
    }
    return 1;
}

int utf8_prev_char_start(const char *s, int pos)
{
    if (pos <= 0)
    {
        return 0;
    }

    pos--;

    /* Skip UTF-8 continuation bytes (10xxxxxx) */
    while (pos > 0 && ((unsigned char)s[pos] & 0xC0) == 0x80)
    {
        pos--;
    }

    return pos;
}

int utf8_display_width(const char *s)
{
    int width;
    int i;

    width = 0;
    i = 0;

    while (s[i] != '\0')
    {
        unsigned char c;
        c = (unsigned char)s[i];

        if (c < 0x80)
        {
            width++;
            i++;
        }
        else
        {
            int skip;

            if ((c & 0xE0) == 0xC0)
            {
                skip = 2;
            }
            else if ((c & 0xF0) == 0xE0)
            {
                skip = 3;
            }
            else if ((c & 0xF8) == 0xF0)
            {
                skip = 4;
            }
            else
            {
                skip = 1;
            }

            width += 2;
            i += skip;
        }
    }

    return width;
}

int utf8_byte_offset_to_display_col(const char *s, int byte_offset)
{
    int width;
    int pos;

    width = 0;
    pos = 0;

    while (pos < byte_offset && s[pos] != '\0')
    {
        unsigned char c;
        int bytes;

        c = (unsigned char)s[pos];

        if (c < 0x80)
        {
            if (c == '\t')
            {
                width = (width / 4 + 1) * 4;
            }
            else
            {
                width += 1;
            }
            pos += 1;
        }
        else
        {
            if ((c & 0xE0) == 0xC0)
            {
                bytes = 2;
            }
            else if ((c & 0xF0) == 0xE0)
            {
                bytes = 3;
            }
            else if ((c & 0xF8) == 0xF0)
            {
                bytes = 4;
            }
            else
            {
                bytes = 1;
            }

            width += 2;
            pos += bytes;
        }
    }

    return width;
}

int utf8_encode(wint_t wch, char *out)
{
    if (wch < 0)
    {
        out[0] = '\0';
        return 0;
    }

    if (wch < 0x80)
    {
        out[0] = (char)wch;
        out[1] = '\0';
        return 1;
    }
    if (wch < 0x800)
    {
        out[0] = (char)(0xC0 | (wch >> 6));
        out[1] = (char)(0x80 | (wch & 0x3F));
        out[2] = '\0';
        return 2;
    }
    if (wch < 0x10000)
    {
        out[0] = (char)(0xE0 | (wch >> 12));
        out[1] = (char)(0x80 | ((wch >> 6) & 0x3F));
        out[2] = (char)(0x80 | (wch & 0x3F));
        out[3] = '\0';
        return 3;
    }
    if (wch < 0x110000)
    {
        out[0] = (char)(0xF0 | (wch >> 18));
        out[1] = (char)(0x80 | ((wch >> 12) & 0x3F));
        out[2] = (char)(0x80 | ((wch >> 6) & 0x3F));
        out[3] = (char)(0x80 | (wch & 0x3F));
        out[4] = '\0';
        return 4;
    }

    out[0] = '\0';
    return 0;
}
