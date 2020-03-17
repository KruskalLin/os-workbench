#include <game.h>
#include <game-macros.h>

#define SIDE 16
static int w, h;

void init_video() {
    _DEV_VIDEO_INFO_t info = {0};
    _io_read(_DEV_VIDEO, _DEVREG_VIDEO_INFO, &info, sizeof(info));
    w = info.width;
    h = info.height;
}

//static void draw_tile(int x, int y, int w, int h, uint32_t color) {
//    uint32_t pixels[w * h]; // careful! stack is limited!
//    _DEV_VIDEO_FBCTRL_t event = {
//            .x = x, .y = y, .w = w, .h = h, .sync = 1,
//            .pixels = pixels,
//    };
//    for (int i = 0; i < w * h; i++) {
//        pixels[i] = color;
//    }
//    _io_write(_DEV_VIDEO, _DEVREG_VIDEO_FBCTRL, &event, sizeof(event));
//}

/**
 *
 * @param type: 0 indicates left type of mario; 1 indicates right type of mario;
 */
void draw_mario(int x, int y, int type) {
    if(type == 0) {
        _DEV_VIDEO_FBCTRL_t event = {
                .x = x, .y = y, .w = MARIO_WIDTH, .h = MARIO_HEIGHT, .sync = 1,
                .pixels = mario_l,
        };
        _io_write(_DEV_VIDEO, _DEVREG_VIDEO_FBCTRL, &event, sizeof(event));
    } else {
        _DEV_VIDEO_FBCTRL_t event = {
                .x = x, .y = y, .w = MARIO_WIDTH, .h = MARIO_HEIGHT, .sync = 1,
                .pixels = mario_r,
        };
        _io_write(_DEV_VIDEO, _DEVREG_VIDEO_FBCTRL, &event, sizeof(event));
    }
}
