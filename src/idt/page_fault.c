#include "kernel.h"
#include "string/string.h"
#include <stdint.h>

void page_fault_handler(uint64_t fault_addr, uint64_t error_code)
{
    char buf[32];

    print("Page fault at address ");
    int_to_string((int)fault_addr, buf);
    print(buf);
    print(", error code ");
    int_to_string((int)error_code, buf);
    print(buf);
    print("\n");

    panic("Halting.\n");
}
