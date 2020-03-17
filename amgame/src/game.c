#include <game.h>

// Operating system is a C program!
int main(const char *args) {
    _ioe_init();
    puts("mainargs = \"");
    puts(args); // make run mainargs=xxx
    puts("\"\n");

    init_video();
    while (1) {
        if(strcmp(read_keys(), "ESC") == 0) {
            _halt(0);
        } else if(strcmp(read_keys(), "RIGHT") == 0) {
            draw_mario(0, 0, 1);
        } else if(strcmp(read_keys(), "LEFT") == 0) {
            draw_mario(0, 0, 0);
        }
    }
    return 0;
}
