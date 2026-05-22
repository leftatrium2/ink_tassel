#include "editor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ---- Menu data definitions ---- */

#define MENU_SEPARATOR "--------"
#define MENU_WIDTH 24

static int g_file_keys[MENU_FILE_COUNT] =
{
    STR_FILE_SAVE,
    STR_FILE_SAVE_EXIT,
    STR_FILE_DISCARD,
    STR_FILE_READ,
    STR_FILE_SET_PWD,
    -1,
    STR_FILE_SETTINGS
};

static int g_edit_keys[MENU_EDIT_COUNT] =
{
    STR_EDIT_UNDO,
    STR_EDIT_REDO,
    -1,
    STR_EDIT_FIND
};

static int g_cloud_keys[MENU_CLOUD_COUNT] =
{
    STR_CLOUD_SYNC
};

static int g_help_keys[MENU_HELP_COUNT] =
{
    STR_HELP_ABOUT
};

static int g_lang_keys[MENU_LANG_COUNT] =
{
    STR_LANG_ZH,
    STR_LANG_EN
};

/* ---- Get current menu item count ---- */

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

/* ---- Get menu item text ---- */

static const char *menu_item_text(int top_sel, int idx)
{
    int *keys = NULL;
    int count = 0;

    switch (top_sel)
    {
        case MENU_TOP_FILE:  keys = g_file_keys;  count = MENU_FILE_COUNT;  break;
        case MENU_TOP_EDIT:  keys = g_edit_keys;  count = MENU_EDIT_COUNT;  break;
        case MENU_TOP_CLOUD: keys = g_cloud_keys; count = MENU_CLOUD_COUNT; break;
        case MENU_TOP_HELP:  keys = g_help_keys;  count = MENU_HELP_COUNT;  break;
        default: return "";
    }
    if (idx < 0 || idx >= count) return "";
    if (keys[idx] < 0) return MENU_SEPARATOR;
    return T(keys[idx]);
}

/* ---- Check if item is a separator ---- */

static int is_separator_idx(int top_sel, int idx)
{
    int *keys = NULL;
    switch (top_sel)
    {
        case MENU_TOP_FILE: keys = g_file_keys; break;
        case MENU_TOP_EDIT: keys = g_edit_keys; break;
        default: return 0;
    }
    return (keys[idx] < 0);
}

/* ---- Helper: bottom input prompt ---- */

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

/* ---- Menu callbacks: File ---- */

