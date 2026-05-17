#include "editor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* 最大临时行数 */
#define TEMP_MAX_LINES  20000

int file_read(const char *filename)
{
    FILE *fp;
    char **temp_lines;
    int temp_count;
    int temp_cap;
    char buf[MAX_LINE_SIZE];
    int i;

    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        return -1;
    }

    temp_cap = LINE_CAP_INIT;
    temp_lines = (char **)malloc(sizeof(char *) * (size_t)temp_cap);
    temp_count = 0;

    /* 读取全部行 */
    while (fgets(buf, MAX_LINE_SIZE, fp) != NULL)
    {
        int len;
        char *line;

        len = (int)strlen(buf);

        /* 去除尾部换行符 */
        if (len > 0 && buf[len - 1] == '\n')
        {
            buf[len - 1] = '\0';
            len--;
        }
        if (len > 0 && buf[len - 1] == '\r')
        {
            buf[len - 1] = '\0';
            len--;
        }

        /* 扩展临时数组 */
        if (temp_count >= temp_cap)
        {
            temp_cap += LINE_CAP_STEP;
            temp_lines = (char **)realloc(temp_lines,
                                         sizeof(char *) * (size_t)temp_cap);
        }

        line = (char *)malloc((size_t)(len + 1));
        memcpy(line, buf, (size_t)(len + 1));

        /* 如果有密码，解密 */
        if (g_editor.has_password)
        {
            password_xor_crypt(line, len, g_editor.password);
        }

        temp_lines[temp_count++] = line;

        if (temp_count >= TEMP_MAX_LINES)
        {
            break;
        }
    }

    fclose(fp);

    /* 清空当前缓冲区 */
    buffer_free();

    /* 加载到编辑器缓冲区 */
    g_editor.lines = (LINE *)malloc(sizeof(LINE) * LINE_CAP_INIT);
    g_editor.cap_lines = LINE_CAP_INIT;
    g_editor.num_lines = 0;

    buffer_ensure_line();

    if (temp_count > 0)
    {
        /* 取第一行内容替换默认空行 */
        LINE *first;
        int first_len;

        first = &g_editor.lines[0];
        first_len = (int)strlen(temp_lines[0]);

        if (first_len + 1 > first->size)
        {
            first->data = (char *)realloc(first->data, (size_t)(first_len + 1));
            first->size = first_len + 1;
        }

        strcpy(first->data, temp_lines[0]);
        first->len = first_len;

        g_editor.num_lines = 1;

        /* 追加剩余行 */
        for (i = 1; i < temp_count; i++)
        {
            int line_len;
            LINE *new_line;

            line_len = (int)strlen(temp_lines[i]);

            /* 扩展行数组 */
            if (g_editor.num_lines >= g_editor.cap_lines)
            {
                g_editor.cap_lines += LINE_CAP_STEP;
                g_editor.lines = (LINE *)realloc(g_editor.lines,
                                sizeof(LINE) * (size_t)g_editor.cap_lines);
            }

            new_line = &g_editor.lines[g_editor.num_lines];
            new_line->size = LINE_DATA_INIT;
            if (new_line->size < line_len + 1)
            {
                new_line->size = line_len + 1;
            }
            new_line->data = (char *)malloc((size_t)new_line->size);
            memcpy(new_line->data, temp_lines[i], (size_t)(line_len + 1));
            new_line->len = line_len;

            g_editor.num_lines++;
        }
    }

    /* 释放临时行 */
    for (i = 0; i < temp_count; i++)
    {
        free(temp_lines[i]);
    }
    free(temp_lines);

    return 0;
}

int file_write(const char *filename)
{
    FILE *fp;
    int i;

    fp = fopen(filename, "w");
    if (fp == NULL)
    {
        return -1;
    }

    for (i = 0; i < g_editor.num_lines; i++)
    {
        LINE *line;
        char *write_buf;
        int write_len;

        line = &g_editor.lines[i];
        write_len = line->len;
        write_buf = (char *)malloc((size_t)(write_len + 1));

        memcpy(write_buf, line->data, (size_t)write_len);
        write_buf[write_len] = '\0';

        /* 如果有密码，加密 */
        if (g_editor.has_password)
        {
            password_xor_crypt(write_buf, write_len, g_editor.password);
        }

        /* 写入行内容 */
        if (write_len > 0)
        {
            fputs(write_buf, fp);
        }

        /* 最后一行不写换行符 */
        if (i < g_editor.num_lines - 1)
        {
            fputc('\n', fp);
        }

        free(write_buf);
    }

    fclose(fp);
    return 0;
}
