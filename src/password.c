#include "editor.h"
#include <string.h>
#include <stdio.h>

void password_xor_crypt(char *data, int len, const char *key)
{
    int key_len;
    int i;

    key_len = (int)strlen(key);
    if (key_len == 0)
    {
        return;
    }

    for (i = 0; i < len; i++)
    {
        data[i] ^= key[i % key_len];
    }
}

void password_set(void)
{
    /* 密码设置通过 menu_action_set_password 完成 */
    /* 此函数保留给未来独立调用 */
}

int password_verify(void)
{
    char input[64];

    if (!g_editor.has_password)
    {
        return 1;
    }

    echo();
    curs_set(1);
    move(LINES - 1, 0);
    clrtoeol();
    mvaddstr(LINES - 1, 0, T(STR_PROMPT_ENTER_PWD));
    refresh();

    {
        int pos;
        int ch;
        int done;

        pos = 0;
        done = 0;
        memset(input, 0, sizeof(input));

        while (!done)
        {
            ch = getch();
            if (ch == '\n' || ch == '\r')
            {
                done = 1;
            }
            else if (ch == 27)
            {
                input[0] = '\0';
                done = 1;
            }
            else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b')
            {
                if (pos > 0)
                {
                    pos--;
                    input[pos] = '\0';
                    move(LINES - 1, (int)strlen(T(STR_PROMPT_ENTER_PWD)) + pos);
                    addch(' ');
                    move(LINES - 1, (int)strlen(T(STR_PROMPT_ENTER_PWD)) + pos);
                }
            }
            else if (ch >= 32 && ch < 127 && pos < 63)
            {
                input[pos++] = (char)ch;
                input[pos] = '\0';
                addch('*');
            }
        }
    }

    noecho();
    curs_set(1);

    if (input[0] != '\0' && strcmp(input, g_editor.password) == 0)
    {
        return 1;
    }

    return 0;
}
