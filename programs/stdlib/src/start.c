#include "vana.h"

extern int main(int argc, char** argv);

int c_start()
{
    struct process_arguments arguments;
    vana_process_get_arguments(&arguments);

    int res = main(arguments.argc, arguments.argv);
    return res;
}
