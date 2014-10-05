#include "engine.h"
#include "process_key_event.h"

#define is_alpha(c) (((c) >= IBUS_a && (c) <= IBUS_z) || ((c) >= IBUS_A && (c) <= IBUS_Z))
#define is_tone(c) ((c) >= IBUS_1 && (c) <= IBUS_5)

/**
 * process punctuation to convert it into chinese punctuation
 */
static gboolean ibus_rustpinyin_engine_process_if_punctuation(
    IBusEngine* engine,
    guint keyval,
    guint keycode,
    guint modifiers
);

/**
 *
 */
gboolean ibus_rustpinyin_engine_process_key_event (
    IBusEngine* engine,
    guint keyval,
    guint keycode,
    guint modifiers
) {
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
        return ibus_rustpinyin_engine_select_candidate (rustpinyin);
    case IBUS_Return:
        return ibus_rustpinyin_engine_commit_preedit (rustpinyin);

    case IBUS_Escape:
        if (rustpinyin->preedit->len == 0)
            return FALSE;

        ibus_rustpinyin_engine_clear (rustpinyin);
        return TRUE;

    case IBUS_Left:
        if (rustpinyin->preedit->len == 0)
            return FALSE;
        if (rustpinyin->cursor_pos > 0) {
            rustpinyin->cursor_pos --;
            ibus_rustpinyin_engine_update_preedit (rustpinyin);
        }
        return TRUE;

    case IBUS_Right:
        if (rustpinyin->preedit->len == 0)
            return FALSE;
        if (rustpinyin->cursor_pos < rustpinyin->preedit->len) {
            rustpinyin->cursor_pos ++;
            ibus_rustpinyin_engine_update_preedit (rustpinyin);
        }
        return TRUE;

    case IBUS_Up:
        if (rustpinyin->preedit->len == 0) {
            return FALSE;
        }

        ibus_lookup_table_cursor_up(rustpinyin->table);
        ibus_engine_update_lookup_table (
            (IBusEngine*) rustpinyin,
            rustpinyin->table,
            TRUE
        );
        return TRUE;

    case IBUS_Down:
        if (rustpinyin->preedit->len == 0) {
            return FALSE;
        }
        ibus_lookup_table_cursor_down(rustpinyin->table);
        ibus_engine_update_lookup_table (
            (IBusEngine*) rustpinyin,
            rustpinyin->table,
            TRUE
        );
        return TRUE;

    case IBUS_BackSpace:
        if (rustpinyin->preedit->len == 0)
            return FALSE;

        // in case we're in the middle of a piece by piece selection
        // of a word, pressing backspace cancels the being-made candidate
        // and put back the already consumed characters back in the preedit
        // string
        if (rustpinyin->consumed->len != 0) {
            g_string_prepend(
                rustpinyin->preedit,
                rustpinyin->consumed->str
            );
            rustpinyin->cursor_pos += rustpinyin->consumed->len;

            g_string_assign (rustpinyin->consumed, "");
            g_string_assign (rustpinyin->precommit, "");
            ibus_rustpinyin_engine_update_auxilliary(rustpinyin);
            ibus_rustpinyin_engine_update_preedit(rustpinyin);
            ibus_rustpinyin_engine_update_lookup_table (rustpinyin);
            return TRUE;
        }
        if (rustpinyin->cursor_pos > 0) {
            rustpinyin->cursor_pos --;
            g_string_erase (rustpinyin->preedit, rustpinyin->cursor_pos, 1);
            ibus_rustpinyin_engine_update_preedit(rustpinyin);
            ibus_rustpinyin_engine_update_lookup_table (rustpinyin);
        }
        return TRUE;

    case IBUS_Delete:
        if (rustpinyin->preedit->len == 0)
            return FALSE;
        if (rustpinyin->cursor_pos < rustpinyin->preedit->len) {
            g_string_erase (rustpinyin->preedit, rustpinyin->cursor_pos, 1);

            ibus_rustpinyin_engine_update_preedit(rustpinyin);
            ibus_rustpinyin_engine_update_lookup_table (rustpinyin);
        }
        return TRUE;
    }

    if (is_alpha (keyval) || is_tone(keyval)) {
        g_string_insert_c (
            rustpinyin->preedit,
            rustpinyin->cursor_pos,
            keyval
        );

        rustpinyin->cursor_pos ++;
        ibus_rustpinyin_engine_update_preedit (rustpinyin);
        ibus_rustpinyin_engine_update_lookup_table (rustpinyin);

        return TRUE;
    }

    return ibus_rustpinyin_engine_process_if_punctuation(
        engine,
        keyval,
        keycode,
        modifiers
    );


    // if nothing below return FALSE
}

