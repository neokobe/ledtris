// Copyright (c) 2020 NeoKobe
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "led-matrix-c.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <SDL.h>
#include <sys/time.h>

#define AI_VERSION 1

#define STATE_OVER  0x1
#define STATE_PAUSE 0x2
#define STATE_DEMO  0x4
#define STATE_PLAY  0x8
#define STATE_MASK_NO_INPUT 0x7

#define PTYPE_I 1
#define PTYPE_T 2
#define PTYPE_Z 3
#define PTYPE_S 4
#define PTYPE_O 5
#define PTYPE_J 6
#define PTYPE_L 7

#define DROP_FREQ_MIN      50LL
#define DROP_FREQ_MAX      500LL
#define AUTOPLAY_SPEED     50LL
#define MILLIS_UNTIL_DEMO  10000LL
#define MILLIS_TIL_BTN_RPT 200LL
#define SYNC_ANIM_DELAY ((long long)((1.0/20.0)*1000000.0))

#define CLR(r,g,b) ((r&0xff)<<16|(g&0xff)<<8|(b&0xff))

#define CLR_FIELD     CLR(0x40,0x40,0xff)
#define CLR_GMOVR_BRK CLR(0xff,0x10,0x10)
#define CLR_TEXT      CLR(0x80,0x80,0x80)
#define CLR_BG        CLR(0x00,0x00,0x00)
#define CLR_COVER_1   CLR(0x80,0x00,0x00)
#define CLR_COVER_2   CLR(0x20,0x00,0x00)

#define LOG(args...) fprintf(stderr, ##args)

static struct RGBLedMatrix *matrix;
static struct LedCanvas *canvas;

volatile int interrupt_received = 0;

typedef unsigned char bool;

static void InterruptHandler(int signo) {
  interrupt_received = 1;
}

const unsigned int chars_packed[] = {
  0x4c6cee6a,
  0xaa8a888a,
  0xac8acc8a,
  0xea8a88ae,
  0xac6ce86a,
  0x42a8aa4c,
  0x42a8eaaa,
  0x42a8aeaa,
  0x42c8aaac,
  0x4ca6aa48,
  0x4c6eaaaa,
  0xaa84aaaa,
  0xaa44aaea,
  0xac24aae4,
  0x6ac4464a,
  0xae000000,
  0xa2000000,
  0xa4000000,
  0x48000000,
  0x4e000000,
  // numbers
  0x4ccc2e6e,
  0xa422a882,
  0xe444ace2,
  0xa482e2a4,
  0x44ec2c48,
  0x44000000,
  0xaa000000,
  0x46000000,
  0xa2000000,
  0x4c000000,
};

const int piece_colors[] = {
  CLR(0xee,0xae,0x01), // I #EEAE01
  CLR(0x38,0x87,0x25), // T #4D7DD9
  CLR(0xda,0x1c,0x00), // Z #DA1C00
  CLR(0xcb,0x20,0xd8), // S #CB20D8
  CLR(0x41,0x7a,0xcc), // O #4A7ACC
  CLR(0xa4,0xa5,0xe7), // J #A4A5E7
  CLR(0xe9,0x7e,0x01), // L #E97E01

  CLR(0xff,0x00,0x00), // I #ff0000
  CLR(0xfb,0xb0,0x34), // T #fbb034
  CLR(0xff,0xdd,0x00), // Z #ffdd00
  CLR(0xc1,0xd8,0x2f), // S #c1d82f
  CLR(0x00,0xa4,0xe4), // O #00a4e4
  CLR(0x8a,0x79,0x67), // J #8a7967
  CLR(0x6a,0x73,0x7b), // L #6a737b

  CLR(0xbe,0x00,0x27), // I #be0027
  CLR(0xcf,0x8d,0x2e), // T #cf8d2e
  CLR(0xe4,0xe9,0x32), // Z #e4e932
  CLR(0x2c,0x9f,0x45), // S #2c9f45
  CLR(0x37,0x17,0x77), // O #371777
  CLR(0x52,0x32,0x5d), // J #52325d
  CLR(0x44,0x44,0x44), // L #444444

  CLR(0x03,0x7e,0xf3), // I #037ef3
  CLR(0x00,0xc1,0x6e), // T #00c16e
  CLR(0x0c,0xb9,0xc1), // Z #0cb9c1
  CLR(0xf4,0x89,0x24), // S #f48924
  CLR(0xf8,0x5a,0x40), // O #f85a40
  CLR(0xff,0xc8,0x45), // J #ffc845
  CLR(0xca,0xcc,0xd1), // L #caccd1

  CLR(0x00,0xae,0xff), // I #00aeff
  CLR(0x33,0x69,0xe7), // T #3369e7
  CLR(0x8e,0x43,0xe7), // Z #8e43e7
  CLR(0xb8,0x45,0x92), // S #b84592
  CLR(0xff,0x4f,0x81), // O #ff4f81
  CLR(0xff,0x6c,0x5f), // J #ff6c5f
  CLR(0xff,0xc1,0x68), // L #ffc168
};

