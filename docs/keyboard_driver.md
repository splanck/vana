# Keyboard Driver Overview

This document outlines how the kernel's keyboard subsystem is organised. Two files, `keyboard.c/h`, expose a generic driver interface and ring buffer.  The "classic" PS/2 implementation lives in `classic.c/h`.

## Driver Interface (`keyboard.c/h`)

A `struct keyboard` holds function pointers and state flags for caps lock and shift.  `keyboard_init()` registers available implementations and calls their `init` hooks. A global linked list allows supporting multiple keyboard types.

Input characters are stored in a circular buffer defined by `struct keyboard_buffer`:

```c
struct keyboard_buffer {
    char buffer[VANA_KEYBOARD_BUFFER_SIZE];
    int head;
    int tail;
};
```

`keyboard_push()` appends a non‑zero character and advances `tail`, wrapping with a modulo on `VANA_KEYBOARD_BUFFER_SIZE`. `keyboard_pop()` reads from `head` and clears the consumed slot.  `keyboard_backspace()` simply rewinds `tail` when possible.

## PS/2 Scancode Handling (`classic.c/h`)

`classic.c` implements a simple PS/2 driver. `classic_keyboard_handle_interrupt()` is registered for IRQ 1 and reads the scancode from port `0x60`:

```c
uint8_t scancode = insb(KEYBOARD_INPUT_PORT);
insb(KEYBOARD_INPUT_PORT);
int released = scancode & CLASSIC_KEYBOARD_KEY_RELEASED;
uint8_t keycode = scancode & ~CLASSIC_KEYBOARD_KEY_RELEASED;
```

Shift keys (0x2A and 0x36) toggle the shift state when pressed or released. A scancode with the release bit set is otherwise ignored. Caps lock (0x3A) toggles the driver state.

`classic_keyboard_scancode_to_char()` maps set‑1 scancodes using the `keyboard_scan_set_one` table.  When shift is active, digits become punctuation (e.g. '1' → '!'), and the combined shift/caps lock state decides whether letters are uppercase.  Valid characters are pushed into the keyboard buffer.

## Summary

The generic layer manages a ring buffer so higher layers can retrieve typed characters with `keyboard_pop()`.  The classic PS/2 driver translates raw scancodes, updates shift and caps lock flags, and feeds ASCII characters into that buffer.
