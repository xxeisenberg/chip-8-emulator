# Chip 8 Emulator

A simple CHIP-8 emulator written in C to learn low-level programming. Uses Raylib to handle rendering the display and capturing keyboard input.

## How it works

The core emulator implements the standard CHIP-8 specs:
- 4KB of RAM with programs loaded at `0x200`
- 16 general-purpose 8-bit registers (`V0` - `VF`)
- 16-bit program counter (`PC`) and index register (`I`)
- 64x32 monochrome display buffer
- 60 Hz delay and sound timers
- CPU loop running at ~600 Hz (10 cycles per 60 FPS frame)

## Controls

The original 16-key hex keypad is mapped to the left side of a QWERTY keyboard:

```text
CHIP-8 Keypad        Keyboard
1  2  3  C    ->     1  2  3  4
4  5  6  D    ->     Q  W  E  R
7  8  9  E    ->     A  S  D  F
A  0  B  F    ->     Z  X  C  V
```

## How to use

Download the [latest release](https://github.com/xxeisenberg/chip-8-emulator/releases).

Then play any of the roms by:

```bash
./chip8 roms/<NAME>
```

## Building locally

Make sure gcc & raylib are installed

```bash
gcc main.c -lraylib -lm -o chip8
```

---
> Credits: Roms are from [James Griffin](https://github.com/JamesGriffin/CHIP-8-Emulator)