const int piece_rots[] = {
  0,
  2, // I
  4, // T
  2, // Z
  2, // S
  1, // O
  4, // J
  4, // L
};

const short game_over_bmp[] = {
  0x3253,
  0x4574,
  0x4556,
  0x5754,
  0x3553,
  0x0000,
  0x2536,
  0x5545,
  0x5565,
  0x5546,
  0x2235,
};

struct piece {
  int bitmap[4][4];
  int type;
  int rot;
  int x, y;
};

struct field {
  int bitmap[26][16];
  int h;
  int w;
};

struct piece_state {
  int score;
  int ht;
  int x;
  int y;
  int rot;
};

struct state {
  long long last_drop;
  int game_state;
  long long last_game_state_change;
  int drop_freq;
  int level;
  int lines_cleared;
  struct field field;
  struct piece piece;
  int next_piece[8];
  const struct piece_state *suggestion;
  long long last_automove;
  long long next_automove_delta;
};

const struct piece_state* ai_suggest(const struct piece *piece, const struct field *field);
void animate_game_start(struct state *state);
void animate_game_over(struct state *state);

long long millis() {
  static struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec) * 1000LL+(tv.tv_usec) / 1000LL;
}

const char* millis_to_text(long long millis) {
  static char buffer[128];
  int secs_total = (int) (millis / 1000LL);
  int secs = secs_total % 60; secs_total /= 60;
  int mins = secs_total % 60; secs_total /= 60;
  int hours = secs_total % 24; secs_total /= 24;

  snprintf(buffer, sizeof(buffer), "%dd, %02d:%02d:%02d", secs_total,
    hours, mins, secs);

  return buffer;
}

void set_game_state(struct state *state, int game_state) {
  long long tick = millis();
  LOG("---\n\nSTATE: %d >> %d (%s)\n\n---\n",
    state->game_state, game_state,
    millis_to_text(tick - state->last_game_state_change));
  if ((state->game_state == STATE_DEMO
    || state->game_state == STATE_PLAY)
    && game_state == STATE_OVER) {
    animate_game_over(state);
  } else if (state->game_state == STATE_OVER
    && (game_state == STATE_DEMO
      || game_state == STATE_PLAY)) {
    animate_game_start(state);
  } else if (state->game_state == STATE_DEMO
  	&& game_state == STATE_PLAY) {
    animate_game_over(state);
    animate_game_start(state);
  }
  state->game_state = game_state;
  state->last_game_state_change = tick;
}

// https://benpfaff.org/writings/clc/shuffle.html
void shuffle(int *array, size_t n)
{
  if (n > 1) {
    size_t i;
    for (i = 0; i < n-1; i++) {
      size_t j = i+rand() / (RAND_MAX / (n-i)+1);
      int t = array[j];
      array[j] = array[i];
      array[i] = t;
    }
  }
}

int rand_num(int min, int max) {
  return (rand() % (max-min+1))+min;
}

void init_field(struct field *field) {
  field->h = 26;
  field->w = 16;

  int x, y;
  for (y = 0; y < field->h; y++) {
    for (x = 0; x < field->w; x++) {
      field->bitmap[y][x] = ((x < 3 && y >= 3) 
        || (x >= field->w-3 && y >= 3) 
        || y >= field->h-3) ? CLR_FIELD : 0;
    }
  }
}

