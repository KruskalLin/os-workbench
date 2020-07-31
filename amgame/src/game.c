#include <game.h>

extern uint8_t isJumping;
extern uint8_t isFalling;

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


// Operating system is a C program!
int main(const char *args) {
    _ioe_init();

    init_video(); // init width and height
    init_player(); // set player in the middle of screen
    generate_blocks(); // generate the jump blocks(malloc the memory)

    int next_frame = uptime();

    while (1) {
        while (uptime() < next_frame) ;
        srand(uptime() % 100);
        clear_video(); // use one black pixel to update whole screen
        const char* str = read_keys();
        if(strcmp(str, "ESC") == 0 || strcmp(str, "ESCAPE") == 0) {
            _halt(0);
        } else if(strcmp(str, "RIGHT") == 0) {
            move_right();
        } else if(strcmp(str, "LEFT") == 0) {
            move_left();
        }

        if (isJumping) check_jump(); // jump
        if (isFalling) check_fall(); // fall
        draw_mario(); // draw according the pixel array
        draw_blocks();
        check_collision(); // check if the mario(player) is colliding the block. If so set jump true

        next_frame += 1000 / FPS;
    }

    return 0;
}
