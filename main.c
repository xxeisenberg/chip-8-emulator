#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "raylib.h"

#define SCALE 20

typedef struct {
  uint8_t memory[4096];
  uint8_t v[16];
  uint16_t i;
  uint16_t pc;
  uint16_t stack[16];
  uint8_t sp;
  uint8_t delay_timer;
  uint8_t sound_timer;
  uint8_t keypad[16];
  uint8_t display_buffer[64 * 32];
} Chip8;

void chip8_init(Chip8 *chip8) {
  // Clearing memory
  memset(chip8, 0, sizeof(Chip8));
  chip8->pc = 0x200;

  const uint8_t chip8_fontset[80] = {
      0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
      0x20, 0x60, 0x20, 0x20, 0x70, // 1
      0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
      0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
      0x90, 0x90, 0xF0, 0x10, 0x10, // 4
      0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
      0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
      0xF0, 0x10, 0x20, 0x40, 0x40, // 7
      0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
      0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
      0xF0, 0x90, 0xF0, 0x90, 0x90, // A
      0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
      0xF0, 0x80, 0x80, 0x80, 0xF0, // C
      0xE0, 0x90, 0x90, 0x90, 0xE0, // D
      0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
      0xF0, 0x80, 0xF0, 0x80, 0x80  // F
  };

  uint8_t *memory_with_font_offset = chip8->memory + 0x50;
  for (int i = 0; sizeof(chip8_fontset) > i; i++) {
    memory_with_font_offset[i] = chip8_fontset[i];
  }
}

int chip8_load_rom(Chip8 *chip8, const char *filename) {
  FILE *fp = fopen(filename, "rb");

  if (fp == NULL) {
    perror("Error opening file");
    return -1;
  }

  if (fseek(fp, 0, SEEK_END) != 0) {
    fclose(fp);
    return -1;
  }

  long size = ftell(fp);
  rewind(fp);
  if (size < 0 || size > 4096 - 0x200) {
    perror("Size of file is out of the memory limit");
    return -1;
  }

  fread(chip8->memory + 0x200, 1, size, fp);
  fclose(fp);
  return 0;
}

uint16_t chip8_fetch(Chip8 *chip8) {
  uint8_t high_byte = chip8->memory[chip8->pc];
  uint8_t low_byte = chip8->memory[chip8->pc + 1];

  chip8->pc += 2;

  uint16_t result = (high_byte << 8) | low_byte;

  return result;
}