void fill_piece_bmp(int scratch[4][4], int type, int rot) {
  int x, y;
  for (y = 0; y < 4; y++) {
    for (x = 0; x < 4; x++) {
      scratch[y][x] = 0;
    }
  }
  if (type == PTYPE_I) {
    if (rot == 0) {
      scratch[2][0] = 1;
      scratch[2][1] = 1;
      scratch[2][2] = 1;
      scratch[2][3] = 1;
    } else {
      scratch[0][2] = 1;
      scratch[1][2] = 1;
      scratch[2][2] = 1;
      scratch[3][2] = 1;
    }
  } else if (type == PTYPE_T) {
    if (rot == 0) {
      scratch[2][0] = 1;
      scratch[2][1] = 1;
      scratch[2][2] = 1;
      scratch[3][1] = 1;
    } else if (rot == 1) {
      scratch[1][1] = 1;
      scratch[2][1] = 1;
      scratch[2][2] = 1;
      scratch[3][1] = 1;
    } else if (rot == 2) {
      scratch[2][0] = 1;
      scratch[2][1] = 1;
      scratch[2][2] = 1;
      scratch[1][1] = 1;
    } else {
      scratch[1][1] = 1;
      scratch[2][1] = 1;
      scratch[2][0] = 1;
      scratch[3][1] = 1;
    }
  } else if (type == PTYPE_Z) {
    if (rot == 0) {
      scratch[2][0] = 1;
      scratch[2][1] = 1;
      scratch[3][1] = 1;
      scratch[3][2] = 1;
    } else {
      scratch[1][2] = 1;
      scratch[2][1] = 1;
      scratch[3][1] = 1;
      scratch[2][2] = 1;
    }
  } else if (type == PTYPE_S) {
    if (rot == 0) {
      scratch[2][1] = 1;
      scratch[2][2] = 1;
      scratch[3][0] = 1;
      scratch[3][1] = 1;
    } else {
      scratch[2][1] = 1;
      scratch[2][2] = 1;
      scratch[1][1] = 1;
      scratch[3][2] = 1;
    }
  } else if (type == PTYPE_O) {
    scratch[2][0] = 1;
    scratch[2][1] = 1;
    scratch[3][0] = 1;
    scratch[3][1] = 1;
  } else if (type == PTYPE_J) {
    if (rot == 0) {
      scratch[2][0] = 1;
      scratch[2][1] = 1;
      scratch[2][2] = 1;
      scratch[3][2] = 1;
    } else if (rot == 1) {
      scratch[2][1] = 1;
      scratch[1][1] = 1;
      scratch[1][2] = 1;
      scratch[3][1] = 1;
    } else if (rot == 2) {
      scratch[2][0] = 1;
      scratch[2][1] = 1;
      scratch[2][2] = 1;
      scratch[1][0] = 1;
    } else {
      scratch[2][1] = 1;
      scratch[1][1] = 1;
      scratch[3][0] = 1;
      scratch[3][1] = 1;
    }
  } else if (type == PTYPE_L) {
    if (rot == 0) {
      scratch[2][0] = 1;
      scratch[2][1] = 1;
      scratch[2][2] = 1;
      scratch[3][0] = 1;
    } else if (rot == 1) {
      scratch[2][1] = 1;
      scratch[1][1] = 1;
      scratch[3][1] = 1;
      scratch[3][2] = 1;
    } else if (rot == 2) {
      scratch[2][0] = 1;
      scratch[2][1] = 1;
      scratch[2][2] = 1;
      scratch[1][2] = 1;
    } else {
      scratch[2][1] = 1;
      scratch[1][1] = 1;
      scratch[3][1] = 1;
      scratch[1][0] = 1;
    }
  }
}

void init_piece(struct state *state) {
  struct piece *piece = &state->piece;
  const struct field *field = &state->field;

  int i;
  int type = state->next_piece[0];
  if (state->next_piece[1] == 0) {
    // Fill next sequence
    for (i = 7; i >= 0; i--) {
      state->next_piece[i] = 7-i;
    }
    shuffle(state->next_piece, 7);
  } else {
    for (i = 1; i < 8; i++) {
      state->next_piece[i-1] = state->next_piece[i];
      if (state->next_piece[i] == 0) {
        break;
      }
    }
  }

  LOG("[%d]: ", type);
  for (i = 0; i < 8; i++) {
    LOG("%d", state->next_piece[i]);
    if (i < 7) {
      LOG(",");
    }
  }
  LOG("\n");

  int x, y;
  for (y = 0; y < 4; y++) {
    for (x = 0; x < 4; x++) {
      piece->bitmap[y][x] = 0;
    }
  }

  piece->type = type;
  piece->rot = 0;
  piece->x = field->w / 2-2;
  piece->y = 1; // TODO: depends on field

  fill_piece_bmp(piece->bitmap, piece->type, piece->rot);
  if (type == PTYPE_L) {
    piece->y = 2; // TODO: depends on field
  }

  if (state->game_state == STATE_DEMO) {
    state->suggestion = ai_suggest(piece, field);
  }
  state->last_automove = millis();
  state->next_automove_delta = AUTOPLAY_SPEED + rand_num(0,75);
}

