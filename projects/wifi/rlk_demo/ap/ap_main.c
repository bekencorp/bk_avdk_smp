#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>

#include "rlk_demo_cli.h"
#include "rlk_common.h"

RLK_DEMO rlk_demo_env = {0};

void rlk_demo_callback_init(void)
{
    rlk_demo_env.is_inited = true;
    BK_LOG_ON_ERR(bk_rlk_register_recv_cb(rlk_demo_dispatch));
}

uint8_t rlk_rand_chan(void)
{

    int random_chan = rand() % 13 + 1;

    BK_LOGD(NULL, "set current channel is %d\r\n", random_chan);
    return random_chan;
}

int main(void)
{
	bk_init();
    BK_LOG_ON_ERR(bk_rlk_init());
    rlk_demo_callback_init();
    rlk_demo_env.chnl = rlk_rand_chan();

    // set channel
    BK_LOG_ON_ERR(bk_rlk_set_channel(rlk_demo_env.chnl));
    cli_rlk_init();
	return 0;
}
