#ifndef RUST_PINYIN_PROCESS_KEY_EVENT_H
#define RUST_PINYIN_PROCESS_KEY_EVENT_H

#include <ibus.h>

gboolean ibus_rustpinyin_engine_process_key_event(
    IBusEngine *engine,
    guint keyval,
    guint keycode,
    guint modifiers
);

#endif
