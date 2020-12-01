// frogger.c
// Patrick Malachowski

#include <allegro.h>
#include <stdbool.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define GAME_WIDTH 176
#define GAME_HEIGHT 112
#define GAME_SCALE 5
#define GAME_OFFSET_X (WINDOW_WIDTH / 2 - GAME_WIDTH * GAME_SCALE / 2)
#define GAME_OFFSET_Y (WINDOW_HEIGHT / 2 - GAME_HEIGHT * GAME_SCALE / 2)

#define FPS 60
#define ONE_FRAME (1000 / FPS)
#define GRID_WIDTH 11
#define GRID_HEIGHT 7
#define GRID_SIZE 16

#define LANES_NUM 5
#define CAR_SPEED_MIN (20.0 / FPS)
#define CAR_SPEED_RANGE (40.0 / FPS)
#define CAR_SPACE_MIN ((float)GRID_SIZE * 3.0)
#define CAR_SPACE_RANGE ((float)GRID_SIZE * 3.0)
#define CARS_PER_LANE 4

#define FROGS_NUM 5
#define FROG_JUMP_DELAY ONE_FRAME / 2
#define FROG_ANIM_DELAY ONE_FRAME * 2

#define FROG_X GRID_WIDTH / 2
#define FROG_Y GRID_HEIGHT - 1
#define UP 0 << 16
#define RIGHT 64 << 16
#define DOWN 128 << 16
#define LEFT 192 << 16

// utility functions
float randomFloat() { return (float)rand() / (float)RAND_MAX; }

