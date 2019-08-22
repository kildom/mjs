
#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "mjs/src/mjs_core_public.h"
#include "common/mbuf.h"
#include "common/cs_varint.h"

/*

varint version


varint flags // BYTECODE_PRESENT, MAP_PRESENT, SOURCE_PRESENT, HASH_PRESENT

varint bytecode_len
uint8_t bytecode[]

varint map_len
uint8_t map[]

varint compressed_source_len
uint8_t compressed_source[] // contains uncompressed_source_len

varint hash_len // == 16
uint8_t hash[16] // == MD5(uncompressed_source)


...

varint flags // == END_OF_DATA

 */

void printUsage()
{
    static const char *text =
        "\nUSAGE: mjsc options source.js [source.js [...]]\n\n"
        " -o file      Output file. File format and content must be .\n"
        " -f format    Output file format:\n"
        "              bin - raw binary data (default)\n"
        "              hex - intel hex file\n"
        "              c   - c source code with byte array\n"
        " -b           Place bytecode in the output\n"
        " -s           Place compressed source code in the output\n"
        " -m           Place line number map in the output\n"
        " -h           Place MD5 of the source code in the output\n"
        "              If none of -b, -s, -m, -h is specified then all are placed.\n"
        " -a address   Specifies starting address for 'hex' file format (0 by default)\n"
        "              Hex values should be prefixed by '0x'\n"
        " -d decl      Specifies variable declaration for 'c' file format \n"
        "              ('static const unsigned char' by default)\n"
        " -n name      Specifies variable name for 'c' file format \n"
        "              ('bytecode' by default)\n"
        " -i file      Specifies already compiled raw binary file that will be used\n"
        "              instead of source files. This option can be used to convert or\n"
        "              create multiple outputs.\n\n";
    fprintf(stderr, "%s", text);
    exit(1);
}

#define INIT_BUFFER_SIZE (8 * 1024)
#ifndef MJS_VERSION
#define MJS_VERSION 0uLL
#endif

#define INCLUDE_DEFAULT 0
#define INCLUDE_BYTECODE 1
#define INCLUDE_SOURCE 2
#define INCLUDE_MAP 4
#define INCLUDE_HASH 8
#define END_OF_DATA_FLAG 0x2000

typedef enum file_format
{
    FORMAT_BIN,
    FORMAT_HEX,
    FORMAT_C,
} file_format_t;

static file_format_t output_format = FORMAT_BIN;
static int output_content = INCLUDE_DEFAULT;
static uint32_t hex_start_address = 0x00000000;
static const char *c_declaration = "static const unsigned char";
static const char *c_name = "bytecode";
static const char *output_file = NULL;
static const char *input_file = NULL;

static struct mbuf input_buffer;

static void parse_options(int argc, char *argv[])
{

    char *endptr;
    int c;
    int i;
    int res;
    opterr = 0;

    while ((c = getopt(argc, argv, "o:f:bsmha:d:n:i:")) != -1)
    {
        switch (c)
        {
        case 'o':
            output_file = strdup(optarg);
            break;
        case 'f':
            check_current_output();
            if (stricmp(optarg, "bin") == 0)
                output_format = FORMAT_BIN;
            else if (stricmp(optarg, "hex") == 0)
                output_format = FORMAT_HEX;
            else if (stricmp(optarg, "c") == 0)
                output_format = FORMAT_C;
            else
            {
                fprintf(stderr, "Invalid file format.\n");
                exit(4);
            }
            break;
        case 'b':
            output_content |= INCLUDE_BYTECODE;
            break;
        case 's':
            output_content |= INCLUDE_SOURCE;
            break;
        case 'm':
            output_content |= INCLUDE_MAP;
            break;
        case 'h':
            output_content |= INCLUDE_HASH;
            break;
        case 'a':
            endptr = NULL;
            hex_start_address = strtol(optarg, &endptr, 0);
            if (endptr != optarg + strlen(optarg))
            {
                fprintf(stderr, "Invalid start address format\n");
                exit(5);
            }
            break;
        case 'd':
            c_declaration = strdup(optarg);
            break;
        case 'n':
            c_name = strdup(optarg);
            break;
        case 'i':
            input_file = strdup(optarg);
            break;
        case '?':
        default:
            if ((optopt >= 'a' && optopt <= 'z') || (optopt >= 'A' && optopt <= 'Z') || (optopt >= '0' && optopt <= '9'))
                fprintf(stderr, "Invalid option '-%c'.\n", optopt);
            else
                fprintf(stderr, "Unknown option character '\\x%02x'.\n", optopt);
            printUsage();
            break;
        }
    }

    if (output_content == INCLUDE_DEFAULT) 
    {
        output_content = INCLUDE_BYTECODE | INCLUDE_SOURCE | INCLUDE_MAP | INCLUDE_HASH;
    }
}

static void read_input()
{
    exit(111); // TODO
}

size_t mbuf_append_varint(struct mbuf *a, uint64_t num)
{
    uint8_t temp[11];
    size_t num = cs_varint_encode(num, temp, sizeof(temp));
    size_t res = mbuf_append(a, temp, num);
    check_alloc((void *)res);
}

static void compile_sources(int argc, char *argv[])
{
    int i;
    int res;
    unsigned char temp[INIT_BUFFER_SIZE];
    struct mbuf buf;
    struct mjs *mjs = mjs_create();

    mbuf_init(&input_buffer, INIT_BUFFER_SIZE);
    mbuf_init(&buf, INIT_BUFFER_SIZE);

    check_alloc(mjs);
    check_alloc(input_buffer.buf);
    check_alloc(buf.buf);

    mbuf_append_varint(&input_buffer, MJS_VERSION);

    for (i = optind; i < argc; i++)
    {
        const char *name;
        FILE *f;

        mbuf_append_varint(&input_buffer, output_content);

        if (strcmp(argv[i], "-") == 0)
        {
            name = NULL;
            f = stdin;
        }
        else
        {
            name = strdup(argv[i]);
            do
            {
                char *sep_win = strchr(argv[i], '\\');
                if (sep_win == NULL) break;
                *sep_win = '/';
            } while (1);
            f = fopen(argv[i], "rb");
        }

        if (!f)
        {
            fprintf(stderr, "Cannot open source file '%s'\n", argv[i]);
            exit(6);
        }

        mbuf_clear(&buf);

        do
        {
            int n = fread(temp, 1, sizeof(temp), f);
            if (n == 0)
                break;
            if (n < 0)
            {
                fprintf(stderr, "Error reading file '%s'\n", argv[i]);
                exit(7);
            }
            n = mbuf_append(&buf, temp, n);
            check_alloc((void *)n);
        } while (1);

        if (f != stdin)
            fclose(f);

        const char *bytecode;
        size_t bytecode_length;
        res = mjs_compile(mjs, name, buf.buf, &bytecode, &bytecode_length);
        if (res != MJS_OK)
        {
            const char *error_string = mjs_strerror(mjs, res);
            fprintf(stderr, "%s: ERROR: %s\n", argv[i], error_string);
            exit(1);
        }
    }

    mbuf_free(&buf);
}

int main(int argc, char *argv[])
{
    char *endptr;
    int c;
    int i;
    int res;
    opterr = 0;

    parse_options(argc, argv);

    if (input_file != NULL)
    {
        if (optind < argc)
        {
            fprintf(stderr, "Source file is not allowed if -i options was provided\n");
            exit(8);
        }
        read_input();
    }
    else
    {
        compile_sources(argc, argv);
    }

    printUsage();
    return 0;
}
