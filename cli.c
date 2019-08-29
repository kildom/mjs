/*


byte[8] version

int32 length = including 'length', 'hash' and bytecode
byte[16] hash = MD5_bin(source_code)
byte[] bytecode

...

int32 length == 0

MD5_hex(source_code).js => source code
MD5_hex(bytecode_offset & MD5_bin(source_code) & bytecode).map => line number map


 */
#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "mjs/src/mjs_core_public.h"
#include "common/mbuf.h"
#include "common/cs_varint.h"
#include "compiler.h"

#define MEM_CHECK(...)      // TODO
#define COMPILER_CHECK(...) // TODO

void print_usage(FILE *out)
{
    static const char *text =
        "\nUSAGE: mjsc options source.js [source.js [...]]\n\n"
        " -o file      Output file ('a.bin' by default).\n"
        " -f format    Output file format:\n"
        "              bin - raw binary data (default).\n"
        "              hex - intel hex file.\n"
        "              c   - c source code with byte array.\n"
        " -s dir       Write source code cache files.\n"
        " -m dir       Write line number map cache files.\n"
        " -a address   Specifies starting address for 'hex' file format (0 by default).\n"
        "              Hex values should be prefixed by '0x'.\n"
        " -d decl      Specifies variable declaration for 'c' file format\n"
        "              ('static const unsigned char' by default).\n"
        " -n name      Specifies variable name for 'c' file format\n"
        "              ('bytecode' by default).\n"
        " -w           Write output using <CL><LR> line endings.\n"
        "\n";
    fprintf(out, "%s", text);
}

typedef enum file_format
{
    FORMAT_BIN,
    FORMAT_HEX,
    FORMAT_C,
} file_format_t;

static const char *output_file = "a.bin";
static file_format_t output_format = FORMAT_BIN;
static const char *source_dir = NULL;
static const char *map_dir = NULL;
static uint32_t hex_start_address = 0x00000000;
static const char *c_declaration = "static const unsigned char";
static const char *c_name = "bytecode";
static const char *line_end = "\n";

static void remove_trailing_slashes(char *str)
{
    int len = 0;
    while (str[len])
    {
        if (str[len] == '\\')
        {
            str[len] = '/';
        }
        len++;
    }
    len--;
    while (len >= 0 && str[len] == '/')
    {
        str[len] = 0;
    }
}

static void parse_options(int argc, char *argv[])
{
    char *endptr;
    int c;

    if (argc <= 1)
    {
        print_usage(stdout);
        exit(3);
    }

    opterr = 0;

    while ((c = getopt(argc, argv, "o:f:s:m:a:d:n:w")) != -1)
    {
        switch (c)
        {
        case 'o':
            output_file = strdup(optarg);
            MEM_CHECK(output_file);
            break;
        case 'f':
            if (stricmp(optarg, "bin") == 0)
                output_format = FORMAT_BIN;
            else if (stricmp(optarg, "hex") == 0)
                output_format = FORMAT_HEX;
            else if (stricmp(optarg, "c") == 0)
                output_format = FORMAT_C;
            else
            {
                fprintf(stderr, "Invalid file format.\n");
                exit(3);
            }
            break;
        case 's':
            source_dir = strdup(optarg);
            MEM_CHECK(source_dir);
            remove_trailing_slashes(source_dir);
            break;
        case 'm':
            map_dir = strdup(optarg);
            MEM_CHECK(map_dir);
            remove_trailing_slashes(map_dir);
            break;
        case 'a':
            endptr = NULL;
            hex_start_address = strtol(optarg, &endptr, 0);
            if (endptr != optarg + strlen(optarg))
            {
                fprintf(stderr, "Invalid start address format\n");
                exit(3);
            }
            break;
        case 'd':
            c_declaration = strdup(optarg);
            MEM_CHECK(c_declaration);
            break;
        case 'n':
            c_name = strdup(optarg);
            MEM_CHECK(c_name);
            break;
        case 'w':
            line_end = "\r\n";
            break;
        case '?':
        default:
            if ((optopt >= 'a' && optopt <= 'z') || (optopt >= 'A' && optopt <= 'Z') || (optopt >= '0' && optopt <= '9'))
                fprintf(stderr, "Invalid option '-%c'.\n", optopt);
            else
                fprintf(stderr, "Unknown option character '\\x%02x'.\n", optopt);
            print_usage(stderr);
            exit(3);
            break;
        }
    }

    if (optind >= argc)
    {
        fprintf(stderr, "No source file provided!\n");
        exit(3);
    }
}

static struct compiler *compiler;
static struct mbuf output;

