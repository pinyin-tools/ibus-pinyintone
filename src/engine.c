/* vim:set et sts=4: */

#include <stdio.h>
#include <string.h>
#include <pinyinengine.h>
#include "engine.h"
#include "process_key_event.h"
#include "user_db.h"

G_DEFINE_TYPE (
    IBusRustPinyinEngine,
    ibus_rustpinyin_engine,
    IBUS_TYPE_ENGINE
)

static void ibus_rustpinyin_engine_class_init(
    IBusRustPinyinEngineClass *klass
);

/**
 *
 */
static void ibus_rustpinyin_engine_class_init (IBusRustPinyinEngineClass *klass) {

    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (klass);
    IBusEngineClass *engine_class = IBUS_ENGINE_CLASS (klass);

    ibus_object_class->destroy = (IBusObjectDestroyFunc) ibus_rustpinyin_engine_destroy;

    engine_class->process_key_event = ibus_rustpinyin_engine_process_key_event;
}

/**
 *
 */
void ibus_rustpinyin_engine_init (IBusRustPinyinEngine *rustpinyin) {
    // open the database if they are not yet open
    open_main_db();
    open_user_db();

    rustpinyin->precommit = g_string_new ("");
    rustpinyin->preedit = g_string_new ("");
    rustpinyin->consumed = g_string_new ("");
    rustpinyin->cursor_pos = 0;

    rustpinyin->direct_input = FALSE;
    rustpinyin->prev_key_pressed = IBUS_VoidSymbol;

    rustpinyin->simple_quote_start = TRUE;
    rustpinyin->double_quote_start = TRUE;

    rustpinyin->table = ibus_lookup_table_new (5, 0, TRUE, TRUE);
    g_object_ref_sink (rustpinyin->table);
}

/**
 *
 */
void ibus_rustpinyin_engine_destroy (IBusRustPinyinEngine *rustpinyin)
{
    if (rustpinyin->preedit) {
        g_string_free (rustpinyin->preedit, TRUE);
        rustpinyin->preedit = NULL;
    }

    if (rustpinyin->precommit) {
        g_string_free (rustpinyin->precommit, TRUE);
        rustpinyin->precommit = NULL;
    }

    if (rustpinyin->table) {
        g_object_unref (rustpinyin->table);
        rustpinyin->table = NULL;
    }

    if (rustpinyin->consumed) {
        g_string_free (rustpinyin->consumed, TRUE);
        rustpinyin->consumed = NULL;
    }

    dump_user_db();

    ((IBusObjectClass *) ibus_rustpinyin_engine_parent_class)->destroy(
        (IBusObject *)rustpinyin
    );
}

/**
 *
 */
void ibus_rustpinyin_engine_update_lookup_table (
    IBusRustPinyinEngine* rustpinyin
) {

    if (rustpinyin->preedit->len == 0) {
        ibus_engine_hide_lookup_table ((IBusEngine *) rustpinyin);
        return;
    }

    ibus_lookup_table_clear (rustpinyin->table);
    

    // we get the suggestions and the number of it
    void* suggestions = main_db_get_suggestions_from_str(
        rustpinyin->preedit->str
    );
    unsigned n_sug = vec_string_size(suggestions);


    if (suggestions == NULL || n_sug == 0) {
        ibus_engine_hide_lookup_table ((IBusEngine *) rustpinyin);
        return;
    }

    // we add the suggestions one by one in lookup table
    for (unsigned i = 0; i < n_sug; i++) {
        const gchar* value = vec_string_value_get(
            suggestions,
            i
        );
        ibus_lookup_table_append_candidate (
            rustpinyin->table,
            ibus_text_new_from_string (value)
        );

        vec_string_value_free(value);
    }

    // once finished we refresh the UI
    ibus_engine_update_lookup_table (
        (IBusEngine*) rustpinyin,
        rustpinyin->table,
        TRUE
    );

    if (suggestions != NULL) {
        vec_string_free(suggestions);
    }
}

/**
 *
 */
void ibus_rustpinyin_engine_update_auxilliary(
    IBusRustPinyinEngine *rustpinyin
) {
    IBusText *auxtext;
    auxtext = ibus_text_new_from_static_string (
        rustpinyin->precommit->str
    );

    ibus_engine_update_auxiliary_text(
        (IBusEngine*) rustpinyin,
        auxtext,
        TRUE
    );

}

/**
 *
 */
void ibus_rustpinyin_engine_update_preedit (
    IBusRustPinyinEngine *rustpinyin
) {
    IBusText *text;

    //mirror of preedit, only used for display purpose
    //in the interface, with modification (space, '|' cursor etc.)
    GString* preedit_display = g_string_new(rustpinyin->preedit->str);
    //except for empty preedit, we had a visual cursor
    //in the preedit string
    if (rustpinyin->preedit->len != 0) {
        g_string_insert_c(
            preedit_display,
            rustpinyin->cursor_pos,
            '|'
        );
    }

    text = ibus_text_new_from_static_string (
        preedit_display->str
    );

    text->attrs = ibus_attr_list_new ();
    
    ibus_attr_list_append (
        text->attrs,
        ibus_attr_underline_new (
            IBUS_ATTR_UNDERLINE_SINGLE,
            0,
            rustpinyin->preedit->len
        )
    );
    
    ibus_engine_update_preedit_text (
        (IBusEngine*) rustpinyin,
        text,
        rustpinyin->cursor_pos,
        TRUE
    );

}