void init_state(struct state *state, int game_state) {
  state->last_drop = millis();
  set_game_state(state, game_state);
  state->lines_cleared = 0;
  state->level = 1;
  state->drop_freq = DROP_FREQ_MAX; // drop every x millis
  state->suggestion = NULL;

  int i;
  for (i = 7; i >= 0; i--) {
    state->next_piece[i] = 7-i;
  }
  shuffle(state->next_piece, 7);

  init_field(&state->field);
  init_piece(state);
}

void draw_square(int x, int y, int c) {
  int r = (c >> 16) & 0xff;
  int g = (c >> 8) & 0xff;
  int b = c & 0xff;
  led_canvas_set_pixel(canvas, x * 2, y * 2, r, g, b);
  led_canvas_set_pixel(canvas, x * 2+1, y * 2, r, g, b);
  led_canvas_set_pixel(canvas, x * 2, y * 2+1, r, g, b);
  led_canvas_set_pixel(canvas, x * 2+1, y * 2+1, r, g, b);
}

void draw_pixel(int x, int y, int c) {
  led_canvas_set_pixel(canvas, x, y, (c>>16)&0xff, (c>>8)&0xff, c&0xff);
}

void print_text(int x, int y, const char *text, int color) {
  const char *ch = text;
  while (*ch != '\0') {
    int index = -1;
    int offset = -1;
    if (*ch >= 'A' && *ch <= 'Z') {
      index = ((*ch - 'A') / 8) * 5;
      offset = 31-((*ch - 'A') % 8) * 4;
    } else if (*ch >= 'a' && *ch <= 'z') {
      index = ((*ch - 'a') / 8) * 5;
      offset = 31-((*ch - 'a') % 8) * 4;
    } else if (*ch >= '0' && *ch <= '9') {
      index = ((*ch - '0') / 8 + 4) * 5;
      offset = 31-((*ch - '0') % 8) * 4;
    }
    if (index != -1 && offset != -1) {
      for (int top = 0; top < 5; top++) {
        for (int left = 0; left < 4; left++) {
          draw_pixel(x + left, y + top,
            (chars_packed[index+top]&(1<<(offset-left)))?color:0);
        }
      }
      x += 4;
    }
    ch++;
  }
}

const int field_x0 = -2;
const int field_y0 = -2;
int piece_color(const struct state *state, int piece_type) {
  int theme_count = (sizeof(piece_colors)/sizeof(int))/7; // 7 piece types
  return piece_colors[((state->level-1)%theme_count)*7+piece_type-1];
}

void recolor_field(struct state *state) {
	int x, y;
  int theme_count = (sizeof(piece_colors)/sizeof(int))/7; // 7 piece types

  struct field *field = &state->field;
  for (y = 3; y < field->h-3; y++) {
	  for (x = 3; x < field->w-3; x++) {
	  	if (field->bitmap[y][x] != CLR_BG) {
	  		for (int c = 0; c < 7; c++) {
	  			if (field->bitmap[y][x] == piece_colors[((state->level-1)%theme_count)*7+c]) {
	  				field->bitmap[y][x] = piece_colors[((state->level)%theme_count)*7+c];
	  			}
	  		}
	  	}
		}
  }
}

void draw_piece(const struct state *state, struct piece *piece) {
  int x, y;
  for (y = 0; y < 4; y++) {
    for (x = 0; x < 4; x++) {
      if (piece->bitmap[y][x] && y+piece->y+field_y0 >= 1) {
        draw_square(piece->x+x+field_x0, piece->y+y+field_y0,
          piece_color(state, piece->type));
      }
    }
  }
}

void increment_level(struct state *state) {
	recolor_field(state);
  state->level++;
}

