#include "vana.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
int main(int argc, char** argv)
{
    print(argv[0]);
    for (int i = 0; i < 5; i++)
    {
        print("Hello from blank process!\n");
    }

    return 0;
}