int main(void) {
    // allegro boiler-plate code
    allegro_init();
    set_color_depth(16);
    set_gfx_mode(GFX_AUTODETECT_WINDOWED, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0);
    install_keyboard();
    install_timer();
    install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL);
    srand(time(NULL));

    const int BLACK = makecol(0, 0, 0);
    const int WHITE = makecol(255, 255, 255);
    const int FONT_HEIGHT = text_height(font);

    // iterator varss
    int i, j;

    // load bitmaps
    BITMAP *screen_buffer = create_bitmap(SCREEN_W, SCREEN_H),
           *game_buffer = create_bitmap(GAME_WIDTH, GAME_HEIGHT),
           *bg = load_bitmap("bg.bmp", NULL),
           *car = load_bitmap("car.bmp", NULL),
           *car2 = load_bitmap("car2.bmp", NULL),
           *frog = load_bitmap("frog.bmp", NULL),
           *dead = load_bitmap("dead.bmp", NULL);

    // music
    MIDI *music = load_midi("frogger-theme.mid");
    play_looped_midi(music, 0, -1);

    // game state
    int frogs_num = FROGS_NUM;
    int score = 0;
    int time = 0;

    // cars state
    float car_offsets[LANES_NUM] = {0};
    float car_spaces[LANES_NUM] = {0};
    float car_speeds[LANES_NUM] = {0};

    for (i = 0; i < LANES_NUM; ++i) {
        car_speeds[i] = CAR_SPEED_MIN + (CAR_SPEED_RANGE * randomFloat());
        car_spaces[i] = CAR_SPACE_MIN + (CAR_SPACE_RANGE * randomFloat());
        car_offsets[i] = car_spaces[i] * randomFloat();
    }

    // frog state
    fixed frog_direction = UP;
    int frog_x = FROG_X;
    int frog_y = FROG_Y;
    int frog_jump_delay = 0;   // FROG_JUMP_DELAY
    int frog_death_delay = 0;  // FROG_ANIM_DELAY
    int frog_score_delay = 0;  // FROG_ANIM_DELAY

    int reset_delay = 0;

    // main gameplay loop
    while (!key[KEY_ESC]) {
        // ================================================================== //
        // GAME LOGIC                                                         //
        // ================================================================== //

        // r to reset
        if(reset_delay == 0 && key[KEY_R]) {
            reset_delay = 20;
            frogs_num = FROGS_NUM;
            score = 0;
            time = 0;

            for (i = 0; i < LANES_NUM; ++i) {
                car_speeds[i] = CAR_SPEED_MIN + (CAR_SPEED_RANGE * randomFloat());
                car_spaces[i] = CAR_SPACE_MIN + (CAR_SPACE_RANGE * randomFloat());
                car_offsets[i] = car_spaces[i] * randomFloat();
            }

            frog_direction = UP;
            frog_x = FROG_X;
            frog_y = FROG_Y;
            frog_jump_delay = 0;
            frog_death_delay = 0;
            frog_score_delay = 0;
        } else if (reset_delay > 0) {
            -- reset_delay;
        }

        if (frogs_num > 0) {
            // if no input delays than move frog
            if (frog_jump_delay == 0 && frog_death_delay == 0 && frog_score_delay == 0) {
                int oldx = frog_x, oldy = frog_y;
                if (key[KEY_UP]) {
                    frog_direction = UP;
                    if (frog_y > 0) --frog_y;
                } else if (key[KEY_DOWN]) {
                    frog_direction = DOWN;
                    if (frog_y < GRID_HEIGHT - 1) ++frog_y;
                } else if (key[KEY_LEFT]) {
                    frog_direction = LEFT;
                    if (frog_x > 0) --frog_x;
                } else if (key[KEY_RIGHT]) {
                    frog_direction = RIGHT;
                    if (frog_x < GRID_WIDTH - 1) ++frog_x;
                }
                if (oldx != frog_x || oldy != frog_y)
                    frog_jump_delay = FROG_JUMP_DELAY;
            }

            // move cars
            for (i = 0; i < LANES_NUM; ++i) {
                car_offsets[i] += car_speeds[i];
                if (car_offsets[i] >= car_spaces[i] - GRID_SIZE)
                    car_offsets[i] -= car_spaces[i];
            }

            // handle car collision with frog
            if (frog_death_delay == 0 && frog_y > 0 && frog_y < GRID_HEIGHT - 1) {
                for (i = 0; i < CARS_PER_LANE; i++) {
                    float car_x = car_offsets[frog_y - 1] + (car_spaces[frog_y - 1] * i);
                    if (frog_x * GRID_SIZE + GRID_SIZE / 2 >= car_x && frog_x * GRID_SIZE <= car_x + GRID_SIZE / 2) {
                        frog_death_delay = FROG_ANIM_DELAY;
                        --frogs_num;
                        break;
                    }
                }
            }

            // handle reaching other side
            if (frog_score_delay == 0 && frog_y == 0) {
                --frogs_num;
                score += 100;
                frog_score_delay = FROG_ANIM_DELAY;
            }

            // update game
            // ONE_FRAME * 2 because * 1 was too slow, probably slowed down by rendering time
            // might have to adjust some more if time is too slow
            time += 20;

            if (frog_jump_delay > 0) --frog_jump_delay;
            if (frog_death_delay > 0) {
                --frog_death_delay;
                if (frog_death_delay == 0) {
                    frog_direction = UP;
                    frog_x = FROG_X;
                    frog_y = FROG_Y;
                }
            }
            if (frog_score_delay > 0) {
                --frog_score_delay;
                if (frog_score_delay == 0) {
                    frog_direction = UP;
                    frog_x = FROG_X;
                    frog_y = FROG_Y;
                }
            }
        }  // if (frogs_num > 0)

        // ================================================================== //
        // GAME RENDER                                                        //
        // ================================================================== //

        clear_bitmap(screen_buffer);
        // draw bg, overwrite old game_buffer
        blit(bg, game_buffer, 0, 0, 0, 0, GAME_WIDTH, GAME_HEIGHT);

        // draw cars
        for (i = 0; i < LANES_NUM; ++i) {
            for (j = 0; j < CARS_PER_LANE; j++) {
                draw_sprite(game_buffer, (i%2==0) ? car : car2, car_offsets[i] + (j * car_spaces[i]), (i + 1) * GRID_SIZE);
            }
        }

        if (frogs_num > 0) {
            // draw frog
            int offset_x = 0, offset_y = 0;
            if (frog_jump_delay > 0) {
                float offset = (float)frog_jump_delay / (float)FROG_JUMP_DELAY * (float)GRID_SIZE;
                switch (frog_direction) {
                    case UP:
                        offset_y += offset;
                        break;
                    case DOWN:
                        offset_y -= offset;
                        break;
                    case LEFT:
                        offset_x += offset;
                        break;
                    case RIGHT:
                        offset_x -= offset;
                        break;
                }
            }

            if (frog_death_delay > 0)
                draw_sprite(game_buffer, dead, frog_x * GRID_SIZE, frog_y * GRID_SIZE);
            else
                rotate_sprite(game_buffer, frog,
                              frog_x * GRID_SIZE + offset_x,
                              frog_y * GRID_SIZE + offset_y,
                              frog_direction);
        } else {
            rectfill(game_buffer, GAME_WIDTH/8, GAME_HEIGHT/4, GAME_WIDTH * 7/8, GAME_HEIGHT * 3/4, BLACK);
            rect(game_buffer, GAME_WIDTH/8, GAME_HEIGHT/4, GAME_WIDTH * 7/8, GAME_HEIGHT * 3/4, WHITE);

            int time_bonus = 120 - (time/1000);
            if(time_bonus < 0) time_bonus = 0;

            textprintf_centre_ex(game_buffer, font, GAME_WIDTH / 2, (GAME_HEIGHT / 2) - (FONT_HEIGHT * 3), WHITE, -1,
                              "GAME OVER");
            textprintf_centre_ex(game_buffer, font, GAME_WIDTH / 2, (GAME_HEIGHT / 2) - (FONT_HEIGHT * 2), WHITE, -1,
                              "score: %d", score);
            textprintf_centre_ex(game_buffer, font, GAME_WIDTH / 2, (GAME_HEIGHT / 2) - FONT_HEIGHT, WHITE, -1,
                              "time: %ds", time/1000);
            textprintf_centre_ex(game_buffer, font, GAME_WIDTH / 2, (GAME_HEIGHT / 2), WHITE, -1,
                              "time bonus: %d", time_bonus);
            textprintf_centre_ex(game_buffer, font, GAME_WIDTH / 2, (GAME_HEIGHT / 2) + (FONT_HEIGHT * 2), WHITE, -1,
                              "TOTAL: %d", score + time_bonus);
        }

        // draw game_buffer to screen_buffer
        stretch_blit(game_buffer, screen_buffer, 0, 0,
                     GAME_WIDTH, GAME_HEIGHT, GAME_OFFSET_X, GAME_OFFSET_Y,
                     GAME_WIDTH * GAME_SCALE, GAME_HEIGHT * GAME_SCALE);

        if (frogs_num == 0) {
            //void rectfill(BITMAP *bmp, int x1, int y1, int x2, int y2, int color);

        } else {
            // draw hud to screen_buffer
            textprintf_centre_ex(screen_buffer, font,
                              SCREEN_W / 2, GAME_OFFSET_Y - FONT_HEIGHT - 5, WHITE, -1,
                              "FROGS: %d   SCORE: %d   TIME: %d",
                              frogs_num, score, time / 1000);
        }

        textprintf_ex(screen_buffer, font,
                              5, WINDOW_HEIGHT - FONT_HEIGHT - 5, WHITE, -1,
                              "[esc] to close, [r] to reset");

        /* // debug
        textprintf_centre_ex(screen_buffer, font,
                             SCREEN_W / 2, GAME_OFFSET_Y - ((FONT_HEIGHT + 5) * 2), WHITE, -1,
                             "jump: %d   death: %d   score: %d",
                             frog_jump_delay, frog_death_delay, frog_score_delay);
        */

        // finally, draw screen_buffer to actual screen
        acquire_screen();
        blit(screen_buffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
        release_screen();

        rest(10);
    }

    allegro_exit();
    return 0;
}
END_OF_MAIN()