static void menu_action_save(void)
{
    char fname[FILENAME_SIZE];

    if (!g_editor.has_filename)
    {
        prompt_input(T(STR_PROMPT_SAVE_AS), fname, FILENAME_SIZE);
        if (fname[0] == '\0')
        {
            snprintf(g_editor.msg, MSG_SIZE, "%s", T(STR_MSG_CANCEL_SAVE));
            return;
        }
        strncpy(g_editor.filename, fname, FILENAME_SIZE - 1);
        g_editor.filename[FILENAME_SIZE - 1] = '\0';
        g_editor.has_filename = 1;
    }

    if (file_write(g_editor.filename) == 0)
    {
        snprintf(g_editor.msg, MSG_SIZE, T(STR_MSG_SAVED), g_editor.filename);
        g_editor.modified = 0;
    }
    else
    {
        snprintf(g_editor.msg, MSG_SIZE, T(STR_MSG_SAVE_FAILED),
                 g_editor.filename);
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

/* MENU_ACTIONS_CONT */

static void menu_action_read(void)
{
    char fname[FILENAME_SIZE];

    prompt_input(T(STR_PROMPT_FILENAME), fname, FILENAME_SIZE);
    if (fname[0] == '\0')
    {
        snprintf(g_editor.msg, MSG_SIZE,
                 EDITOR_NAME " " EDITOR_VERSION " - %s", T(STR_MSG_ESC_MENU));
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
        snprintf(g_editor.msg, MSG_SIZE, T(STR_MSG_FILE_READ_OK), fname);
    }
    else
    {
        snprintf(g_editor.msg, MSG_SIZE, T(STR_MSG_FILE_READ_FAIL), fname);
    }
}

static void menu_action_set_password(void)
{
    char pwd[64];
    char confirm[64];

    prompt_input(T(STR_PROMPT_NEW_PWD), pwd, 64);
    if (pwd[0] == '\0')
    {
        snprintf(g_editor.msg, MSG_SIZE, "%s", T(STR_MSG_CANCEL_PWD));
        return;
    }

    prompt_input(T(STR_PROMPT_CONFIRM_PWD), confirm, 64);
    if (confirm[0] == '\0')
    {
        snprintf(g_editor.msg, MSG_SIZE, "%s", T(STR_MSG_CANCEL_PWD));
        return;
    }
/* MENU_ACTIONS_CONT2 */

    if (strcmp(pwd, confirm) != 0)
    {
        snprintf(g_editor.msg, MSG_SIZE, "%s", T(STR_MSG_PWD_MISMATCH));
        return;
    }

    strncpy(g_editor.password, pwd, 63);
    g_editor.password[63] = '\0';
    g_editor.has_password = 1;
    snprintf(g_editor.msg, MSG_SIZE, "%s", T(STR_MSG_PWD_SET));
}

static void menu_action_not_implemented(void)
{
    snprintf(g_editor.msg, MSG_SIZE, "%s", T(STR_MSG_NOT_IMPL));
}

static void menu_action_about(void)
{
    snprintf(g_editor.msg, MSG_SIZE,
             T(STR_MSG_ABOUT_FMT), EDITOR_NAME, EDITOR_VERSION);
}

/* ---- Execute menu item ---- */

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
/* MENU_EXECUTE_CONT */

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

/* ---- Execute right-side language menu ---- */

static void menu_execute_lang(int sel)
{
    switch (sel)
    {
        case MENU_LANG_ZH: i18n_set_lang(LANG_ZH); break;
        case MENU_LANG_EN: i18n_set_lang(LANG_EN); break;
        default: break;
    }
    snprintf(g_editor.msg, MSG_SIZE,
             EDITOR_NAME " " EDITOR_VERSION " - %s", T(STR_MSG_ESC_MENU));
}

/* ---- Draw top menu bar ---- */

static void menu_draw_top_bar(void)
{
    int i;
    int x = 1;
    int right_x;
    const char *lang_label;

    move(1, 0);
    clrtoeol();

    for (i = 0; i < MENU_TOP_COUNT; i++)
    {
        const char *name = T(STR_MENU_FILE + i);
        move(1, x);
        if (!g_editor.menu_right_active && i == g_editor.menu_top_sel)
            attron(A_REVERSE);
        addch(' ');
        addstr(name);
        addch(' ');
/* MENU_TOPBAR_CONT */
        if (!g_editor.menu_right_active && i == g_editor.menu_top_sel)
            attroff(A_REVERSE);
        x += utf8_display_width(name) + 3;
    }

    lang_label = T(STR_MENU_LANG);
    right_x = COLS - utf8_display_width(lang_label) - 3;
    if (right_x < x)
        right_x = x + 2;

    move(1, right_x);
    if (g_editor.menu_right_active)
        attron(A_REVERSE);
    addch(' ');
    addstr(lang_label);
    addch(' ');
    if (g_editor.menu_right_active)
        attroff(A_REVERSE);
}

/* ---- Calculate left submenu X offset ---- */

static int menu_sub_x(int top_sel)
{
    int x = 1;
    int i;
    for (i = 0; i < top_sel; i++)
        x += utf8_display_width(T(STR_MENU_FILE + i)) + 3;
    return x;
}

/* MENU_DRAW_START */

/* ---- Draw submenu ---- */

void menu_draw(void)
{
    int start_x;
    int start_y;
    int count;
    int i;

    for (i = 1; i < LINES - 1; i++)
    {
        move(i, 0);
        clrtoeol();
    }

    menu_draw_top_bar();

    if (g_editor.menu_right_active)
    {
        const char *lang_label = T(STR_MENU_LANG);
        int left_x = 1;
        for (i = 0; i < MENU_TOP_COUNT; i++)
            left_x += utf8_display_width(T(STR_MENU_FILE + i)) + 3;
        start_x = COLS - utf8_display_width(lang_label) - 3;
        if (start_x < left_x)
            start_x = left_x + 2;
        start_y = 2;
        count = MENU_LANG_COUNT;

        move(start_y, start_x);
        addch('+');
        for (i = 0; i < MENU_WIDTH - 2; i++)
            addch('-');
        addch('+');

        for (i = 0; i < count; i++)
        {
            int j;
            int item_w;
            const char *text = T(g_lang_keys[i]);
            move(start_y + 1 + i, start_x);
            addch('|');
            item_w = utf8_display_width(text);
            addch(' ');
            if (i == g_editor.menu_right_sel)
                attron(A_REVERSE);
            addstr(text);
/* MENU_DRAW_CONT */
            for (j = item_w; j < MENU_WIDTH - 4; j++)
                addch(' ');
            if (i == g_editor.menu_right_sel)
                attroff(A_REVERSE);
            addch(' ');
            addch('|');
        }

        move(start_y + 1 + count, start_x);
        addch('+');
        for (i = 0; i < MENU_WIDTH - 2; i++)
            addch('-');
        addch('+');
        return;
    }

    start_x = menu_sub_x(g_editor.menu_top_sel);
    start_y = 2;
    count = menu_item_count(g_editor.menu_top_sel);

    move(start_y, start_x);
    addch('+');
    for (i = 0; i < MENU_WIDTH - 2; i++)
        addch('-');
    addch('+');

    for (i = 0; i < count; i++)
    {
        int j;
        const char *text = menu_item_text(g_editor.menu_top_sel, i);
        move(start_y + 1 + i, start_x);
        addch('|');

        if (is_separator_idx(g_editor.menu_top_sel, i))
        {
            for (j = 0; j < MENU_WIDTH - 2; j++)
                addch('-');
        }
        else
        {
            int item_w = utf8_display_width(text);
            addch(' ');
            if (i == g_editor.menu_sel)
                attron(A_REVERSE);
            addstr(text);
            for (j = item_w; j < MENU_WIDTH - 4; j++)
                addch(' ');
            if (i == g_editor.menu_sel)
                attroff(A_REVERSE);
            addch(' ');
        }
        addch('|');
    }
/* MENU_DRAW_BOTTOM */

    move(start_y + 1 + count, start_x);
    addch('+');
    for (i = 0; i < MENU_WIDTH - 2; i++)
        addch('-');
    addch('+');
}

/* ---- Skip separator helper ---- */

static void menu_sel_skip_sep_down(void)
{
    int count = menu_item_count(g_editor.menu_top_sel);
    while (g_editor.menu_sel < count &&
           is_separator_idx(g_editor.menu_top_sel, g_editor.menu_sel))
        g_editor.menu_sel++;
    if (g_editor.menu_sel >= count)
        g_editor.menu_sel = count - 1;
}

static void menu_sel_skip_sep_up(void)
{
    while (g_editor.menu_sel >= 0 &&
           is_separator_idx(g_editor.menu_top_sel, g_editor.menu_sel))
        g_editor.menu_sel--;
    if (g_editor.menu_sel < 0)
        g_editor.menu_sel = 0;
}

/* MENU_HANDLE_KEY_START */

/* ---- Menu key handler ---- */

int menu_handle_key(int ch)
{
    if (g_editor.menu_right_active)
    {
        switch (ch)
        {
            case KEY_UP:
                g_editor.menu_right_sel--;
                if (g_editor.menu_right_sel < 0)
                    g_editor.menu_right_sel = MENU_LANG_COUNT - 1;
                break;
            case KEY_DOWN:
                g_editor.menu_right_sel++;
                if (g_editor.menu_right_sel >= MENU_LANG_COUNT)
                    g_editor.menu_right_sel = 0;
                break;
            case KEY_LEFT:
                g_editor.menu_right_active = 0;
                g_editor.menu_top_sel = MENU_TOP_COUNT - 1;
                g_editor.menu_sel = 0;
                menu_sel_skip_sep_down();
                break;
            case KEY_RIGHT:
                g_editor.menu_right_active = 0;
                g_editor.menu_top_sel = 0;
                g_editor.menu_sel = 0;
                menu_sel_skip_sep_down();
                break;
            case '\n':
            case '\r':
            case KEY_ENTER:
                g_editor.menu_active = 0;
                g_editor.menu_right_active = 0;
                menu_execute_lang(g_editor.menu_right_sel);
                break;
            case 27:
                g_editor.menu_active = 0;
                g_editor.menu_right_active = 0;
                snprintf(g_editor.msg, MSG_SIZE,
                         EDITOR_NAME " " EDITOR_VERSION " - %s",
                         T(STR_MSG_ESC_MENU));
                break;
            default:
                break;
        }
        return 0;
    }
/* MENU_HANDLE_LEFT_KEY */

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
                {
                    g_editor.menu_right_active = 1;
                    g_editor.menu_right_sel = 0;
                    return 0;
                }
                g_editor.menu_sel = 0;
                menu_sel_skip_sep_down();
                break;

            case KEY_RIGHT:
                g_editor.menu_top_sel++;
                if (g_editor.menu_top_sel >= MENU_TOP_COUNT)
                {
                    g_editor.menu_right_active = 1;
                    g_editor.menu_right_sel = 0;
                    return 0;
                }
                g_editor.menu_sel = 0;
                menu_sel_skip_sep_down();
                break;

            case '\n':
            case '\r':
            case KEY_ENTER:
                g_editor.menu_active = 0;
                menu_execute(g_editor.menu_top_sel, g_editor.menu_sel);
                break;
/* MENU_HANDLE_ESC */

            case 27:
                g_editor.menu_active = 0;
                snprintf(g_editor.msg, MSG_SIZE,
                         EDITOR_NAME " " EDITOR_VERSION " - %s",
                         T(STR_MSG_ESC_MENU));
                break;

            default:
                break;
        }
    }

    return 0;
}