void read_file(const char *file_name, struct mbuf *buf)
{
    size_t done = 0;
    FILE *f = fopen(file_name, "rb");
    if (!f)
    {
        fprintf(stderr, "Cannot read file '%s'!", file_name);
        exit(4);
    }
    if (buf->size < 1024)
    {
        mbuf_resize(buf, 1024);
        MEM_CHECK(buf->size >= 1024);
    }
    while (1)
    {
        int n = fread(&buf->buf[done], 1, buf->size - done, f);
        if (n < 0)
        {
            fprintf(stderr, "Cannot read file '%s'!", file_name);
            exit(4);
        }
        else if (n == 0)
        {
            buf->buf[done] = 0;
            break;
        }
        else if (n >= buf->size - done)
        {
            mbuf_resize(buf, buf->size * 2);
        }
        done += n;
    }
    buf->len = done;
    fclose(f);
}

void write_file(const char *file_name, const char *data, size_t length)
{
    FILE *f = fopen(file_name, "wb");
    if (!f)
    {
        fprintf(stderr, "Cannot write file '%s'!", file_name);
        exit(4);
    }
    while (length > 0)
    {
        int n = fwrite(data, 1, length, f);
        if (n <= 0)
        {
            fprintf(stderr, "Cannot write file '%s'!", file_name);
            exit(4);
        }
        data += n;
        length -= n;
    }
    fclose(f);
}

void write_bin(const char* buf, size_t len)
{
    write_file(output_file, buf, len);
}

void write_hex(const char* buf, size_t len)
{
    fprintf(stderr, "HEX format: Not implemented");
    exit(10);
}

void write_c(const char* buf, size_t len)
{
    fprintf(stderr, "C format: Not implemented");
    exit(10);
}

int main(int argc, char *argv[])
{
    unsigned char *data;
    int length;
    int i;
    int res;

    parse_options(argc, argv);

    mbuf_init(&output, 1024);
    MEM_CHECK(output.size >= 1024);

    compiler = c_create();
    MEM_CHECK(compiler);

    res = c_begin(compiler, &data, &length);
    COMPILER_CHECK(res);
    mbuf_append(&output, data, length);

    struct mbuf buf;
    char *temp = malloc((source_dir ? strlen(source_dir) : 0) + (map_dir ? strlen(map_dir) : 0) + 40);

    mbuf_init(&buf, 16 * 1024);

    for (i = optind; i < argc; i++)
    {
        read_file(argv[i], &buf);
        res = c_compile(compiler, buf.buf, argv[i], &data, &length);
        COMPILER_CHECK(res);
        mbuf_append(&output, data, length);
        if (source_dir)
        {
            res = c_get_source_hash(compiler, &data, &length);
            COMPILER_CHECK(res);
            sprintf(temp, "%s/%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X.js", source_dir,
                    data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
                    data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
            write_file(temp, buf.buf, buf.len);
        }
        if (map_dir)
        {
            res = c_get_map_hash(compiler, &data, &length);
            COMPILER_CHECK(res);
            sprintf(temp, "%s/%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X.map", map_dir,
                    data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
                    data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
            res = c_get_map_data(compiler, &data, &length);
            COMPILER_CHECK(res);
            write_file(temp, data, length);
        }
    }

    res = c_end(compiler, &data, &length);
    COMPILER_CHECK(res);
    mbuf_append(&output, data, length);

    c_free(compiler);

    switch (output_format)
    {
    case FORMAT_BIN:
        write_bin(output.buf, output.len);
        break;
    case FORMAT_HEX:
        write_hex(output.buf, output.len);
        break;
    case FORMAT_C:
        write_c(output.buf, output.len);
        break;
    }

    mbuf_free(&output);

    return 0;
}

#if 0
#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "mjs/src/mjs_core.h"
#include "mjs/src/mjs_core_public.h"
#include "mjs/src/mjs_exec_public.h"
#include "common/mbuf.h"
#include "common/cs_varint.h"
#include "compiler.h"
#include "md5.h"

struct mjs* instance;


int main(int argc, char* argv[])
{
    int res;
    const char *p;
    size_t len;
    mjs_val_t ret;

    instance = mjs_create();

    struct mbuf line_no_map;

    res = mjs_compile(instance, "<stdin>", "\r\nlet x = 123;\r\n(x+1);\r\nfunction f() { break; }; f();", &p, &len, &line_no_map);
    if (res != MJS_OK)
    {
        printf("%s\n", mjs_strerror(instance, res));
    }

    printf("%d %d \n", len, line_no_map.len);

    instance = mjs_create();

    res = mjs_exec_compiled(instance, p, len, &ret);
    if (res != MJS_OK)
    {
        printf("%s\n", mjs_strerror(instance, res));
        printf("%s\n", instance->stack_trace);
    }
    
    printf("OK\n");
    return 0;
}
#endif
