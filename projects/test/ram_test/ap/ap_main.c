#include "bk_private/bk_init.h"
#include <os/os.h>

int main(void)
{
    bk_init();

    bk_printf("M55 main running...\r\n");

    return 0;
}
