#include "editor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* 菜单项名称 */
static const char *g_menu_items[MENU_FILE_ITEMS] =
{
    "保存文件",
    "存盘退出",
    "放弃存盘",
    "读文件",
    "设置密码"
};

/* 菜单尺寸 */
#define MENU_WIDTH   24
#define MENU_HEIGHT  (MENU_FILE_ITEMS + 2)

/* ---- 辅助: 底部输入提示 ---- */

static void prompt_input(const char *prompt_str, char *out_buf, int buf_size)
{
    int pos;
    int done;
    wint_t wch;
    int prompt_width;

    curs_set(1);

    prompt_width = utf8_display_width(prompt_str);

    move(LINES - 1, 0);
    clrtoeol();
    mvaddstr(LINES - 1, 0, prompt_str);
    refresh();

    pos = 0;
    done = 0;
    memset(out_buf, 0, (size_t)buf_size);

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
            out_buf[0] = '\0';
            done = 1;
        }
        else if (wch == KEY_BACKSPACE || wch == 127 || wch == '\b')
        {
            if (pos > 0)
            {
                pos = utf8_prev_char_start(out_buf, pos);
                out_buf[pos] = '\0';

                /* 清空输入区并重新显示剩余内容 */
                move(LINES - 1, prompt_width);
                clrtoeol();
                addstr(out_buf);
                move(LINES - 1, prompt_width + utf8_display_width(out_buf));
                refresh();
            }
        }
        else if (wch >= 32 && pos < buf_size - 1)
        {
            char utf8[5];
            int bytes;
            int k;
            bytes = utf8_encode(wch, utf8);

            if (pos + bytes < buf_size)
            {
                for (k = 0; k < bytes; k++)
                {
                    out_buf[pos++] = utf8[k];
                }
                out_buf[pos] = '\0';

                /* 清空并重新显示整个输入区 */
                move(LINES - 1, prompt_width);
                clrtoeol();
                addstr(out_buf);
                move(LINES - 1, prompt_width + utf8_display_width(out_buf));
                refresh();
            }
        }
    }

    curs_set(1);
}

/* ---- 菜单回调 ---- */

