/* sample_parser: data parsing with tbox
 *  - XML streaming (SAX-like reader)
 *  - XML DOM (reader_load + node tree)
 *  - XML writer
 *  - JSON read/serialize and plist output via the object module
 */
#include <tbox/tbox.h>

int main(int argc, char** argv)
{
    if (!tb_init(tb_null, tb_null)) return 1;

#ifdef TB_CONFIG_MODULE_HAVE_XML
    {
        static tb_char_t const xml[] =
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
            "<library>\n"
            "   <book id=\"1\">\n"
            "       <title>tbox in action</title>\n"
            "       <price>9.99</price>\n"
            "   </book>\n"
            "   <book id=\"2\" lang=\"cn\">\n"
            "       <title>tbox in action</title>\n"
            "       <price>19.99</price>\n"
            "   </book>\n"
            "</library>\n";

        /* ---- 1. SAX-like streaming reader ---- */
        tb_xml_reader_ref_t reader = tb_xml_reader_init();
        if (reader && tb_xml_reader_open(reader,
            tb_stream_init_from_data((tb_byte_t const*)xml, sizeof(xml) - 1), tb_true))
        {
            tb_size_t event = TB_XML_READER_EVENT_NONE;
            while ((event = tb_xml_reader_next(reader)) != TB_XML_READER_EVENT_NONE)
            {
                switch (event)
                {
                case TB_XML_READER_EVENT_DOCUMENT:
                    tb_printf("[xml] document: version=%s charset=%s\n",
                        tb_xml_reader_version(reader), tb_xml_reader_charset(reader));
                    break;
                case TB_XML_READER_EVENT_ELEMENT_BEG:
                case TB_XML_READER_EVENT_ELEMENT_EMPTY:
                {
                    tb_printf("[xml] element(%lu): <%s%s>\n", tb_xml_reader_level(reader),
                        event == TB_XML_READER_EVENT_ELEMENT_EMPTY ? "" : "",
                        tb_xml_reader_element(reader));
                    tb_xml_node_ref_t attr = tb_xml_reader_attributes(reader);
                    for (; attr; attr = attr->next)
                        tb_printf("[xml]   attribute: %s = \"%s\"\n",
                            tb_string_cstr(&attr->name), tb_string_cstr(&attr->data));
                    break;
                }
                case TB_XML_READER_EVENT_ELEMENT_END:
                    tb_printf("[xml] element end: </%s>\n", tb_xml_reader_element(reader));
                    break;
                case TB_XML_READER_EVENT_TEXT:
                    tb_printf("[xml] text: %s\n", tb_xml_reader_text(reader));
                    break;
                case TB_XML_READER_EVENT_CDATA:
                    tb_printf("[xml] cdata: %s\n", tb_xml_reader_cdata(reader));
                    break;
                case TB_XML_READER_EVENT_COMMENT:
                    tb_printf("[xml] comment: %s\n", tb_xml_reader_comment(reader));
                    break;
                default:
                    break;
                }
            }
            tb_xml_reader_close(reader);
        }
        if (reader) tb_xml_reader_exit(reader);

        /* ---- 2. DOM: load the whole document into a node tree ---- */
        tb_xml_reader_ref_t reader2 = tb_xml_reader_init();
        tb_xml_node_ref_t document = tb_null;
        if (reader2 && tb_xml_reader_open(reader2,
            tb_stream_init_from_data((tb_byte_t const*)xml, sizeof(xml) - 1), tb_true))
            document = tb_xml_reader_load(reader2);
        if (reader2) tb_xml_reader_exit(reader2);

        if (document)
        {
            /* find the "library" child and walk its "book" elements */
            tb_xml_node_ref_t node = document->chead;
            while (node)
            {
                if (node->type == TB_XML_NODE_TYPE_ELEMENT
                    && !tb_strcmp(tb_string_cstr(&node->name), "library"))
                {
                    tb_xml_node_ref_t book = node->chead;
                    while (book)
                    {
                        if (book->type == TB_XML_NODE_TYPE_ELEMENT)
                        {
                            tb_xml_node_ref_t attr = book->ahead;
                            for (; attr; attr = attr->next)
                                tb_printf("[xml] book attribute: %s = \"%s\"\n",
                                    tb_string_cstr(&attr->name), tb_string_cstr(&attr->data));

                            tb_xml_node_ref_t field = book->chead;
                            while (field)
                            {
                                if (field->type == TB_XML_NODE_TYPE_ELEMENT)
                                {
                                    tb_xml_node_ref_t text = field->chead;
                                    tb_printf("[xml] book.%s = %s\n",
                                        tb_string_cstr(&field->name),
                                        text ? tb_string_cstr(&text->data) : "");
                                }
                                field = field->next;
                            }
                        }
                        book = book->next;
                    }
                }
                node = node->next;
            }

            /* ---- 3. write the DOM tree back as XML ---- */
            tb_xml_writer_ref_t writer = tb_xml_writer_init();
            if (writer)
            {
                tb_stream_ref_t out = tb_stream_init_from_file("tbox_demo_out.xml",
                    TB_FILE_MODE_WO | TB_FILE_MODE_CREAT | TB_FILE_MODE_TRUNC);
                if (out && tb_stream_open(out)
                    && tb_xml_writer_open(writer, tb_true, out, tb_true))
                {
                    tb_xml_writer_save(writer, document);
                    tb_xml_writer_close(writer);
                    tb_printf("[xml] saved DOM to tbox_demo_out.xml\n");
                }
                else
                {
                    tb_printf("[xml] writer open failed\n");
                    if (out) tb_stream_exit(out);
                }
                tb_xml_writer_exit(writer);
            }

            tb_xml_node_exit(document);
        }
        else
        {
            tb_printf("[xml] document load failed\n");
        }
    }
#else
    tb_printf("[xml] tbox built without the xml module\n");
#endif /* TB_CONFIG_MODULE_HAVE_XML */

#ifdef TB_CONFIG_MODULE_HAVE_OBJECT
    {
        static tb_char_t const json[] =
            "{\"name\": \"tbox\", \"features\": [\"stream\", \"coroutine\", \"memory\"], "
            "\"count\": 3, \"price\": 9.99}";

        /* ---- 4. JSON read ---- */
        tb_object_ref_t root = tb_object_read_from_data((tb_byte_t const*)json, (tb_size_t)tb_strlen(json));
        if (root)
        {
            tb_printf("[json] object type: %lu\n", tb_object_type(root));
            if (tb_object_type(root) == TB_OBJECT_TYPE_DICTIONARY)
            {
                tb_iterator_ref_t itor = tb_oc_dictionary_itor(root);
                tb_size_t i = 0;
                tb_size_t count = tb_iterator_size(itor);
                for (i = 0; i < count; i++)
                {
                    tb_oc_dictionary_item_t* item = (tb_oc_dictionary_item_t*)tb_iterator_item(itor, i);
                    if (!item) continue;

                    tb_object_ref_t value = (tb_object_ref_t)item->val;
                    switch (tb_object_type(value))
                    {
                    case TB_OBJECT_TYPE_STRING:
                        tb_printf("[json] %s = \"%s\"\n", item->key, tb_oc_string_cstr(value));
                        break;
                    case TB_OBJECT_TYPE_NUMBER:
                        tb_printf("[json] %s = %lf\n", item->key, tb_oc_number_double(value));
                        break;
                    case TB_OBJECT_TYPE_ARRAY:
                    {
                        tb_iterator_ref_t ait = tb_oc_array_itor(value);
                        tb_size_t j = 0;
                        tb_size_t size = tb_iterator_size(ait);
                        tb_printf("[json] %s = [ ... %lu items ]\n", item->key, size);
                        for (j = 0; j < size; j++)
                        {
                            tb_object_ref_t elem = (tb_object_ref_t)tb_iterator_item(ait, j);
                            if (elem && tb_object_type(elem) == TB_OBJECT_TYPE_STRING)
                                tb_printf("[json]   [%lu] \"%s\"\n", j, tb_oc_string_cstr(elem));
                        }
                        break;
                    }
                    default:
                        tb_printf("[json] %s = (type %lu)\n", item->key, tb_object_type(value));
                        break;
                    }
                }

                /* path seek like $.features[1] */
                tb_object_ref_t found = tb_object_seek(root, "$.features[1]", tb_true);
                if (found && tb_object_type(found) == TB_OBJECT_TYPE_STRING)
                    tb_printf("[json] seek $.features[1] = \"%s\"\n", tb_oc_string_cstr(found));

                /* ---- 5. serialize: json & plist ---- */
                {
                    static tb_byte_t buffer[8192];
                    tb_long_t size = tb_object_writ_to_data(root, buffer, sizeof(buffer), TB_OBJECT_FORMAT_JSON);
                    if (size > 0) tb_printf("[json] serialized: %.*s\n", (int)size, (tb_char_t const*)buffer);

                    size = tb_object_writ_to_data(root, buffer, sizeof(buffer), TB_OBJECT_FORMAT_XML);
                    if (size > 0) tb_printf("[plist] serialized (xml plist): %ld bytes\n", size);
                }
            }
            tb_object_exit(root);
        }
        else
        {
            tb_printf("[json] parse failed\n");
        }
    }
#else
    tb_printf("[json] tbox built without the object module\n");
#endif /* TB_CONFIG_MODULE_HAVE_OBJECT */

    tb_exit();
    return 0;
}
