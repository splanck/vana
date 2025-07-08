/*
 * Classic PS/2 keyboard driver.
 *
 * A PS/2 keyboard raises **IRQ&nbsp;1** every time a key is pressed or
 * released.  The controller places a scancode byte on I/O port `0x60`
 * which this driver reads inside the interrupt handler.  Port `0x64`
 * is used for command/status operations such as enabling the first PS/2
 * port.
 *
 * Scancodes follow **set&nbsp;1** and are translated to ASCII characters.
 * The resulting character stream is appended to the kernel keyboard buffer
 * via `keyboard_push()`.  Higher level code (for example the shell) reads
 * from this buffer using `keyboard_pop()` when it needs user input.
 */

#include "classic.h"
#include "keyboard.h"
#include "io/io.h"
#include "idt/idt.h"
#include <stdint.h>
#include <stddef.h>

#define CLASSIC_KEYBOARD_CAPSLOCK 0x3A
// Scancode values for the left and right shift keys
#define CLASSIC_KEYBOARD_LSHIFT 0x2A
#define CLASSIC_KEYBOARD_RSHIFT 0x36

int classic_keyboard_init();

/*
 * Lookup table for set-1 scancodes. The index is the raw scancode read
 * from the controller and the value is the base ASCII character before
 * any shift/caps lock modifiers are applied.
 */
static uint8_t keyboard_scan_set_one[] = {
    0x00, 0x1B, '1', '2', '3', '4', '5',
    '6', '7', '8', '9', '0', '-', '=',
    0x08, '\t', 'Q', 'W', 'E', 'R', 'T',
    'Y', 'U', 'I', 'O', 'P', '[', ']',
    0x0d, 0x00, 'A', 'S', 'D', 'F', 'G',
    'H', 'J', 'K', 'L', ';', '\'', '`',
    0x00, '\\', 'Z', 'X', 'C', 'V', 'B',
    'N', 'M', ',', '.', '/', 0x00, '*',
    0x00, 0x20, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, '7', '8', '9', '-', '4', '5',
    '6', '+', '1', '2', '3', '0', '.'
};

/*
 * Driver instance registered with the keyboard subsystem. The caps lock
 * and shift flags are stored inside this structure so the interrupt
 * handler can update them as scancodes are processed.
 */
static struct keyboard classic_keyboard = {
    .name = {"Classic"},
    .init = classic_keyboard_init
};

void classic_keyboard_handle_interrupt();

/* Set up the IRQ handler and reset driver state. */
int classic_keyboard_init()
{
    /* Bind the interrupt handler so we receive IRQ 1 events. */
    idt_register_interrupt_callback(ISR_KEYBOARD_INTERRUPT, classic_keyboard_handle_interrupt);

    /* Reset caps lock and shift flags. */
    keyboard_set_capslock(&classic_keyboard, KEYBOARD_CAPS_LOCK_OFF);
    keyboard_set_shift(&classic_keyboard, KEYBOARD_SHIFT_OFF);

    /*
     * Issue a command to the PS/2 controller's command port (0x64)
     * to enable the first keyboard port.  Without this the controller
     * would ignore key presses.
     */
    outb(PS2_PORT, PS2_COMMAND_ENABLE_FIRST_PORT);
    return 0;
}

/*
 * Convert a raw set-1 scancode to an ASCII character. The lookup table
 * yields a base character which is then adjusted based on the current
 * shift and caps lock state tracked in `classic_keyboard`.
 *
 * Returns 0 when the scancode is outside the table or does not map to a
 * printable character.
 */
