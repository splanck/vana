#include "shell.h"
#include "stdio.h"
#include "stdlib.h"
#include "vana.h"
int main(int argc, char** argv)
{
    print("Vana v1.0.0\n");
    while(1) 
    {
        print("> ");
        char buf[1024];
        vana_terminal_readline(buf, sizeof(buf), true);
        print("\n");
        vana_system_run(buf);
        
        print("\n");
    }
    return 0;
}