static void menu_action_save(void)
{
    char fname[FILENAME_SIZE];

    if (!g_editor.has_filename)
    {
        prompt_input("保存为(文件名): ", fname, FILENAME_SIZE);
        if (fname[0] == '\0')
        {
            snprintf(g_editor.msg, MSG_SIZE, "取消保存");
            return;
        }
        strncpy(g_editor.filename, fname, FILENAME_SIZE - 1);
        g_editor.filename[FILENAME_SIZE - 1] = '\0';
        g_editor.has_filename = 1;
    }

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

static void menu_action_save_exit(void)
{
    menu_action_save();
    if (g_editor.modified == 0)
    {
        g_editor.running = 0;
    }
}

static void menu_action_discard(void)
{
    g_editor.running = 0;
}

static void menu_action_read(void)
{
    char fname[FILENAME_SIZE];

    prompt_input("文件名: ", fname, FILENAME_SIZE);
    if (fname[0] == '\0')
    {
        snprintf(g_editor.msg, MSG_SIZE, EDITOR_NAME " " EDITOR_VERSION " - ESC:菜单");
        return;
    }

    if (file_read(fname) == 0)
    {
        strncpy(g_editor.filename, fname, FILENAME_SIZE - 1);
        g_editor.filename[FILENAME_SIZE - 1] = '\0';
        g_editor.has_filename = 1;
        g_editor.modified = 0;
        g_editor.cur_row = 0;
        g_editor.cur_col = 0;
        g_editor.top_row = 0;
        snprintf(g_editor.msg, MSG_SIZE, "已读取文件: %s", fname);
    }
    else
    {
        snprintf(g_editor.msg, MSG_SIZE, "读取文件失败: %s", fname);
    }
}

static void menu_action_set_password(void)
{
    char pwd[64];
    char confirm[64];

    prompt_input("输入新密码: ", pwd, 64);
    if (pwd[0] == '\0')
    {
        snprintf(g_editor.msg, MSG_SIZE, "取消设置密码");
        return;
    }

    prompt_input("确认密码: ", confirm, 64);
    if (confirm[0] == '\0')
    {
        snprintf(g_editor.msg, MSG_SIZE, "取消设置密码");
        return;
    }

    if (strcmp(pwd, confirm) != 0)
    {
        snprintf(g_editor.msg, MSG_SIZE, "密码不匹配");
        return;
    }

    strncpy(g_editor.password, pwd, 63);
    g_editor.password[63] = '\0';
    g_editor.has_password = 1;
    snprintf(g_editor.msg, MSG_SIZE, "密码已设置");
}

/* ---- 执行菜单项 ---- */

static void menu_execute(int sel)
{
    switch (sel)
    {
        case MENU_FILE_SAVE:
            menu_action_save();
            break;
        case MENU_FILE_SAVE_EXIT:
            menu_action_save_exit();
            break;
        case MENU_FILE_DISCARD:
            menu_action_discard();
            break;
        case MENU_FILE_READ:
            menu_action_read();
            break;
        case MENU_FILE_SET_PASSWORD:
            menu_action_set_password();
            break;
        default:
            break;
    }
}

/* ---- 绘制菜单 ---- */

void menu_draw(void)
{
    int start_x;
    int start_y;
    int i;

    start_x = 1;
    start_y = 1;

    /* 顶部边框 */
    move(start_y, start_x);
    addch('+');
    for (i = 0; i < MENU_WIDTH - 2; i++)
    {
        addch('-');
    }
    addch('+');

    /* 标题行 */
    move(start_y + 1, start_x);
    addch('|');
    {
        int j;
        int title_w;
        addstr(" 文件");
        title_w = utf8_display_width(" 文件");
        for (j = title_w; j < MENU_WIDTH - 2; j++)
        {
            addch(' ');
        }
    }
    addch('|');

    /* 分隔线 */
    move(start_y + 2, start_x);
    addch('+');
    for (i = 0; i < MENU_WIDTH - 2; i++)
    {
        addch('-');
    }
    addch('+');

    /* 菜单项 */
    for (i = 0; i < MENU_FILE_ITEMS; i++)
    {
        int item_w;
        int j;

        item_w = utf8_display_width(g_menu_items[i]);

        move(start_y + 3 + i, start_x);
        addch('|');
        addch(' ');

        if (i == g_editor.menu_sel)
        {
            attron(A_REVERSE);
            addstr(g_menu_items[i]);
            for (j = item_w; j < MENU_WIDTH - 4; j++)
            {
                addch(' ');
            }
            attroff(A_REVERSE);
        }
        else
        {
            addstr(g_menu_items[i]);
            for (j = item_w; j < MENU_WIDTH - 4; j++)
            {
                addch(' ');
            }
        }

        addch(' ');
        addch('|');
    }

    /* 底部边框 */
    move(start_y + 3 + MENU_FILE_ITEMS, start_x);
    addch('+');
    for (i = 0; i < MENU_WIDTH - 2; i++)
    {
        addch('-');
    }
    addch('+');
}

/* ---- 菜单按键处理 ---- */

int menu_handle_key(int ch)
{
    switch (ch)
    {
        case KEY_UP:
            g_editor.menu_sel--;
            if (g_editor.menu_sel < 0)
            {
                g_editor.menu_sel = MENU_FILE_ITEMS - 1;
            }
            break;

        case KEY_DOWN:
            g_editor.menu_sel++;
            if (g_editor.menu_sel >= MENU_FILE_ITEMS)
            {
                g_editor.menu_sel = 0;
            }
            break;

        case '\n':
        case '\r':
        case KEY_ENTER:
            g_editor.menu_active = 0;
            menu_execute(g_editor.menu_sel);
            break;

        case 27: /* ESC */
            g_editor.menu_active = 0;
            snprintf(g_editor.msg, MSG_SIZE, EDITOR_NAME " " EDITOR_VERSION " - ESC:菜单");
            break;

        default:
            break;
    }

    return 0;
}