void chip8_step(Chip8 *chip8) {
  uint16_t opcode = chip8_fetch(chip8);

  uint8_t type = (opcode & 0xF000) >> 12;
  uint8_t x = (opcode & 0x0F00) >> 8;
  uint8_t y = (opcode & 0x00F0) >> 4;
  uint8_t n = opcode & 0x000F;
  uint8_t nn = opcode & 0x00FF;
  uint16_t nnn = opcode & 0x0FFF;

  switch (type) {
  case 0x0:
    if (nn == 0xE0) {
      memset(chip8->display_buffer, 0, sizeof(chip8->display_buffer));
    } else if (nn == 0xEE) {
      chip8->sp--;
      chip8->pc = chip8->stack[chip8->sp];
    }
    break;

  case 0x1:
    chip8->pc = nnn;
    break;

  case 0x2:
    chip8->stack[chip8->sp] = chip8->pc;
    chip8->sp++;
    chip8->pc = nnn;
    break;

  case 0x3:
    if (chip8->v[x] == nn) {
      chip8->pc += 2;
    }
    break;

  case 0x4:
    if (chip8->v[x] != nn) {
      chip8->pc += 2;
    }
    break;

  case 0x5:
    if (n == 0) {
      if (chip8->v[x] == chip8->v[y]) {
        chip8->pc += 2;
      }
    }
    break;

  case 0x6:
    chip8->v[x] = nn;
    break;

  case 0x7:
    chip8->v[x] += nn;
    break;

  case 0x8:
    switch (n) {
    case 0:
      chip8->v[x] = chip8->v[y];
      break;
    case 1:
      chip8->v[x] = chip8->v[x] | chip8->v[y];
      break;
    case 2:
      chip8->v[x] = chip8->v[x] & chip8->v[y];
      break;
    case 3:
      chip8->v[x] = chip8->v[x] ^ chip8->v[y];
      break;
    case 4:
      uint16_t sum = (uint16_t)chip8->v[x] + chip8->v[y];
      chip8->v[x] = (uint8_t)(sum & 0xFF);
      chip8->v[0xF] = (sum > 0xFF) ? 1 : 0;
      break;
    case 5:
      uint8_t not_borrow = (chip8->v[x] >= chip8->v[y]) ? 1 : 0;
      chip8->v[x] -= chip8->v[y];
      chip8->v[0xF] = not_borrow;
      break;
    case 6:
      chip8->v[0xF] = chip8->v[x] & 0x1;
      chip8->v[x] >>= 1;
      break;
    case 0xE:
      chip8->v[0xF] = (chip8->v[x] & 0x80) >> 7;
      chip8->v[x] <<= 1;
      break;
    }
    break;

  case 0x9:
    if (n == 0) {
      if (chip8->v[x] != chip8->v[y]) {
        chip8->pc += 2;
      }
    }
    break;

  case 0xA:
    chip8->i = nnn;
    break;

  case 0xB:
    chip8->pc = nnn + chip8->v[0];
    break;

  case 0xC:
    chip8->v[x] = (rand() % 256) & nn;
    break;

  case 0xD:
    uint8_t x_coord = chip8->v[x] % 64;
    uint8_t y_coord = chip8->v[y] % 32;

    chip8->v[0xF] = 0;

    for (int row = 0; row < n; row++) {
      uint8_t sprite_byte = chip8->memory[chip8->i + row];
      for (int col = 0; col < 8; col++) {
        uint8_t result = sprite_byte & (0x80 >> col);
        if (result != 0) {
          int px = x_coord + col;
          int py = y_coord + row;
          if (py >= 32) {break;}
          if (px >= 64) {continue;}
          if (chip8->display_buffer[py * 64 + px] == 1){
            chip8->v[0xF] = 1;
          }
          chip8->display_buffer[py * 64 + px] ^= 1;
        }
      } 
    }
    break;

    case 0xE:
      if (nn == 0x9E){
        if (chip8->keypad[chip8->v[x]]) { chip8->pc += 2; }
      } else if(nn == 0xA1){
        if (!chip8->keypad[chip8->v[x]]) { chip8->pc += 2; }
      }
      break;

    case 0xF:
      switch (nn) {
        case 0x07:
          chip8->v[x] = chip8->delay_timer;
          break;
        case 0x0A: 
          int key_pressed = 0;
          for (int k = 0; k < 16; k++) {
            if (chip8->keypad[k]){
              chip8->v[x] = k;
              key_pressed =1;
              break;
            }
          }
          if (!key_pressed) {
            chip8->pc -= 2;
          }
          break;
        case 0x15:
          chip8->delay_timer = chip8->v[x];
          break;
        case 0x18:
          chip8->sound_timer = chip8->v[x];
          break;
        case 0x1E:
          chip8->i += chip8->v[x];
          break;
        case 0x29:
          chip8->i = 0x50 + (chip8->v[x] * 5);
          break;
        case 0x33:
          chip8->memory[chip8->i] = chip8->v[x] / 100;
          chip8->memory[chip8->i + 1] = (chip8->v[x] / 10) % 10;
          chip8->memory[chip8->i + 2] = chip8->v[x] % 10;
          break;
        case 0x55:
          for(int idx = 0; x >= idx; idx++){
            chip8->memory[chip8->i + idx] = chip8->v[idx];
          }
          break;
        case 0x65:
          for(int idx = 0; x >= idx; idx++){
            chip8->v[idx] = chip8->memory[chip8->i + idx];
          }
          break;
      }
      break;
  }
}

void chip8_tick_timer(Chip8 *chip8){
  if (chip8->delay_timer > 0) chip8->delay_timer --;
  if (chip8->sound_timer > 0) chip8->sound_timer --;
}

const int keymap[16] = {
    KEY_X,     // 0x0
    KEY_ONE,   KEY_TWO,   KEY_THREE, // 0x1, 0x2, 0x3
    KEY_Q,     KEY_W,     KEY_E,     // 0x4, 0x5, 0x6
    KEY_A,     KEY_S,     KEY_D,     // 0x7, 0x8, 0x9
    KEY_Z,     KEY_C,                // 0xA, 0xB
    KEY_FOUR,  KEY_R,     KEY_F,     KEY_V // 0xC, 0xD, 0xE, 0xF
};

int main(int argc, char *argv[]){
  if (argc <= 1) {
    fprintf(stderr, "Usage: %s <ROM>\n", argv[0]);
    return 1;
  }
  Chip8 chip8;
  chip8_init(&chip8);
  chip8_load_rom(&chip8, argv[1]);

  const int screenWidth = 64 * SCALE;
  const int screenHeight = 32 * SCALE;

  InitWindow(screenWidth,screenHeight, "CHIP-8 Emulator");

  SetTargetFPS(60);

  while(!WindowShouldClose()){

    for (int k = 0; k< 16; k++) {
      chip8.keypad[k] = IsKeyDown(keymap[k]) ? 1 : 0;
    }
    
    for (int i = 0; i < 10; i++) {
        chip8_step(&chip8);
    }

    chip8_tick_timer(&chip8);

    BeginDrawing();
    ClearBackground(BLACK);

    for (int y = 0; y < 32; y++) {
      for (int x =0; x<64; x++) {
        if (chip8.display_buffer[y*64 + x]) {
          DrawRectangle(x * SCALE, y* SCALE, SCALE, SCALE, WHITE);
        }
      }
    }
    EndDrawing();
  }

  CloseWindow();
  return 0;
}