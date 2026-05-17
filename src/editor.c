#include "editor.h"
#include <stdlib.h>
#include <string.h>

EDITOR g_editor;

void editor_init(void)
{
    memset(&g_editor, 0, sizeof(EDITOR));
    g_editor.running = 1;

    buffer_init();
    strcpy(g_editor.msg, EDITOR_NAME " " EDITOR_VERSION " - ESC:菜单");
}

void editor_cleanup(void)
{
    buffer_free();
    memset(&g_editor, 0, sizeof(EDITOR));
}
