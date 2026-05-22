#include "editor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

EDITOR g_editor;

void editor_init(void)
{
    memset(&g_editor, 0, sizeof(EDITOR));
    g_editor.running = 1;

    i18n_init();
    buffer_init();
    snprintf(g_editor.msg, MSG_SIZE,
             EDITOR_NAME " " EDITOR_VERSION " - %s", T(STR_MSG_ESC_MENU));
}

void editor_cleanup(void)
{
    buffer_free();
    memset(&g_editor, 0, sizeof(EDITOR));
}
