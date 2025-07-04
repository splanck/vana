# Keyboard Driver Overview

The keyboard subsystem is responsible for turning raw PS/2 scancodes into
ASCII characters that the rest of the kernel can consume.  Its default
implementation is the "classic" driver which listens on **IRQ&nbsp;1** so an
interrupt is raised every time a key is pressed or released.

At the heart of the subsystem is a small ring buffer used to decouple the
interrupt handler from code that reads input.  Characters translated from the
scancode stream are appended to this buffer while consumers call
`keyboard_pop()` to retrieve them later.  The buffer maintains two indices:
`head` points to the next character to read and `tail` marks the next free slot
for writing.  Both wrap around `VANA_KEYBOARD_BUFFER_SIZE` so the structure acts
as a continuous queue.

Scancodes follow **set&nbsp;1** and are translated using the
`keyboard_scan_set_one` table.  The handler reads a byte from port `0x60`,
checks the release bit and looks up the corresponding ASCII value while taking
shift and caps lock state into account.  Valid characters are then pushed into
the ring buffer.

## Driver Interface (`keyboard.c/h`)

A `struct keyboard` holds function pointers and state flags for caps lock and shift.
`keyboard_init()` registers available implementations and calls their `init`
hooks.  Each driver registers its interrupt handler with the IDT using
`idt_register_interrupt_callback`. The PS/2 driver binds to IRQ&nbsp;1 via the
constant `ISR_KEYBOARD_INTERRUPT` so key presses trigger `classic_keyboard_handle_interrupt`.

Input characters are stored in a circular buffer defined by `struct keyboard_buffer`.
The `head` index points to the next character to read while `tail` marks where
the next character will be written.  Both indices are modulo
`VANA_KEYBOARD_BUFFER_SIZE` so the buffer acts like a ring:

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
