//
// Created by kruskallin on 2020/3/18.
//
#include <game.h>

extern uint32_t w, h;
extern uint32_t x, y;
extern uint32_t jumpSpeed;
extern uint8_t isJumping;
extern uint8_t isFalling;
extern uint32_t points;

block_t* blocks[MAX_BLOCKS];

block_t* generate_block(uint32_t x_, uint32_t y_) {
    block_t* block = (block_t*)malloc(sizeof(block_t));
    block->x = x_;
    block->y = y_;
    block->type = rand() % 2;
    block->isMoving = rand() % 2;
    block->direction = rand() % 2;
    return block;
}

void generate_blocks() {
    uint32_t position = 0;
    for (int i = 0; i < MAX_BLOCKS; i++) {
        blocks[i] = generate_block((int)(rand() * 1.0 / 32767 * (w - BLOCK_WIDTH)), position);
        if (position < h - BLOCK_HEIGHT)
            position += (int)(h / MAX_BLOCKS);
    }
}

void check_collision() {
        for (int i = 0; i < MAX_BLOCKS; i++) {
            if(
                    (isFalling) &&
                    (x < blocks[i]->x + BLOCK_WIDTH) &&
                    (x + MARIO_WIDTH> blocks[i]->x) &&
                    (y + MARIO_HEIGHT > blocks[i]->y) &&
                    (y + MARIO_HEIGHT < blocks[i]->y + BLOCK_HEIGHT + SPEED_MAX)
                    ) {
                fall_stop();
                if(blocks[i]->type == 1) {
                    jumpSpeed = SPEED_SUPERMAX;
                }
            }

        }
}

void draw_block(block_t* block) {
    uint32_t pixels[BLOCK_WIDTH * BLOCK_HEIGHT];
    if(block->type == 0) {
        for(int i = 0; i < BLOCK_WIDTH * BLOCK_HEIGHT; i++) {
            pixels[i] = TYPE_ONE_COLOR;
        }
    } else {
        for(int i = 0; i < BLOCK_WIDTH * BLOCK_HEIGHT; i++) {
            pixels[i] = TYPE_TWO_COLOR;
        }
    }
    _DEV_VIDEO_FBCTRL_t event = {
            .x = block->x, .y = block->y, .w = BLOCK_WIDTH, .h = BLOCK_HEIGHT, .sync = 1,
            .pixels = pixels,
    };
    _io_write(_DEV_VIDEO, _DEVREG_VIDEO_FBCTRL, &event, sizeof(event));
}

void draw_blocks() {
    for (int i = 0; i < MAX_BLOCKS; i++) {
        if (blocks[i]->isMoving) {
            if (blocks[i]->x < 0) {
                blocks[i]->direction = 1;
            } else if (blocks[i]->x > w - BLOCK_WIDTH) {
                blocks[i]->direction = 0;
            }
            if(blocks[i]->direction == 1) {
                blocks[i]->x += (int)((i / 2) * (points / 100));
            } else {
                blocks[i]->x -= (int)((i / 2) * (points / 100));
            }
        }
        draw_block(blocks[i]);
    }
}