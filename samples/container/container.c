/* sample_container: hash map, vector, stack, queue and algorithms (sort /
 * binary find / count) with tbox iterators.
 */
#include <tbox/tbox.h>

int main(int argc, char** argv)
{
    if (!tb_init(tb_null, tb_null)) return 1;

    /* ---- 1. hash map: c-string -> long ---- */
    tb_hash_map_ref_t map = tb_hash_map_init(8, tb_element_str(tb_true), tb_element_long());
    if (!map)
    {
        tb_printf("[container] init hash map failed\n");
        tb_exit();
        return 1;
    }

    tb_hash_map_insert(map, "tbox", (tb_pointer_t)4);
    tb_hash_map_insert(map, "xmake", (tb_pointer_t)5);
    tb_hash_map_insert(map, "lua", (tb_pointer_t)3);
    tb_hash_map_insert(map, "c", (tb_pointer_t)1);
    tb_printf("[container] hash map size: %lu\n", tb_hash_map_size(map));
    tb_printf("[container] hash map [tbox]  -> %lu\n", (tb_size_t)tb_hash_map_get(map, "tbox"));
    tb_printf("[container] hash map [xmake] -> %lu\n", (tb_size_t)tb_hash_map_get(map, "xmake"));

    tb_hash_map_remove(map, "lua");
    tb_printf("[container] after remove [lua]: size %lu, get -> %p\n",
        tb_hash_map_size(map), tb_hash_map_get(map, "lua"));
    tb_hash_map_exit(map);

    /* ---- 2. vector ---- */
    {
        tb_vector_ref_t vector = tb_vector_init(4, tb_element_str(tb_true));
        if (vector)
        {
            tb_vector_insert_tail(vector, "stream");
            tb_vector_insert_tail(vector, "coroutine");
            tb_vector_insert_tail(vector, "container");
            tb_vector_insert_tail(vector, "parser");
            tb_printf("[container] vector size: %lu, head: %s, last: %s\n",
                tb_vector_size(vector),
                (tb_char_t const*)tb_vector_head(vector),
                (tb_char_t const*)tb_vector_last(vector));
            tb_vector_exit(vector);
        }
    }

    /* ---- 3. stack (LIFO) ---- */
    {
        tb_stack_ref_t stack = tb_stack_init(0, tb_element_long());
        if (stack)
        {
            tb_size_t n = 5;
            while (n--) tb_stack_put(stack, (tb_pointer_t)(tb_size_t)(n + 1));
            tb_printf("[container] stack pops:");
            while (tb_stack_size(stack))
            {
                tb_printf(" %lu", (tb_size_t)tb_stack_top(stack));
                tb_stack_pop(stack);
            }
            tb_printf("\n");
            tb_stack_exit(stack);
        }
    }

    /* ---- 4. queue (FIFO) ---- */
    {
        tb_queue_ref_t queue = tb_queue_init(0, tb_element_long());
        if (queue)
        {
            tb_size_t n = 5;
            while (n--) tb_queue_put(queue, (tb_pointer_t)(tb_size_t)(n + 1));
            tb_printf("[container] queue gets:");
            while (tb_queue_size(queue))
            {
                tb_printf(" %lu", (tb_size_t)tb_queue_get(queue));
                tb_queue_pop(queue);
            }
            tb_printf("\n");
            tb_queue_exit(queue);
        }
    }

    /* ---- 5. algorithms: sort, binary find, count over iterators ---- */
    {
        tb_long_t data[10] = {9, 1, 7, 3, 5, 8, 2, 6, 4, 0};
        tb_array_iterator_t array_iterator;
        tb_iterator_ref_t iterator = tb_array_iterator_init_long(&array_iterator, data, 10);

        tb_sort_all(iterator, tb_null);
        tb_printf("[container] sorted:");
        {
            tb_size_t i = 0;
            for (i = 0; i < 10; i++)
                tb_printf(" %ld", data[i]);
        }
        tb_printf("\n");
        tb_printf("[container] binary find(6): %lu, count(3): %lu\n",
            tb_binary_find_all(iterator, (tb_cpointer_t)6),
            tb_count_all(iterator, (tb_cpointer_t)3));
    }

    tb_printf("[container] done\n");
    tb_exit();
    return 0;
}
