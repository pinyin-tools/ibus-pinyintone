#include "engine.h"
#include "process_key_event.h"

#define is_alpha(c) (((c) >= IBUS_a && (c) <= IBUS_z) || ((c) >= IBUS_A && (c) <= IBUS_Z))
#define is_tone(c) ((c) >= IBUS_1 && (c) <= IBUS_5)

/**
 *
 */
gboolean ibus_rustpinyin_engine_process_key_event (
    IBusEngine* engine,
    guint keyval,
    guint keycode,
    guint modifiers
) {
    IBusText *text;
    IBusRustPinyinEngine *rustpinyin = (IBusRustPinyinEngine *)engine;

    if (modifiers & IBUS_RELEASE_MASK) {
        return FALSE;
    }

    modifiers &= (IBUS_CONTROL_MASK | IBUS_MOD1_MASK);

    if (modifiers != 0) {
        if (rustpinyin->preedit->len == 0) {
            return FALSE;
        } else {
            return TRUE;
        }
    }


    switch (keyval) {
    case IBUS_space:
        g_string_append (rustpinyin->preedit, " ");
        return ibus_rustpinyin_engine_commit_preedit (rustpinyin);
    case IBUS_Return:
        return ibus_rustpinyin_engine_commit_preedit (rustpinyin);

    case IBUS_Escape:
        if (rustpinyin->preedit->len == 0)
            return FALSE;

        g_string_assign (rustpinyin->preedit, "");
        rustpinyin->cursor_pos = 0;
        ibus_rustpinyin_engine_update (rustpinyin);
        return TRUE;        

    case IBUS_Left:
        if (rustpinyin->preedit->len == 0)
            return FALSE;
        if (rustpinyin->cursor_pos > 0) {
            rustpinyin->cursor_pos --;
            ibus_rustpinyin_engine_update (rustpinyin);
        }
        return TRUE;

    case IBUS_Right:
        if (rustpinyin->preedit->len == 0)
            return FALSE;
        if (rustpinyin->cursor_pos < rustpinyin->preedit->len) {
            rustpinyin->cursor_pos ++;
            ibus_rustpinyin_engine_update (rustpinyin);
        }
        return TRUE;
    
    case IBUS_Up:
        if (rustpinyin->preedit->len == 0)
            return FALSE;
        if (rustpinyin->cursor_pos != 0) {
            rustpinyin->cursor_pos = 0;
            ibus_rustpinyin_engine_update (rustpinyin);
        }
        return TRUE;

    case IBUS_Down:
        if (rustpinyin->preedit->len == 0)
            return FALSE;
        
        if (rustpinyin->cursor_pos != rustpinyin->preedit->len) {
            rustpinyin->cursor_pos = rustpinyin->preedit->len;
            ibus_rustpinyin_engine_update (rustpinyin);
        }
        
        return TRUE;
    
    case IBUS_BackSpace:
        if (rustpinyin->preedit->len == 0)
            return FALSE;
        if (rustpinyin->cursor_pos > 0) {
            rustpinyin->cursor_pos --;
            g_string_erase (rustpinyin->preedit, rustpinyin->cursor_pos, 1);
            ibus_rustpinyin_engine_update (rustpinyin);
            ibus_rustpinyin_engine_update_lookup_table (rustpinyin);
        }
        return TRUE;
    
    case IBUS_Delete:
        if (rustpinyin->preedit->len == 0)
            return FALSE;
        if (rustpinyin->cursor_pos < rustpinyin->preedit->len) {
            g_string_erase (rustpinyin->preedit, rustpinyin->cursor_pos, 1);
            ibus_rustpinyin_engine_update (rustpinyin);
        }
        return TRUE;
    }

    if (is_alpha (keyval) || is_tone(keyval)) {
        g_string_insert_c (rustpinyin->preedit,
                           rustpinyin->cursor_pos,
                           keyval);

        rustpinyin->cursor_pos ++;
        ibus_rustpinyin_engine_update (rustpinyin);
        ibus_rustpinyin_engine_update_lookup_table (rustpinyin);
        
        return TRUE;
    }

    return FALSE;
}
