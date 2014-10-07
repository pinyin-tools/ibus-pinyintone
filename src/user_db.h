#ifndef RUST_PINYIN_USER_DB_H
#define RUST_PINYIN_USER_DB_H

// functions to abstract to other C files that actually the database
// are stored in ugly global variables

void open_user_db();
void dump_user_db();

void open_main_db();

void rust_pinyin_update_db_with_word(
    const char* pinyin,
    const char* sinograms
);

void* main_db_get_suggestions_from_str(
    const char* preedit
);


#endif 
