/* vim:set et sts=4: */

#include <stdio.h>
#include "rustpinyin.h"
#include "engine.h"
#include "process_key_event.h"
static void *db = NULL;

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

    if (db == NULL) {
        db =  db_new(PKGDATADIR"/data/filtered_db.csv");
    }

    rustpinyin->preedit = g_string_new ("");
    rustpinyin->cursor_pos = 0;

    rustpinyin->table = ibus_lookup_table_new (9, 0, TRUE, TRUE);
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

    if (rustpinyin->table) {
        g_object_unref (rustpinyin->table);
        rustpinyin->table = NULL;
    }

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

    gboolean retval;

    if (rustpinyin->preedit->len == 0) {
        ibus_engine_hide_lookup_table ((IBusEngine *) rustpinyin);
        return;
    }

    ibus_lookup_table_clear (rustpinyin->table);
    

    void* suggestions = pinyin2suggestions_c(
        db,
        rustpinyin->preedit->str
    );
    unsigned n_sug = suggestions_size(suggestions);

    if (suggestions == NULL || n_sug == 0) {
        ibus_engine_hide_lookup_table ((IBusEngine *) rustpinyin);
        return;
    }

    for (unsigned i = 0; i < n_sug; i++) {
        const gchar* value = suggestions_value_get(
            suggestions,
            i
        );
        ibus_lookup_table_append_candidate (
            rustpinyin->table,
            ibus_text_new_from_string (value)
        );

        suggestions_value_free(value);
    }

    ibus_engine_update_lookup_table (
        (IBusEngine*) rustpinyin,
        rustpinyin->table,
        TRUE
    );

    if (suggestions != NULL) {
        suggestions_free(suggestions);
    }
}

/**
 *
 */
void ibus_rustpinyin_engine_update_preedit (
    IBusRustPinyinEngine *rustpinyin
) {
    IBusText *text;
    gint retval;

    text = ibus_text_new_from_static_string (rustpinyin->preedit->str);
    text->attrs = ibus_attr_list_new ();
    
    ibus_attr_list_append (
        text->attrs,
        ibus_attr_underline_new (
            IBUS_ATTR_UNDERLINE_SINGLE,
            0,
            rustpinyin->preedit->len
        )
    );
    //TODO : need to check what this is doing
    //if (rustpinyin->preedit->len > 0) {
    //    retval = rustpinyin_dict_check (dict, rustpinyin->preedit->str, rustpinyin->preedit->len);
    //    if (retval != 0) {
    //        ibus_attr_list_append (text->attrs,
    //                           ibus_attr_foreground_new (0xff0000, 0, rustpinyin->preedit->len));
    //    }
    //}
    
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
    
    ibus_rustpinyin_engine_commit_string (rustpinyin, rustpinyin->preedit->str);
    g_string_assign (rustpinyin->preedit, "");
    rustpinyin->cursor_pos = 0;

    ibus_rustpinyin_engine_update (rustpinyin);

    return TRUE;
}

/**
 *
 */
gboolean ibus_rustpinyin_engine_commit_candidate(
    IBusRustPinyinEngine *rustpinyin
) {
    if (rustpinyin->preedit->len == 0) {
        return FALSE;
    }

    IBusText* candidate = ibus_lookup_table_get_candidate(
        rustpinyin->table,
        ibus_lookup_table_get_cursor_pos(rustpinyin->table)
    );
    if (candidate != NULL) {
        ibus_engine_commit_text ((IBusEngine *)rustpinyin, candidate);
    } else {
        ibus_rustpinyin_engine_commit_string (rustpinyin, rustpinyin->preedit->str);
    }

    g_string_assign (rustpinyin->preedit, "");
    rustpinyin->cursor_pos = 0;

    ibus_rustpinyin_engine_update (rustpinyin);

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
void ibus_rustpinyin_engine_update (
    IBusRustPinyinEngine *rustpinyin
) {
    ibus_rustpinyin_engine_update_preedit (rustpinyin);
    ibus_engine_hide_lookup_table ((IBusEngine*) rustpinyin);
}