void draw_field(struct state *state) {
  int x, y;
  for (y = 3; y < state->field.h-3; y++) {
    draw_pixel(field_x0+3, (field_y0+y) * 2, CLR_FIELD);
    draw_pixel(field_x0+3, (field_y0+y) * 2+1, CLR_FIELD);
    draw_pixel(field_x0+(state->field.w-6) * 2+4, (field_y0+y) * 2, CLR_FIELD);
    draw_pixel(field_x0+(state->field.w-6) * 2+4, (field_y0+y) * 2+1, CLR_FIELD);
    for (x = 3; x < state->field.w-3; x++) {
      draw_square(x+field_x0, y+field_y0, state->field.bitmap[y][x]);
    }
  }
  for (x = 3; x < state->field.w-3; x++) {
    draw_pixel((field_x0+x) * 2, field_y0+(state->field.h-6) * 2+4, CLR_FIELD);
    draw_pixel((field_x0+x) * 2+1, field_y0+(state->field.h-6) * 2+4, CLR_FIELD);
  }
}

void draw_statics(struct state *state) {
  int x, y;
  if (state->game_state == STATE_OVER) {
    const int go_x0 = 3;
    const int go_y0 = 13;
    for (y = 0; y < 11; y++) {
      for (x = 0; x < 16; x++) {
        draw_pixel(go_x0+x, go_y0+y,
          game_over_bmp[y] & (1<<(15-x)) ? CLR_TEXT : CLR_BG);
      }
    }
  // } else if (state->game_state == STATE_DEMO) {
  //   const int go_x0 = 3;
  //   const int go_y0 = 2;
  //   for (y = 0; y < 5; y++) {
  //     for (x = 0; x < 16; x++) {
  //       draw_pixel(state->canvas, go_x0+x, go_y0+y,
  //         demo_bmp[y] & (1<<(15-x)) ? CLR_TEXT : CLR_BG);
  //     }
  //   }
  }

  // next piece
  const int x0 = 12;
  const int y0 = 0;
  static int bitmap[4][4];
  int next_type = state->next_piece[0];
  fill_piece_bmp(bitmap, next_type, 0);

  for (y = 0; y < 4; y++) {
    for (x = 0; x < 4; x++) {
      draw_square(x0+x, y0+y,
        state->game_state != STATE_OVER && bitmap[y][x] != 0
          ? piece_color(state, next_type) : CLR(0x00,0x00,0x00));
    }
  }

  // static char buffer[128];
  // snprintf(buffer, sizeof(buffer), "LVL %d", state->level);
  // print_text(1, 44, buffer, CLR_TEXT);
}

void overlay_piece(struct state *state, const struct piece *piece) {
  int x, y;
  struct field *field = &state->field;
  for (y = 0; y < 4; y++) {
    for (x = 0; x < 4; x++) {
      if (piece->bitmap[y][x]) {
        field->bitmap[piece->y+y][piece->x+x] = piece_color(state, piece->type);
      }
    }
  }
}

void animate_game_start(struct state *state) {
  struct field *field = &state->field;
  for (int btm = field->h-3-1; btm >= 3; btm--) {
    for (int y = btm; y >= 3; y--) {
      for (int x = 3; x < field->w-3; x++) {
        if (y == btm) {
          field->bitmap[y][x] = CLR_BG;
        } else {
          field->bitmap[y][x] = ((btm + y) % 2)
            ? CLR_COVER_1 : CLR_COVER_2;
        }
      }
    }

    draw_field(state);
    canvas = led_matrix_swap_on_vsync(matrix, canvas);
    usleep(SYNC_ANIM_DELAY);
  }
}

void animate_game_over(struct state *state) {
  struct field *field = &state->field;
  for (int btm = 3; btm < field->h-3; btm++) {
    for (int y = btm; y >= 3; y--) {
      for (int x = 3; x < field->w-3; x++) {
        field->bitmap[y][x] = ((btm + y) % 2)
          ? CLR_COVER_2 : CLR_COVER_1;
      }
    }

    draw_field(state);
    canvas = led_matrix_swap_on_vsync(matrix, canvas);
    usleep(SYNC_ANIM_DELAY);
  }
}

