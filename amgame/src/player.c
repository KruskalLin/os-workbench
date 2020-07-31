//
// Created by kruskallin on 2020/3/18.
//

#include <game.h>

extern uint32_t w, h;
extern block_t* blocks[MAX_BLOCKS];
extern uint8_t isLeft;

uint32_t points = 0;
uint32_t x = 0;
uint32_t y = 0;
uint32_t jumpSpeed = 0;
uint32_t fallSpeed = SPEED_MAX;
uint8_t isJumping = 0;
uint8_t isFalling = 1;

void init_player() {
    set_position((int)((w - MARIO_WIDTH)/2), h - MARIO_HEIGHT);
    jump();
}


void jump() {
    if (!isJumping && !isFalling) {
        fallSpeed = 0;
        isJumping = 1;
        jumpSpeed = SPEED_MAX;
    }
}

void check_jump() {
    if (y > h * 0.4) {
        set_position(x, y - jumpSpeed); // if too high, set the position of player
    } else { // else move the block down
        if (jumpSpeed > 10)
            points++;

        for(int i = 0; i < MAX_BLOCKS; i++) {
            blocks[i]->y += jumpSpeed;
            if (blocks[i]->y > h) {
                blocks[i] = generate_block((int)(rand() * 1.0 / 32767 * (w - BLOCK_WIDTH)), blocks[i]->y - h);
            }
        }
    }

    jumpSpeed--;
    if (jumpSpeed == 0) {
        isJumping = 0;
        isFalling = 1;
        fallSpeed = 1;
    }

}

void fall_stop() {
    isFalling = 0;
    fallSpeed = 0;
    jump();
}

void check_fall() {
    if (y < h - MARIO_HEIGHT) {
        set_position(x, y + fallSpeed);
        fallSpeed++;
    } else {
//        if (points == 0)
        fall_stop();
//        else
//            game_over();
    }
}

void move_left(){
    isLeft = 1;
    if (x >= MOVE) {
        set_position(x - MOVE, y);
    }
}

void move_right() {
    isLeft = 0;
    if (x + MOVE + MARIO_WIDTH < w) {
        set_position(x + MOVE, y);
    }
}

void set_position(uint32_t x_, uint32_t y_) {
    x = x_;
    y = y_;
}

void game_over() {
    _halt(0);
}