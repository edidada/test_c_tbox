/* sample_database: the unified sql database interface backed by sqlite3.
 * Needs tbox built with --database=y and the sqlite3 package.
 */
#include <tbox/tbox.h>

#if defined(TB_CONFIG_MODULE_HAVE_DATABASE) && defined(TB_CONFIG_PACKAGE_HAVE_SQLITE3)

static tb_void_t done_sql(tb_database_sql_ref_t database, tb_char_t const* sql)
{
    if (!tb_database_sql_done(database, sql))
    {
        tb_printf("[database] done %s failed: %s\n", sql,
            tb_state_cstr(tb_database_sql_state(database)));
        return;
    }

    tb_iterator_ref_t result = tb_database_sql_result_load(database, tb_true);
    if (!result) return;

    tb_printf("[database] %s -> %lu rows\n", sql, tb_iterator_size(result));

    tb_size_t i = 0;
    tb_size_t rows = tb_iterator_size(result);
    for (i = 0; i < rows; i++)
    {
        tb_iterator_ref_t row = (tb_iterator_ref_t)tb_iterator_item(result, i);
        if (!row) continue;

        tb_size_t j = 0;
        tb_size_t cols = tb_iterator_size(row);
        tb_printf("[database]   row %lu:", i);
        for (j = 0; j < cols; j++)
        {
            tb_database_sql_value_t const* value = (tb_database_sql_value_t const*)tb_iterator_item(row, j);
            if (value)
                tb_printf(" %s=%s", tb_database_sql_value_name(value), tb_database_sql_value_text(value));
        }
        tb_printf("\n");
    }

    tb_database_sql_result_exit(database, result);
}

static tb_void_t demo_database(void)
{
    /* a plain file path url: tbox probes it as sqlite3 (no host/port) */
    tb_database_sql_ref_t database = tb_database_sql_init("tbox_demo.sqlite3");
    if (!database)
    {
        tb_printf("[database] init sqlite3 failed\n");
        return;
    }

    if (!tb_database_sql_open(database))
    {
        tb_printf("[database] open failed: %s\n", tb_state_cstr(tb_database_sql_state(database)));
        tb_database_sql_exit(database);
        return;
    }

    tb_printf("[database] type: %s\n",
        tb_database_sql_type(database) == TB_DATABASE_SQL_TYPE_SQLITE3 ? "sqlite3" : "other");

    done_sql(database, "drop table if exists books");
    done_sql(database, "create table books (id int, name text, price int)");
    done_sql(database, "insert into books values(1, 'tbox in action', 100)");
    done_sql(database, "insert into books values(2, 'hello c', 90)");
    done_sql(database, "insert into books values(3, 'stream & coroutine', 95)");
    done_sql(database, "select * from books");

    /* a prepared statement with bound arguments */
    tb_database_sql_statement_ref_t statement = tb_database_sql_statement_init(database,
        "insert into books values (?, ?, ?)");
    if (statement)
    {
        tb_database_sql_value_t list[3] = {0};
        tb_database_sql_value_set_int32(&list[0], 4);
        tb_database_sql_value_set_text(&list[1], "bound insert", (tb_size_t)tb_strlen("bound insert"));
        tb_database_sql_value_set_int32(&list[2], 88);
        if (tb_database_sql_statement_bind(database, statement, list, tb_arrayn(list)))
        {
            if (tb_database_sql_statement_done(database, statement))
                tb_printf("[database] statement insert ok\n");
            else
                tb_printf("[database] statement done failed: %s\n",
                    tb_state_cstr(tb_database_sql_state(database)));
        }
        tb_database_sql_statement_exit(database, statement);
    }
    done_sql(database, "select * from books");

    tb_database_sql_close(database);
    tb_database_sql_exit(database);
    tb_printf("[database] done\n");
}

#else

static tb_void_t demo_database(void)
{
    tb_printf("[database] tbox built without the database module or without sqlite3\n");
}

#endif

int main(int argc, char** argv)
{
    if (!tb_init(tb_null, tb_null)) return 1;

    demo_database();

    tb_exit();
    return 0;
}