void animate_collapse(struct state *state) {
  struct field *field = &state->field;
  int x, y;
  for (x = field->w/2-1; x >= 3; x--) {
    for (y = field->h-3-1; y >= 3; y--) {
      int full_row = 1;
      for (int sx = 3; sx < field->w-3; sx++) {
        if (field->bitmap[y][sx] == 0) {
          full_row = 0;
          break;
        }
      }

      if (full_row) {
        field->bitmap[y][x] = CLR(0x01,0x01,0x01);
        field->bitmap[y][field->w-x-1] = CLR(0x01,0x01,0x01);
      }
    }
    draw_field(state);
    canvas = led_matrix_swap_on_vsync(matrix, canvas);
    usleep(SYNC_ANIM_DELAY);
  }
}

void collapse_rows(struct state *state) {
  struct field *field = &state->field;
  int x, y;
  int delta = 0;
  bool animated = 0;
  for (y = field->h-3-1; y >= 3; y--) {
    int increment = 1;
    for (x = 3; x < field->w-3; x++) {
      if (field->bitmap[y][x] == 0) {
        increment = 0;
        break;
      }
    }

    if (increment) {
      if (!animated) {
        animate_collapse(state);
        animated = 1;
      }
      delta++;
      continue;
    }

    if (delta > 0) {
      for (x = 3; x < field->w-3; x++) {
        field->bitmap[y+delta][x] = field->bitmap[y][x];
        field->bitmap[y][x] = 0;
      }
    }
  }

  int lines_cleared = state->lines_cleared;
  state->lines_cleared += delta;
  if (lines_cleared / 10 != state->lines_cleared / 10) {
    if (state->drop_freq > DROP_FREQ_MIN) {
      state->drop_freq -= 50;
    }
    increment_level(state);
  }
  if (delta > 0) {
    LOG("Lines: %d (+%d); Level: %d; Freq: %d; Elapsed: %s\n",
      state->lines_cleared, delta, state->level, state->drop_freq,
      millis_to_text(millis()-state->last_game_state_change));
  }
}

int can_put_bmp(const int bitmap[4][4], const struct field *field, int fx, int fy) {
  int x, y;
  for (y = 0; y < 4; y++) {
    for (x = 0; x < 4; x++) {
      if (bitmap[y][x] && field->bitmap[fy+y][fx+x]) {
        return 0;
      }
    }
  }
  return 1;
}

void ai_score_bmp(struct piece_state *ps, const int bitmap[4][4], const struct field *field, int fx, int fy) {
  int x, y;
  int ymin = 4;
  int ymax = 0;
  ps->score = 0;
  for (y = 0; y < 4; y++) {
    for (x = 0; x < 4; x++) {
      if (bitmap[y][x]) {
        if (y < ymin) ymin = y;
        if (y > ymax) ymax = y;
        if (field->bitmap[fy+y][fx+x-1]) ps->score++;
        if (field->bitmap[fy+y+1][fx+x-1]) ps->score++;
        if (field->bitmap[fy+y+1][fx+x]) ps->score++;
        if (field->bitmap[fy+y+1][fx+x+1]) ps->score++;
        if (field->bitmap[fy+y][fx+x+1]) ps->score++;
      }
    }
  }
  ps->ht = ymax-ymin+1;
}

int can_put(const struct piece *piece, const struct field *field, int fx, int fy) {
  return can_put_bmp(piece->bitmap, field, fx, fy);
}

int incr_wrap(int n, int d, int size) {
  if (d < 0) {
    return (n+d < 0) ? size-1 : n+d;
  } else if (d > 0) {
    return (n+d) % size;
  } else {
    return n;
  }
}

void ai_dump_suggestion(int ptype, const struct piece_state *best, const struct field *field) {
  static int scratch[4][4];
  fill_piece_bmp(scratch, ptype, best->rot);

  for (int y = 3; y < field->h-3; y++) {
    LOG("\n%2d: ", y-2);
    for (int x = 3; x < field->w-3; x++) {
      LOG("%c ",
        y >= best->y && y < best->y+4 && x >= best->x && x < best->x+4 && scratch[y-best->y][x-best->x] != 0
          ? '@' : field->bitmap[y][x] != 0 ? '*' : '.');
    }
  }
  LOG("\n");
}

