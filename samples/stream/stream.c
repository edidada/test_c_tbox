/* sample_stream: unified stream interface (file IO, blocking mode),
 * static_stream bit-level access and in-memory data streams.
 */
#include <tbox/tbox.h>

int main(int argc, char** argv)
{
    if (!tb_init(tb_null, tb_null)) return 1;

    tb_char_t const* path = argc > 1 ? argv[1] : "tbox_stream_demo.bin";

    /* ---- 1. write a file through the stream interface (blocking) ---- */
    tb_stream_ref_t output = tb_stream_init_from_file(path,
        TB_FILE_MODE_WO | TB_FILE_MODE_CREAT | TB_FILE_MODE_TRUNC);
    if (!output || !tb_stream_open(output))
    {
        tb_printf("[stream] open %s for writing failed\n", path);
        if (output) tb_stream_exit(output);
        tb_exit();
        return 1;
    }

    if (!tb_stream_bwrit_u32_le(output, 0x54424f58)      /* magic "TBOX" */
        || !tb_stream_bwrit_u16_le(output, 1337)         /* version */
        || !tb_stream_bwrit_u8(output, 42))              /* flags */
    {
        tb_printf("[stream] write header failed: %s\n", tb_state_cstr(tb_stream_state(output)));
    }
    else if (tb_stream_printf(output, "hello from tbox stream!\n") <= 0)
    {
        tb_printf("[stream] write text failed\n");
    }
    else
    {
        tb_printf("[stream] wrote header + text, offset: %llu\n", tb_stream_offset(output));
    }

    tb_stream_sync(output, tb_true);
    tb_stream_close(output);
    tb_stream_exit(output);

    /* ---- 2. read it back again (blocking) ---- */
    tb_stream_ref_t input = tb_stream_init_from_file(path, TB_FILE_MODE_RO);
    if (!input || !tb_stream_open(input))
    {
        tb_printf("[stream] open %s for reading failed\n", path);
        if (input) tb_stream_exit(input);
        tb_exit();
        return 1;
    }

    tb_uint32_t magic = 0;
    tb_uint16_t version = 0;
    tb_uint8_t flags = 0;
    if (tb_stream_bread_u32_le(input, &magic)
        && tb_stream_bread_u16_le(input, &version)
        && tb_stream_bread_u8(input, &flags))
    {
        tb_printf("[stream] header ok, magic: %#x, version: %u, flags: %u\n", magic, version, flags);
    }
    else
    {
        tb_printf("[stream] header read failed\n");
    }

    tb_char_t line[256] = {0};
    while (tb_stream_bread_line(input, line, sizeof(line)) > 0)
        tb_printf("[stream] line: %s", line);

    tb_printf("[stream] file size: %llu, closed: %s\n",
        tb_stream_size(input), tb_stream_is_closed(input) ? "no" : "yes");

    tb_stream_close(input);
    tb_stream_exit(input);

    /* ---- 3. static_stream: in-memory bit access ---- */
    {
        tb_byte_t sdata[8] = {0};
        tb_static_stream_t sstream;
        if (tb_static_stream_init(&sstream, sdata, sizeof(sdata))
            && tb_static_stream_writ_ubits32(&sstream, 0xab, 8)
            && tb_static_stream_writ_ubits32(&sstream, 0xcdef, 16)
            && tb_static_stream_writ_u8(&sstream, 0x5a)
            && tb_static_stream_sync(&sstream))
        {
            tb_static_stream_goto(&sstream, sdata);
            tb_printf("[stream] static bits: %x, %x, %x\n",
                tb_static_stream_read_ubits32(&sstream, 8),
                tb_static_stream_read_ubits32(&sstream, 16),
                tb_static_stream_read_u8(&sstream));
        }
    }

    /* ---- 4. data stream: stream over memory ---- */
    {
        tb_byte_t const data[] = "the tbox data stream reads memory like a file";
        tb_stream_ref_t ds = tb_stream_init_from_data(data, (tb_size_t)tb_strlen((tb_char_t const*)data));
        if (ds && tb_stream_open(ds))
        {
            tb_byte_t chunk[16] = {0};
            tb_size_t total = 0;
            tb_long_t real = 0;
            while ((real = tb_stream_read(ds, chunk, sizeof(chunk) - 1)) > 0)
                total += (tb_size_t)real;
            tb_printf("[stream] data stream read: %lu bytes\n", total);
            tb_stream_close(ds);
        }
        if (ds) tb_stream_exit(ds);
    }

    tb_exit();
    return 0;
}
