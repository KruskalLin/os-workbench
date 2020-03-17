#include <game.h>
const char* read_keys() {
    _DEV_INPUT_KBD_t event = { .keycode = _KEY_NONE };
    #define KEYNAME(key) \
    [_KEY_##key] = #key,
    static const char *key_names[] = {
            _KEYS(KEYNAME)
    };
    _io_read(_DEV_INPUT, _DEVREG_INPUT_KBD, &event, sizeof(event));
    if (event.keycode != _KEY_NONE && event.keydown) {
        return key_names[event.keycode];
    } else {
        return NULL;
    }
}