/* commit preedit to client and update preedit */
/**
 *
 */
gboolean ibus_rustpinyin_engine_commit_preedit(
    IBusRustPinyinEngine *rustpinyin
) {
    if (rustpinyin->preedit->len == 0) {
        return FALSE;
    }
    ibus_rustpinyin_engine_commit_string (
        rustpinyin,
        rustpinyin->preedit->str
    );


    ibus_rustpinyin_engine_clear (rustpinyin);

    return TRUE;
}

/**
 *
 */
gboolean ibus_rustpinyin_engine_select_candidate(
    IBusRustPinyinEngine *rustpinyin
) {
    if (rustpinyin->preedit->len == 0) {
        return FALSE;
    }

    IBusText* candidate = ibus_lookup_table_get_candidate(
        rustpinyin->table,
        ibus_lookup_table_get_cursor_pos(rustpinyin->table)
    );


    // if no candidate, we commit the preedit string and insert a space
    // useful if you want to type for example an english word here and
    // there without needing to switch the IME
    // (Hack as right now this function is only called when pressing
    // enter)
    if (candidate == NULL) {
        ibus_rustpinyin_engine_commit_string (
            rustpinyin,
            rustpinyin->preedit->str
        );
        ibus_rustpinyin_engine_commit_string (rustpinyin, " ");
        ibus_rustpinyin_engine_clear (rustpinyin);
        return TRUE;
    }


    guint candidate_size = ibus_text_get_length(candidate);

    g_string_append(
        rustpinyin->precommit,
        ibus_text_get_text(candidate)
    );

    // we decompose the preedit string into pinyin tokens
    // e.g "ni3hao3aaaaa"  will return a vector of size 2
    // made of  "ni3"  and "hao3"
    void* tokens = string_to_tokens_as_strings_c(
        rustpinyin->preedit->str
    );
    guint tokens_size = vec_string_size(tokens);

    // check how many characters have been already consumed
    guint consumed_chars = 0;
    for (guint i = 0; i < candidate_size; i++) {
        const gchar* value = vec_string_value_get(
            tokens,
            i
        );
        consumed_chars += strlen(value);
        // we add the pinyin of the candidate
        // to the string of consumed characters
        g_string_append(
            rustpinyin->consumed,
            value
        );
         
        vec_string_value_free(value);
    }
    vec_string_free(tokens);

    // we removed the consumed characters
    g_string_erase(
        rustpinyin->preedit,
        0,
        consumed_chars
    );
    // we update the cursor_pos so that for the user
    // it stays at the same place
    rustpinyin->cursor_pos -= consumed_chars;

    // if we have consumed all the tokens, then we commit the string
    if (candidate_size == tokens_size) {

        ibus_rustpinyin_engine_commit_string(
            rustpinyin,
            rustpinyin->precommit->str
        );

        // we give a feed back to our internal database
        // about the word we have chosen, so that it can
        // update the frequency of this word etc.
        rust_pinyin_update_db_with_word(
            rustpinyin->consumed->str,
            rustpinyin->precommit->str
        );

        ibus_rustpinyin_engine_clear (rustpinyin);

        return TRUE;
    }

    // update UI
    ibus_rustpinyin_engine_update_auxilliary(rustpinyin);
    ibus_rustpinyin_engine_update_preedit (rustpinyin);
    ibus_rustpinyin_engine_update_lookup_table (rustpinyin);
    return TRUE;
}

/**
 *
 */
void ibus_rustpinyin_engine_commit_string (
    IBusRustPinyinEngine* rustpinyin,
    const gchar* string
) {
    IBusText *text;
    text = ibus_text_new_from_static_string (string);
    ibus_engine_commit_text ((IBusEngine *)rustpinyin, text);
}

/**
 *
 */
void ibus_rustpinyin_engine_clear (
    IBusRustPinyinEngine *rustpinyin
) {

    g_string_assign (rustpinyin->preedit, "");
    rustpinyin->cursor_pos = 0;
    rustpinyin->prev_key_pressed = IBUS_VoidSymbol;

    g_string_assign (rustpinyin->precommit, "");
    g_string_assign (rustpinyin->consumed, "");

    ibus_rustpinyin_engine_update_auxilliary(rustpinyin);
    ibus_rustpinyin_engine_update_preedit (rustpinyin);
    ibus_engine_hide_auxiliary_text ((IBusEngine*) rustpinyin);
    ibus_engine_hide_preedit_text ((IBusEngine*) rustpinyin);
    ibus_engine_hide_lookup_table ((IBusEngine*) rustpinyin);
}

