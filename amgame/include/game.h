#include <am.h>
#include <amdev.h>
#include <klib.h>
#include <klib-macros.h>

#define FPS 16
#define SPEED_MAX 32
#define SPEED_SUPERMAX 64
#define MOVE 10
#define MARIO_WIDTH 16
#define MARIO_HEIGHT 16
#define MAX_BLOCKS 10
#define BLOCK_WIDTH 64
#define BLOCK_HEIGHT 16
#define BACKGROUND 0x000000
#define TYPE_ONE_COLOR 0xff8c00
#define TYPE_TWO_COLOR 0xaadd00


typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t type;
    uint8_t isMoving;
    uint8_t direction;
} block_t;

extern uint32_t w, h;
extern uint32_t points;
extern uint32_t x;
extern uint32_t y;
extern uint32_t jumpSpeed;
extern uint32_t fallSpeed;
extern uint8_t isJumping;
extern uint8_t isFalling;
extern uint8_t isLeft;
extern block_t* blocks[MAX_BLOCKS];


static inline void puts(const char *s) {
  for (; *s; s++) _putc(*s);
}

/* video.c */
void init_video();
void clear_video();
void draw_mario();

/* player.c */
void init_player();
void set_position(uint32_t x_, uint32_t y_);
void jump();
void fall_stop();
void check_jump();
void check_fall();
void move_right();
void move_left();
void game_over();

/* block.c */
void generate_blocks();
block_t* generate_block(uint32_t x_, uint32_t y_);
void check_collision();
void draw_blocks();