const struct piece_state* ai_suggest(const struct piece *piece, const struct field *field) {
  static int scratch[4][4];
  static struct piece_state ps;
  static struct piece_state best;

  best.score = -1;
  best.rot = -1;
  best.x = -1;
  best.y = -1;

  for (int rot = 0; rot < piece_rots[piece->type]; rot++) {
    memset(scratch, 0, sizeof(scratch));

    fill_piece_bmp(scratch, piece->type, rot);
    int x = piece->x;
    for (; can_put_bmp(scratch, field, x, piece->y); x--);

    for (x++; can_put_bmp(scratch, field, x, piece->y); x++) {
      int y = piece->y;
      for (; can_put_bmp(scratch, field, x, y); y++);
      y--;

      ai_score_bmp(&ps, scratch, field, x, y);
      if (ps.score > best.score // prefer higher score
        || (ps.score == best.score && y > best.y) // prefer lower pieces
        ) {
        best.score = ps.score;
        best.ht = ps.ht;
        best.x = x;
        best.y = y;
        best.rot = rot;
      }
    }
  }

  ai_dump_suggestion(piece->type, &best, field);

  return best.score == -1 ? NULL : &best;
}

void rotate_piece(struct piece *piece, const struct field *field, int d) {
  static int scratch[4][4];
  memset(scratch, 0, sizeof(scratch));

  int rot = incr_wrap(piece->rot, d, piece_rots[piece->type]);

  fill_piece_bmp(scratch, piece->type, rot);
  if (can_put_bmp(scratch, field, piece->x, piece->y)) {
    memcpy(piece->bitmap, scratch, sizeof(scratch));
    piece->rot = rot;
  }
}

void drop(struct state *state) {
  struct piece *piece = &state->piece;
  struct field *field = &state->field;
  if (can_put(piece, field, piece->x, piece->y+1)) {
    piece->y++;
  } else {
    overlay_piece(state, piece);
    collapse_rows(state);
    init_piece(state);
    if (!can_put(piece, field, piece->x, piece->y)) {
      overlay_piece(state, piece);
      set_game_state(state, STATE_OVER);
      return;
    }
  }
  state->last_drop = millis();
}

void handle_joystick(SDL_Joystick *joy, struct state *state) {
  const int deadzone = 250;
  static long long input_states[7] = {0,0,0,0,0,0,0};
  const long long tick = millis();
  struct piece *piece = &state->piece;
  const struct field *field = &state->field;

  if (SDL_JoystickGetButton(joy, 2)) {
    if (tick-input_states[6] > MILLIS_TIL_BTN_RPT) {
      input_states[6] = tick;
      if (state->game_state == STATE_OVER
        || state->game_state == STATE_DEMO) {
        init_state(state, STATE_PLAY);
        return;
      }
      set_game_state(state, (state->game_state == STATE_PAUSE) ? STATE_PLAY : STATE_PAUSE);
    }
  } else {
    input_states[6] = 0;
  }

  if ((state->game_state & STATE_MASK_NO_INPUT) != 0) {
    return;
  }

  int x = SDL_JoystickGetAxis(joy, 0);
  if (x < -deadzone) {
  	if (can_put(piece, field, piece->x-1, piece->y)
	    && tick-input_states[0] > MILLIS_TIL_BTN_RPT) {
	    input_states[0] = tick;
	    piece->x--;
	  }
    input_states[1] = 0;
  } else if (x > deadzone) {
  	if (can_put(piece, field, piece->x+1, piece->y)
	    && tick-input_states[1] > MILLIS_TIL_BTN_RPT) {
	    input_states[1] = tick;
	    piece->x++;
	  }
    input_states[0] = 0;
  } else {
    input_states[0] = 0;
    input_states[1] = 0;
  }

  int y = SDL_JoystickGetAxis(joy, 1);
  if (y > deadzone) {
  	if (can_put(piece, field, piece->x, piece->y+1)
	    && tick-input_states[3] > MILLIS_TIL_BTN_RPT) {
	    drop(state);
	    input_states[3] = tick;
	  }
  } else {
	  input_states[3] = 0;
  }

  if (SDL_JoystickGetButton(joy, 0)) {
    if (tick-input_states[4] > MILLIS_TIL_BTN_RPT) {
      rotate_piece(piece, field, 1);
      input_states[4] = tick;
    }
  } else {
    input_states[4] = 0;
  }

  if (SDL_JoystickGetButton(joy, 1)) {
    if (tick-input_states[5] > MILLIS_TIL_BTN_RPT) {
      rotate_piece(piece, field, -1);
      input_states[5] = tick;
    }
  } else {
    input_states[5] = 0;
  }
}

