/*
 * Generic keyboard subsystem.
 *
 * A small ring buffer stores characters produced by the active keyboard
 * driver. The buffer decouples interrupt-time processing from code that
 * consumes input, such as the shell. Drivers push characters with
 * `keyboard_push()` while consumers call `keyboard_pop()` to retrieve
 * them. `keyboard_init()` registers available drivers (currently only the
 * classic PS/2 implementation) so they can hook their IRQ handlers.
 */
#include "keyboard.h"
#include "status.h"
#include "classic.h"
#include "io/io.h"
#include "idt/idt.h"
#include <stdint.h>

static struct keyboard* keyboard_list_head = 0;
static struct keyboard* keyboard_list_last = 0;

/*
 * Simple ring buffer used to queue keyboard input.
 * `head` points to the next character to read while `tail` marks the
 * next free slot for writing. Both indices wrap around
 * `VANA_KEYBOARD_BUFFER_SIZE` so the buffer acts as a circular queue.
 */
struct keyboard_buffer
{
    char buffer[VANA_KEYBOARD_BUFFER_SIZE];
    int head;
    int tail;
};

static struct keyboard_buffer kbuffer;

/* Initialise the keyboard subsystem and register built-in drivers. */
void keyboard_init()
{
    keyboard_insert(classic_init());
}

/*
 * Add a keyboard driver to the linked list and invoke its initialisation
 * routine. The driver must provide an `init` callback which typically
 * registers an IRQ handler.
 */
int keyboard_insert(struct keyboard* keyboard)
{
    int res = 0;
    if (!keyboard || !keyboard->init)
    {
        return -EINVARG;
    }

    if (keyboard_list_last)
    {
        keyboard_list_last->next = keyboard;
        keyboard_list_last = keyboard;
    }
    else
    {
        keyboard_list_head = keyboard;
        keyboard_list_last = keyboard;
    }

    res = keyboard->init();
    return res;
}

/* Helper used by push/backspace to wrap the tail index. */
static int keyboard_get_tail_index()
{
    return kbuffer.tail % VANA_KEYBOARD_BUFFER_SIZE;
}

/* Remove the most recently pushed character if the buffer isn't empty. */
void keyboard_backspace()
{
    if (kbuffer.tail == 0)
        return;
    kbuffer.tail -= 1;
    int real_index = keyboard_get_tail_index();
    kbuffer.buffer[real_index] = 0x00;
}

void keyboard_set_capslock(struct keyboard* keyboard, KEYBOARD_CAPS_LOCK_STATE state)
{
    keyboard->capslock_state = state;
}

KEYBOARD_CAPS_LOCK_STATE keyboard_get_capslock(struct keyboard* keyboard)
{
    return keyboard->capslock_state;
}

void keyboard_set_shift(struct keyboard* keyboard, int state)
{
    keyboard->shift_state = state;
}

int keyboard_get_shift(const struct keyboard* keyboard)
{
    return keyboard->shift_state;
}

/* Append a character to the input buffer. Zero values are ignored. */
void keyboard_push(char c)
{
    if (c == 0)
        return;
    int real_index = keyboard_get_tail_index();
    kbuffer.buffer[real_index] = c;
    kbuffer.tail++;
}

/* Retrieve the next character from the buffer, or 0 when empty. */
char keyboard_pop()
{
    int real_index = kbuffer.head % VANA_KEYBOARD_BUFFER_SIZE;
    char c = kbuffer.buffer[real_index];
    if (c == 0x00)
    {
        return 0;
    }
    kbuffer.buffer[real_index] = 0x00;
    kbuffer.head++;
    return c;
}
