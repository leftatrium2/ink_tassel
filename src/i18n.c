#include "i18n.h"

static int g_current_lang = LANG_ZH;

static const char *g_zh_strings[STR_COUNT] =
{
    "文件",
    "编辑",
    "云存储",
    "帮助",
    "切换语言",
    "保存文件",
    "存盘退出",
    "放弃存盘",
    "读文件",
    "设置密码",
    "设置",
    "撤销",
    "重做",
    "查找",
    "同步",
    "关于",
    "中文",
    "English",
    "保存为(文件名): ",
    "文件名: ",
    "输入新密码: ",
    "确认密码: ",
    "请输入密码: ",
    "取消保存",
    "已保存: %s",
    "保存失败: %s",
    "暂无此功能",
    "%s v%s - 文本编辑器",
    "ESC:菜单",
    "已读取文件: %s",
    "读取文件失败: %s",
    "请使用菜单保存新文件",
    "取消设置密码",
    "密码不匹配",
    "密码已设置",
    "文件: ",
    "行: ",
    "列: ",
    "[已修改]",
    "[未命名]"
};

static const char *g_en_strings[STR_COUNT] =
{
    "File",
    "Edit",
    "Cloud",
    "Help",
    "Language",
    "Save",
    "Save & Exit",
    "Discard",
    "Open File",
    "Set Password",
    "Settings",
    "Undo",
    "Redo",
    "Find",
    "Sync",
    "About",
    "中文",
    "English",
    "Save as (filename): ",
    "Filename: ",
    "New password: ",
    "Confirm password: ",
    "Enter password: ",
    "Save cancelled",
    "Saved: %s",
    "Save failed: %s",
    "Not implemented",
    "%s v%s - Text Editor",
    "ESC:Menu",
    "File loaded: %s",
    "Failed to load: %s",
    "Use menu to save new file",
    "Password cancelled",
    "Passwords don't match",
    "Password set",
    "File: ",
    "Ln: ",
    "Col: ",
    "[Modified]",
    "[Untitled]"
};

static const char **g_tables[LANG_COUNT] =
{
    g_zh_strings,
    g_en_strings
};

void i18n_init(void)
{
    g_current_lang = LANG_ZH;
}

void i18n_set_lang(int lang)
{
    if (lang >= 0 && lang < LANG_COUNT)
    {
        g_current_lang = lang;
    }
}

int i18n_get_lang(void)
{
    return g_current_lang;
}

const char *i18n_get(int key)
{
    if (key >= 0 && key < STR_COUNT)
    {
        return g_tables[g_current_lang][key];
    }
    return "";
}