void handle_autoplay(struct state *state) {
  const struct piece_state *sugg = state->suggestion;
  if (sugg == NULL) {
    return;
  }

  long long tick = millis();
  struct piece *piece = &state->piece;
  if (piece->rot == sugg->rot
    && piece->x == sugg->x 
    && tick-state->last_automove > AUTOPLAY_SPEED) {
    drop(state);
    state->last_automove = tick;
  } else if (tick-state->last_automove > state->next_automove_delta) {
    if (piece->rot != sugg->rot) {
    	int dir = 1;
    	int rotations = piece_rots[piece->type];
    	if (rotations == 2) {
    		// only 2 possible states - pick a direction at random
    		dir = rand_num(0,1) ? 1 : -1;
    	} else if (rotations == 4) {
    		// pick the closest
    		if (incr_wrap(piece->rot, 1, rotations) == sugg->rot) {
    			dir = 1; // next one up
    		} else if (incr_wrap(piece->rot, -1, rotations) == sugg->rot) {
    			dir = -1; // next one down
    		} else {
	    		dir = rand_num(0,1) ? 1 : -1; // equidistant, so pick one at random
    		}
    	}

      rotate_piece(piece, &state->field, dir);
      state->last_automove = tick;
    }
    if (piece->x < sugg->x) {
      piece->x++;
      state->last_automove = tick;
    } else if (piece->x > sugg->x) {
      piece->x--;
      state->last_automove = tick;
    }
  }
}

int main(int argc, char **argv) {
  LOG("Starting up - ai v. %d\n", AI_VERSION);

  struct RGBLedMatrixOptions options;
  int width, height;

  memset(&options, 0, sizeof(options));
  options.rows = 32;
  options.chain_length = 1;

  /* This supports all the led commandline options. Try --led-help */
  matrix = led_matrix_create_from_options(&options, &argc, &argv);
  if (matrix == NULL)
    return 1;

  /* Let's do an example with double-buffering. We create one extra
   * buffer onto which we draw, which is then swapped on each refresh.
   * This is typically a good aproach for animations and such.
   */
  canvas = led_matrix_create_offscreen_canvas(matrix);

  led_canvas_get_size(canvas, &width, &height);

  // Set up an interrupt handler to be able to stop animations while they go
  // on. Note, each demo tests for while (running() && !interrupt_received) {},
  // so they exit as soon as they get a signal.
  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  if (SDL_Init(SDL_INIT_JOYSTICK) < 0) {
    LOG("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
  }
  SDL_Joystick *joy = SDL_JoystickOpen(0);
  if (joy == NULL) {
    LOG("Warning: No joysticks detected\n");
  }

  LOG("Size: %dx%d. Hardware gpio mapping: %s\n",
          width, height, options.hardware_mapping);

  srand((unsigned) time(NULL));

  struct state state;
  init_state(&state, STATE_OVER);

  while (!interrupt_received) {
    long long tick = millis();
    if (state.game_state != STATE_PAUSE
      && state.game_state != STATE_OVER
      && tick-state.last_drop > state.drop_freq) {
      drop(&state);
    }

    if (state.game_state == STATE_DEMO) {
      handle_autoplay(&state);
    }

    if (joy != NULL) {
      SDL_JoystickUpdate();
      handle_joystick(joy, &state);
    }

    draw_field(&state);
    if (state.game_state != STATE_OVER) {
      draw_piece(&state, &state.piece);
    } else {
      if (MILLIS_UNTIL_DEMO != 0 && tick-state.last_game_state_change > MILLIS_UNTIL_DEMO) {
        init_state(&state, STATE_DEMO);
      }
    }
    draw_statics(&state);

    /* Now, we swap the canvas. We give swap_on_vsync the buffer we
     * just have drawn into, and wait until the next vsync happens.
     * we get back the unused buffer to which we'll draw in the next
     * iteration.
     */
    canvas = led_matrix_swap_on_vsync(matrix, canvas);
  }

  /*
   * Make sure to always call led_matrix_delete() in the end to reset the
   * display. Installing signal handlers for defined exit is a good idea.
   */
  led_matrix_delete(matrix);

  return 0;
}
