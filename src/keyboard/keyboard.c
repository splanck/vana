#include "keyboard.h"
#include "status.h"
#include "classic.h"
#include "io/io.h"
#include "idt/idt.h"
#include <stdint.h>

static struct keyboard* keyboard_list_head = 0;
static struct keyboard* keyboard_list_last = 0;

struct keyboard_buffer
{
    char buffer[VANA_KEYBOARD_BUFFER_SIZE];
    int head;
    int tail;
};

static struct keyboard_buffer kbuffer;

static void keyboard_irq_handler(struct interrupt_frame* frame);

void keyboard_init()
{
    keyboard_insert(classic_init());
    idt_register_interrupt_callback(ISR_KEYBOARD_INTERRUPT, keyboard_irq_handler);
}

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

static int keyboard_get_tail_index()
{
    return kbuffer.tail % VANA_KEYBOARD_BUFFER_SIZE;
}

static void keyboard_irq_handler(struct interrupt_frame* frame)
{
    (void)frame;
    uint8_t scancode = insb(KEYBOARD_INPUT_PORT);
    insb(KEYBOARD_INPUT_PORT);
    keyboard_push((char)scancode);
}

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

void keyboard_push(char c)
{
    if (c == 0)
        return;
    int real_index = keyboard_get_tail_index();
    kbuffer.buffer[real_index] = c;
    kbuffer.tail++;
}

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
