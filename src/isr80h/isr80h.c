#include "isr80h.h"
#include "idt/idt.h"
#include "io.h"
#include "heap.h"
#include "process.h"
#include "misc.h"
#include "vana/syscall.h"

void isr80h_register_commands()
{
    /* command table no longer used */
}

void isr80h_syscall(struct interrupt_frame* context)
{
    switch (context->eax)
    {
        case VANA_SYS_EXIT:
            context->eax = isr80h_command9_exit(context);
            break;
        /* handle other VANA_SYS_* calls */
        default:
            context->eax = VANA_ENOSYS;
            return;
    }
}
