#include <glib/gstdio.h>
#include <ibus.h>
#include <pinyinengine.h>

#include "user_db.h"

static void *db = NULL;
static void *userDb = NULL;
// keep track if we've modified the user database
// since last dump, to avoid dumping twice the same
// database
static gboolean userDbModified = FALSE;

static gchar* rust_pinyin_user_db_path();
static void rust_pinyin_create_user_db_if_not_exist(const gchar* path);

/**
 *
 */
void open_main_db() {
    if (db != NULL) {
        return; 
    }
    db = db_new(PKGDATADIR"/data/filtered_db.csv");
}

/**
 *
 */
void open_user_db() {

    if (userDb != NULL) {
        return;
    }
    gchar* userDbPath = rust_pinyin_user_db_path();
    rust_pinyin_create_user_db_if_not_exist(userDbPath);

    userDb = db_new(userDbPath);

    g_free(userDbPath);

    db_update_with_user_db(db, userDb);
}

/**
 *
 */
static gchar* rust_pinyin_user_db_path () {
    return g_build_filename(
        g_get_user_cache_dir(),
        "ibus",
        "rustpinyin",
        "user_data.csv",
        NULL
    );
}

/**
 *
 */
static void rust_pinyin_create_user_db_if_not_exist (
    const gchar* path
) {
    if (g_file_test(path, G_FILE_TEST_EXISTS)) {
        return;
    }
    GError *tmp_error;

    gchar* dirname = g_path_get_dirname(path);
    g_mkdir_with_parents(dirname, 0750);

    // we create the file as an empty one if it does not exists
    // (hence while we gcreate and gclose directly)
    g_close(g_creat(path), &tmp_error);
    g_free(dirname);
}

/**
 *
 */
void* main_db_get_suggestions_from_str(const char* preedit) {
    return pinyin2suggestions_c(
        db,
        preedit
    );
}

/**
 *
 */
void dump_user_db() {
    // to avoid useless dump
    if (userDbModified == FALSE) {
        return;
    }
    printf("we dump\n");
    gchar* userDbPath = rust_pinyin_user_db_path();
    db_dump(userDb, userDbPath);
    g_free(userDbPath);

    userDbModified = FALSE;
}

/**
 *
 */
void rust_pinyin_update_db_with_word(
    const gchar* pinyin,
    const gchar* sinograms
) {
    userDbModified = TRUE;
    db_update_with_word(db, pinyin, sinograms);
    db_update_with_word(userDb, pinyin, sinograms);
}
