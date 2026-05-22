#ifndef EDITOR_H
#define EDITOR_H

#define _XOPEN_SOURCE 600
#define _XOPEN_SOURCE_EXTENDED 1
#include <ncurses.h>
#include <wchar.h>
#include "i18n.h"

/* ---- Macro definitions ---- */

#define EDITOR_VERSION "1.0.2"
#define EDITOR_NAME    "Ink Tassel"

#define MAX_LINE_SIZE  1024
#define MAX_LINES      10000
#define MSG_SIZE       256
#define FILENAME_SIZE  256

#define LINE_CAP_INIT  64
#define LINE_CAP_STEP  32
#define LINE_DATA_INIT 128
#define LINE_DATA_STEP 64

/* ---- Top-level menu enum ---- */

enum
{
    MENU_TOP_FILE = 0,
    MENU_TOP_EDIT,
    MENU_TOP_CLOUD,
    MENU_TOP_HELP,
    MENU_TOP_COUNT
};

/* ---- Right menu enum ---- */

enum
{
    MENU_RIGHT_LANG = 0,
    MENU_RIGHT_COUNT
};

/* ---- Language menu items ---- */

enum
{
    MENU_LANG_ZH = 0,
    MENU_LANG_EN,
    MENU_LANG_COUNT
};

/* ---- File menu items (with separator) ---- */

enum
{
    MENU_FILE_SAVE = 0,
    MENU_FILE_SAVE_EXIT,
    MENU_FILE_DISCARD,
    MENU_FILE_READ,
    MENU_FILE_SET_PASSWORD,
    MENU_FILE_SEP,
    MENU_FILE_SETTINGS,
    MENU_FILE_COUNT
};

/* ---- Edit menu items ---- */

enum
{
    MENU_EDIT_UNDO = 0,
    MENU_EDIT_REDO,
    MENU_EDIT_SEP,
    MENU_EDIT_FIND,
    MENU_EDIT_COUNT
};

/* ---- Cloud storage menu items ---- */

enum
{
    MENU_CLOUD_SYNC = 0,
    MENU_CLOUD_COUNT
};

/* ---- Help menu items ---- */

enum
{
    MENU_HELP_ABOUT = 0,
    MENU_HELP_COUNT
};

/* ---- Line structure ---- */

typedef struct
{
    char *data;
    int   size;
    int   len;
} LINE;

/* ---- Editor global state ---- */

typedef struct
{
    LINE *lines;
    int   num_lines;
    int   cap_lines;

    int   cur_row;
    int   cur_col;
    int   top_row;

    char  filename[FILENAME_SIZE];
    int   modified;
    int   has_filename;

    int   menu_active;
    int   menu_top_sel;
    int   menu_sel;
    int   menu_right_active;
    int   menu_right_sel;

    int   running;

    char  password[64];
    int   has_password;

    char  msg[MSG_SIZE];
} EDITOR;

/* ---- Global variable declarations ---- */

extern EDITOR g_editor;

/* ---- editor.c ---- */

void editor_init(void);
void editor_cleanup(void);

/* ---- buffer.c ---- */

void buffer_init(void);
void buffer_free(void);
int  buffer_insert_char(wint_t ch);
int  buffer_delete_char(void);
int  buffer_delete_char_forward(void);
int  buffer_insert_newline(void);
int  buffer_delete_line_at(int row);
void buffer_ensure_line(void);
int  utf8_char_bytes(const char *s, int pos);
int  utf8_prev_char_start(const char *s, int pos);
int  utf8_encode(wint_t wch, char *out);
int  utf8_display_width(const char *s);
int  utf8_byte_offset_to_display_col(const char *s, int byte_offset);

/* ---- display.c ---- */

void display_status_bar(void);
void display_text_area(void);
void display_message_line(void);
void display_refresh(void);

/* ---- input.c ---- */

int  input_process_key(wint_t ch);

/* ---- menu.c ---- */

void menu_draw(void);
int  menu_handle_key(int ch);

/* ---- file_io.c ---- */

int  file_read(const char *filename);
int  file_write(const char *filename);

/* ---- password.c ---- */

void password_xor_crypt(char *data, int len, const char *key);
void password_set(void);
int  password_verify(void);

#endif