// just so that the big switch case can be put one case by line
// which make it easier to understand
#define PINYIN_COMMIT(X) \
    (ibus_rustpinyin_engine_commit_string(rustpinyin,(X)))

/**
 *
 */
gboolean ibus_rustpinyin_engine_process_if_punctuation(
    IBusEngine* engine,
    guint keyval,
    guint keycode,
    guint modifiers
) {
    IBusRustPinyinEngine *rustpinyin = (IBusRustPinyinEngine *)engine;

    // behaviour copied from  ibus-pinyin
    // if the user has already started to entered some texts
    // the user will not be able to input punctuation, this to avoid
    // the punctuation to be commited before the text entered
    if (rustpinyin->preedit->len != 0) {
        return FALSE;
    }

    // switch case shamelessy copy/pasted from IBus-pinyin
    // source code (GPL V2 code normally)
    switch (keyval) {
    case '`': PINYIN_COMMIT("·"); return TRUE;
    case '~': PINYIN_COMMIT("～"); return TRUE;
    case '!': PINYIN_COMMIT("！"); return TRUE;
    // case '@':
    // case '#':
    case '$': PINYIN_COMMIT("￥"); return TRUE;
    // case '%':
    case '^': PINYIN_COMMIT("……"); return TRUE;
    // case '&':
    // case '*':
    case '(': PINYIN_COMMIT("（"); return TRUE;
    case ')': PINYIN_COMMIT("）"); return TRUE;
    // case '-':
    case '_': PINYIN_COMMIT("——"); return TRUE;
    // case '=':
    // case '+':
    case '[': PINYIN_COMMIT("【"); return TRUE;
    case ']': PINYIN_COMMIT("】"); return TRUE;
    case '{': PINYIN_COMMIT("『"); return TRUE;
    case '}': PINYIN_COMMIT("』"); return TRUE;
    case '\\': PINYIN_COMMIT("、"); return TRUE;
    case ';': PINYIN_COMMIT("；"); return TRUE;
    case ':': PINYIN_COMMIT("："); return TRUE;
    case ',': PINYIN_COMMIT("，"); return TRUE;
    case '<': PINYIN_COMMIT("《"); return TRUE;
    case '>': PINYIN_COMMIT("》"); return TRUE;
    case '?': PINYIN_COMMIT("？"); return TRUE;
    // case '|':
    //TODO handle these case, need to add a flag in rustpinyin
    //case '\'':
    //    PINYIN_COMMIT(m_quote ? "‘" : "’");
    //    m_quote = !m_quote;
    //    return TRUE;
    //case '"':
    //    PINYIN_COMMIT(m_double_quote ? "“" : "”");
    //    m_double_quote = !m_double_quote;
    //    return TRUE;
    case '.':
        //TODO: handle this case, need to add a prev_committed_char or
        // related in rustpinyin
        //if (m_prev_committed_char >= '0' && m_prev_committed_char <= '9')
        //    PINYIN_COMMIT(keyval);
        //else
            PINYIN_COMMIT("。");
        return TRUE;
    // case '/':
    }
    return FALSE;



}