uint8_t classic_keyboard_scancode_to_char(uint8_t scancode)
{
    size_t size_of_keyboard_set_one = sizeof(keyboard_scan_set_one) / sizeof(uint8_t);
    if (scancode > size_of_keyboard_set_one)
    {
        return 0;
    }

    char c = keyboard_scan_set_one[scancode];

    /*
     * When shift is held down digits and punctuation map to the
     * alternative characters typically found above them on a US
     * keyboard. Letters are handled later when determining case.
     */
    if (keyboard_get_shift(&classic_keyboard))
    {
        switch (c)
        {
        case '1': c = '!'; break;
        case '2': c = '@'; break;
        case '3': c = '#'; break;
        case '4': c = '$'; break;
        case '5': c = '%'; break;
        case '6': c = '^'; break;
        case '7': c = '&'; break;
        case '8': c = '*'; break;
        case '9': c = '('; break;
        case '0': c = ')'; break;
        case '-': c = '_'; break;
        case '=': c = '+'; break;
        case '[': c = '{'; break;
        case ']': c = '}'; break;
        case ';': c = ':'; break;
        case '\'': c = '"'; break;
        case '`': c = '~'; break;
        case '\\': c = '|'; break;
        case ',': c = '<'; break;
        case '.': c = '>'; break;
        case '/': c = '?'; break;
        default: break;
        }
    }

    /*
     * Determine if letters should be uppercase. Caps lock inverts the shift
     * state so we XOR the two flags. Digits and punctuation are unaffected
     * by caps lock.
     */
    int uppercase = keyboard_get_capslock(&classic_keyboard) ^ keyboard_get_shift(&classic_keyboard);
    /* Letters are stored in the table as uppercase. Convert to
     * lowercase when appropriate or vice versa depending on the
     * computed `uppercase` flag above.
     */
    if (c >= 'A' && c <= 'Z')
    {
        if (!uppercase)
        {
            c += 32;
        }
    }
    else if (c >= 'a' && c <= 'z')
    {
        /* letters from the table are already lowercase here. Promote to
         * uppercase when needed.
         */
        if (uppercase)
        {
            c -= 32;
        }
    }

    return c;
}

/*
 * Interrupt handler for IRQ 1. Reads a byte from port 0x60 and updates
 * the driver's shift and caps lock state. Printable characters are
 * translated and pushed into the keyboard buffer for later consumption.
 */
void classic_keyboard_handle_interrupt()
{
    /*
     * Read the scancode from the keyboard data port (0x60).  Some
     * controllers require a second read to acknowledge the interrupt,
     * so the value returned by the second read is ignored.
     */
    uint8_t scancode = insb(KEYBOARD_INPUT_PORT);
    insb(KEYBOARD_INPUT_PORT);

    /* The high bit signals key release. Mask it off to get the keycode. */
    int released = scancode & CLASSIC_KEYBOARD_KEY_RELEASED;
    uint8_t keycode = scancode & ~CLASSIC_KEYBOARD_KEY_RELEASED;

    /* Update shift state when either shift key is pressed or released. */
    if (keycode == CLASSIC_KEYBOARD_LSHIFT || keycode == CLASSIC_KEYBOARD_RSHIFT)
    {
        keyboard_set_shift(&classic_keyboard, released ? KEYBOARD_SHIFT_OFF : KEYBOARD_SHIFT_ON);
        return;
    }

    /* Ignore release events for other keys. */
    if (released)
    {
        return;
    }

    /* Toggle caps lock state on key press. */
    if (scancode == CLASSIC_KEYBOARD_CAPSLOCK)
    {
        KEYBOARD_CAPS_LOCK_STATE old_state = keyboard_get_capslock(&classic_keyboard);
        keyboard_set_capslock(&classic_keyboard, old_state == KEYBOARD_CAPS_LOCK_ON ? KEYBOARD_CAPS_LOCK_OFF : KEYBOARD_CAPS_LOCK_ON);
        return;
    }

    uint8_t c = classic_keyboard_scancode_to_char(scancode);
    if (c != 0)
    {
        keyboard_push(c);
    }
}

struct keyboard* classic_init()
{
    /* Return the driver descriptor so the keyboard subsystem can
     * register it and access its state fields. */
    return &classic_keyboard;
}
