/* sample_coroutine: stackful coroutines with scheduler, channel
 * communication and coroutine switching.
 */
#include <tbox/tbox.h>

#ifndef TB_CONFIG_MODULE_HAVE_COROUTINE
int main(void)
{
    tb_printf("[coroutine] tbox built without the coroutine module\n");
    return 0;
}
#else

/* computing values in a coroutine and sending them through a channel */
static tb_void_t producer(tb_cpointer_t priv)
{
    tb_co_channel_ref_t channel = (tb_co_channel_ref_t)priv;
    tb_size_t i = 0;
    tb_size_t n = 10;
    for (i = 0; i < n; i++)
    {
        tb_printf("[coroutine] producer(%p): send %lu\n", tb_coroutine_self(), (tb_size_t)i);
        tb_co_channel_send(channel, (tb_cpointer_t)(tb_size_t)(i * 2));
    }
}

static tb_void_t consumer(tb_cpointer_t priv)
{
    tb_co_channel_ref_t channel = (tb_co_channel_ref_t)priv;
    /* two consumers join this coroutine, each receives 5 items,
     * so the total received equals what the producer sends (10 items)
     */
    tb_size_t i = 0;
    tb_size_t n = 5;
    for (i = 0; i < n; i++)
    {
        tb_size_t value = (tb_size_t)tb_co_channel_recv(channel);
        tb_printf("[coroutine] consumer(%p): recv %lu\n", tb_coroutine_self(), value);
    }
}

int main(int argc, char** argv)
{
    if (!tb_init(tb_null, tb_null)) return 1;

    /* init the coroutine scheduler (stackful) */
    tb_co_scheduler_ref_t scheduler = tb_co_scheduler_init();
    if (!scheduler)
    {
        tb_printf("[coroutine] init scheduler failed\n");
        tb_exit();
        return 1;
    }

    /* a channel with buffer size 1; send/recv suspend coroutines on full/empty */
    tb_co_channel_ref_t channel = tb_co_channel_init(1, tb_null, tb_null);
    if (!channel)
    {
        tb_printf("[coroutine] init channel failed\n");
        tb_co_scheduler_exit(scheduler);
        tb_exit();
        return 1;
    }

    /* one producer feeds two consumers over the channel */
    tb_coroutine_start(scheduler, producer, channel, 0);
    tb_coroutine_start(scheduler, consumer, channel, 0);
    tb_coroutine_start(scheduler, consumer, channel, 0);

    /* run until all coroutines finished */
    tb_co_scheduler_loop(scheduler, tb_true);

    tb_co_channel_exit(channel);
    tb_co_scheduler_exit(scheduler);

    tb_printf("[coroutine] done\n");
    tb_exit();
    return 0;
}

#endif /* TB_CONFIG_MODULE_HAVE_COROUTINE */
