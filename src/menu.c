#include "editor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ---- 菜单数据定义 ---- */

#define MENU_SEPARATOR "--------"

static const char *g_top_menu_names[MENU_TOP_COUNT] =
{
    "文件", "编辑", "云存储", "帮助"
};

static const char *g_file_items[MENU_FILE_COUNT] =
{
    "保存文件",
    "存盘退出",
    "放弃存盘",
    "读文件",
    "设置密码",
    MENU_SEPARATOR,
    "设置"
};

static const char *g_edit_items[MENU_EDIT_COUNT] =
{
    "撤销",
    "重做",
    MENU_SEPARATOR,
    "查找"
};

static const char *g_cloud_items[MENU_CLOUD_COUNT] =
{
    "同步"
};

static const char *g_help_items[MENU_HELP_COUNT] =
{
    "关于"
};

#define MENU_WIDTH 24

/* ---- 获取当前菜单的项数 ---- */

static int menu_item_count(int top_sel)
{
    switch (top_sel)
    {
        case MENU_TOP_FILE:  return MENU_FILE_COUNT;
        case MENU_TOP_EDIT:  return MENU_EDIT_COUNT;
        case MENU_TOP_CLOUD: return MENU_CLOUD_COUNT;
        case MENU_TOP_HELP:  return MENU_HELP_COUNT;
        default:             return 0;
    }
}

/* ---- 获取当前菜单的项名称 ---- */

static const char **menu_items(int top_sel)
{
    switch (top_sel)
    {
        case MENU_TOP_FILE:  return g_file_items;
        case MENU_TOP_EDIT:  return g_edit_items;
        case MENU_TOP_CLOUD: return g_cloud_items;
        case MENU_TOP_HELP:  return g_help_items;
        default:             return NULL;
    }
}

/* ---- 判断是否为分割线 ---- */

static int is_separator(const char *item)
{
    return (strcmp(item, MENU_SEPARATOR) == 0);
}

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
/* PROMPT_LOOP */

    while (!done)
    {
        int rc;
        rc = get_wch(&wch);
        if (rc == ERR)
            continue;

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
                    out_buf[pos++] = utf8[k];
                out_buf[pos] = '\0';
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
/* ACTIONS_MARKER */

/* ---- 菜单回调: 文件 ---- */

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
        g_editor.running = 0;
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
        snprintf(g_editor.msg, MSG_SIZE,
                 EDITOR_NAME " " EDITOR_VERSION " - ESC:菜单");
        return;
    }
/* READ_CONT */

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
/* EXECUTE_MARKER */

static void menu_action_not_implemented(void)
{
    snprintf(g_editor.msg, MSG_SIZE, "暂无此功能");
}

static void menu_action_about(void)
{
    snprintf(g_editor.msg, MSG_SIZE,
             EDITOR_NAME " v" EDITOR_VERSION " - 文本编辑器");
}

/* ---- 执行菜单项 ---- */

static void menu_execute(int top_sel, int sel)
{
    switch (top_sel)
    {
        case MENU_TOP_FILE:
            switch (sel)
            {
                case MENU_FILE_SAVE:         menu_action_save(); break;
                case MENU_FILE_SAVE_EXIT:    menu_action_save_exit(); break;
                case MENU_FILE_DISCARD:      menu_action_discard(); break;
                case MENU_FILE_READ:         menu_action_read(); break;
                case MENU_FILE_SET_PASSWORD:  menu_action_set_password(); break;
                case MENU_FILE_SETTINGS:     menu_action_not_implemented(); break;
                default: break;
            }
            break;

        case MENU_TOP_EDIT:
            switch (sel)
            {
                case MENU_EDIT_UNDO: menu_action_not_implemented(); break;
                case MENU_EDIT_REDO: menu_action_not_implemented(); break;
                case MENU_EDIT_FIND: menu_action_not_implemented(); break;
                default: break;
            }
            break;

        case MENU_TOP_CLOUD:
            menu_action_not_implemented();
            break;

        case MENU_TOP_HELP:
            menu_action_about();
            break;

        default:
            break;
    }
}
/* DRAW_MARKER */

/* ---- 绘制顶部菜单栏 ---- */

static void menu_draw_top_bar(void)
{
    int i;
    int x = 1;

    move(1, 0);
    clrtoeol();

    for (i = 0; i < MENU_TOP_COUNT; i++)
    {
        move(1, x);
        if (i == g_editor.menu_top_sel)
            attron(A_REVERSE);
        addch(' ');
        addstr(g_top_menu_names[i]);
        addch(' ');
        if (i == g_editor.menu_top_sel)
            attroff(A_REVERSE);
        x += utf8_display_width(g_top_menu_names[i]) + 3;
    }
}

