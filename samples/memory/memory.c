/* sample_memory: tbox memory pool and allocators.
 * Wraps a plain buffer in the tbox default allocator and does all
 * allocations through the pool, plus a fixed-pool demo.
 */
#include <stdlib.h>
#include <tbox/tbox.h>

int main(int argc, char** argv)
{
    /* -- init the tbox memory pool over a plain buffer --
     * tb_default_allocator turns static memory into a tbox-managed pool,
     * everything allocated with tb_malloc afterwards comes from it.
     */
    tb_size_t const pool_size = 128 * 1024 * 1024;
    tb_byte_t* pool_data = (tb_byte_t*)malloc(pool_size);
    if (!pool_data) return 2;

    tb_allocator_ref_t pool = tb_default_allocator(pool_data, pool_size);
    if (!pool)
    {
        free(pool_data);
        return 2;
    }

    if (!tb_init(tb_null, pool))
    {
        free(pool_data);
        return 2;
    }

    /* -- plain allocations through the pool -- */
    tb_pointer_t data = tb_malloc(1024);
    tb_pointer_t data0 = tb_malloc0(3 * 1024);
    if (!data || !data0)
    {
        tb_printf("[memory] malloc failed\n");
        tb_exit();
        return 1;
    }
    tb_printf("[memory] malloc 1024 -> %p, malloc0 3072 -> %p\n", data, data0);

    data = tb_ralloc(data, 2048);
    tb_printf("[memory] realloc to 2048 -> %p\n", data);
    if (data) tb_free(data);
    if (data0) tb_free(data0);

    /* -- fixed pool: fixed-size slot allocator -- */
    {
        tb_fixed_pool_ref_t fixed = tb_fixed_pool_init(tb_null, 0, 64, tb_null, tb_null, tb_null);
        if (fixed)
        {
            tb_pointer_t items[8] = {0};
            tb_size_t i = 0;
            for (i = 0; i < 8; i++) items[i] = tb_fixed_pool_malloc(fixed);
            tb_printf("[memory] fixed pool: allocated 8 slots of 64 bytes\n");
            for (i = 0; i < 8; i++)
                if (items[i]) tb_fixed_pool_free(fixed, items[i]);
            tb_fixed_pool_exit(fixed);
            tb_printf("[memory] fixed pool: freed all slots\n");
        }
        else
        {
            tb_printf("[memory] fixed pool init failed\n");
        }
    }

    /* dump only prints in debug builds of tbox (memory check mode) */
    tb_allocator_dump(tb_allocator());
    tb_printf("[memory] allocator dump issued (leak check active in debug tbox builds)\n");

    tb_exit();
    free(pool_data);
    return 0;
}
