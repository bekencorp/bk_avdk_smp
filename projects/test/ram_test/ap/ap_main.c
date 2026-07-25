#include "bk_private/bk_init.h"
#include <os/os.h>

int main(void)
{
    bk_init();

    BK_LOGI(NULL, "AP main running...\r\n");

    return 0;
}