/* ---- 计算子菜单X偏移 ---- */

static int menu_sub_x(int top_sel)
{
    int x = 1;
    int i;
    for (i = 0; i < top_sel; i++)
        x += utf8_display_width(g_top_menu_names[i]) + 3;
    return x;
}

/* ---- 绘制子菜单 ---- */

void menu_draw(void)
{
    int start_x;
    int start_y;
    int count;
    const char **items;
    int i;

    /* 清除文本区域，防止切换菜单时残留 */
    for (i = 1; i < LINES - 1; i++)
    {
        move(i, 0);
        clrtoeol();
    }

    menu_draw_top_bar();

    start_x = menu_sub_x(g_editor.menu_top_sel);
    start_y = 2;
    count = menu_item_count(g_editor.menu_top_sel);
    items = menu_items(g_editor.menu_top_sel);
    if (!items)
        return;
/* DRAW_ITEMS */

    /* 顶部边框 */
    move(start_y, start_x);
    addch('+');
    for (i = 0; i < MENU_WIDTH - 2; i++)
        addch('-');
    addch('+');

    /* 菜单项 */
    for (i = 0; i < count; i++)
    {
        int j;
        move(start_y + 1 + i, start_x);
        addch('|');

        if (is_separator(items[i]))
        {
            for (j = 0; j < MENU_WIDTH - 2; j++)
                addch('-');
        }
        else
        {
            int item_w = utf8_display_width(items[i]);
            addch(' ');
            if (i == g_editor.menu_sel)
                attron(A_REVERSE);
            addstr(items[i]);
            for (j = item_w; j < MENU_WIDTH - 4; j++)
                addch(' ');
            if (i == g_editor.menu_sel)
                attroff(A_REVERSE);
            addch(' ');
        }
        addch('|');
    }

    /* 底部边框 */
    move(start_y + 1 + count, start_x);
    addch('+');
    for (i = 0; i < MENU_WIDTH - 2; i++)
        addch('-');
    addch('+');
}
/* HANDLE_KEY_MARKER */

/* ---- 跳过分割线辅助 ---- */

static void menu_sel_skip_sep_down(void)
{
    int count = menu_item_count(g_editor.menu_top_sel);
    const char **items = menu_items(g_editor.menu_top_sel);
    if (!items)
        return;
    while (g_editor.menu_sel < count && is_separator(items[g_editor.menu_sel]))
        g_editor.menu_sel++;
    if (g_editor.menu_sel >= count)
        g_editor.menu_sel = count - 1;
}

static void menu_sel_skip_sep_up(void)
{
    const char **items = menu_items(g_editor.menu_top_sel);
    if (!items)
        return;
    while (g_editor.menu_sel >= 0 && is_separator(items[g_editor.menu_sel]))
        g_editor.menu_sel--;
    if (g_editor.menu_sel < 0)
        g_editor.menu_sel = 0;
}

/* ---- 菜单按键处理 ---- */

int menu_handle_key(int ch)
{
    int count = menu_item_count(g_editor.menu_top_sel);

    switch (ch)
    {
        case KEY_UP:
            g_editor.menu_sel--;
            if (g_editor.menu_sel < 0)
                g_editor.menu_sel = count - 1;
            menu_sel_skip_sep_up();
            break;

        case KEY_DOWN:
            g_editor.menu_sel++;
            if (g_editor.menu_sel >= count)
                g_editor.menu_sel = 0;
            menu_sel_skip_sep_down();
            break;

        case KEY_LEFT:
            g_editor.menu_top_sel--;
            if (g_editor.menu_top_sel < 0)
                g_editor.menu_top_sel = MENU_TOP_COUNT - 1;
            g_editor.menu_sel = 0;
            menu_sel_skip_sep_down();
            break;

        case KEY_RIGHT:
            g_editor.menu_top_sel++;
            if (g_editor.menu_top_sel >= MENU_TOP_COUNT)
                g_editor.menu_top_sel = 0;
            g_editor.menu_sel = 0;
            menu_sel_skip_sep_down();
            break;

        case '\n':
        case '\r':
        case KEY_ENTER:
            g_editor.menu_active = 0;
            menu_execute(g_editor.menu_top_sel, g_editor.menu_sel);
            break;

        case 27:
            g_editor.menu_active = 0;
            snprintf(g_editor.msg, MSG_SIZE,
                     EDITOR_NAME " " EDITOR_VERSION " - ESC:菜单");
            break;

        default:
            break;
    }

    return 0;
}
