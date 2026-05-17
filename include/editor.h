#ifndef EDITOR_H
#define EDITOR_H

#define _XOPEN_SOURCE_EXTENDED 1
#include <ncurses.h>
#include <wchar.h>

/* ---- 宏定义 ---- */

#define EDITOR_VERSION "1.0.0"
#define EDITOR_NAME    "墨流苏"

#define MAX_LINE_SIZE  1024
#define MAX_LINES      10000
#define MSG_SIZE       256
#define FILENAME_SIZE  256

#define LINE_CAP_INIT  64
#define LINE_CAP_STEP  32
#define LINE_DATA_INIT 128
#define LINE_DATA_STEP 64

/* ---- 菜单项枚举 ---- */

enum
{
    MENU_FILE_SAVE = 0,
    MENU_FILE_SAVE_EXIT,
    MENU_FILE_DISCARD,
    MENU_FILE_READ,
    MENU_FILE_SET_PASSWORD,
    MENU_FILE_COUNT
};

#define MENU_FILE_ITEMS 5

/* ---- 行结构 ---- */

typedef struct
{
    char *data;
    int   size;
    int   len;
} LINE;

/* ---- 编辑器全局状态 ---- */

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
    int   menu_sel;

    int   running;

    char  password[64];
    int   has_password;

    char  msg[MSG_SIZE];
} EDITOR;

/* ---- 全局变量声明 ---- */

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
