#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include <unistd.h>

#include "cic.h"

static int U = 192;
static int D = 125;
static int N = 3;
static int zero_stuffing = 1;
static char out_file[FILENAME_MAX] = "output.raw";

static int16_t getInt16LE(uint8_t *buf)
{
    return (int16_t)((buf[1] << 8) | buf[0]);
    // return res;
}

static void putInt16LE(uint8_t *buf, int16_t data)
{
    buf[0] = data & 0xff;
    buf[1] = (data >> 8) & 0xff;
}

#define HLPMSG                                                                \
    "Usage: cic [options] <file.raw>\n"                                       \
    "\n"                                                                      \
    "Options: \n"                                                             \
    "  -u<num>   Upsampling factor (default: %d)\n"                           \
    "  -d<num>   Downsampling factor (default: %d)\n"                         \
    "  -n<num>   Number of stages (default: %d)\n"                            \
    "  -z<num>   Use zero stuffing on upsampling. 0:off 1:on (default:%d).\n" \
    "  -o<file>  Output filename (default: %s)\n"                             \
    "  -h        Print this help.\n"                                          \
    "\n"

static void print_help()
{
    printf(HLPMSG, U, D, N, zero_stuffing, out_file);
}

static void parse_options(int argc, char **argv)
{
    int ch;
    while ((ch = getopt(argc, argv, "u:d:n:o:z:h")) != -1)
    {
        switch (ch)
        {
        case 'u':
            U = atoi(optarg);
            break;
        case 'd':
            D = atoi(optarg);
            break;
        case 'n':
            N = atoi(optarg);
            break;
        case 'o':
            strcpy(out_file, optarg);
            break;
        case 'z':
            zero_stuffing = atoi(optarg);
            break;
        case 'h':
            print_help();
            exit(0);
        default:
            printf("Uknown option: %c", ch);
            exit(1);
            break;
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        print_help();
        exit(1);
    }

    parse_options(argc, argv);

    if (optind >= argc)
    {
        print_help();
        exit(1);
    }

    char *inp_file = argv[optind];

    printf("Input: %s\n", inp_file);
    printf("Upsampling factor: %d\n", U);
    printf("Downsampling factor: %d\n", D);
    printf("Number of Cascading: %d\n", N);
    printf("Output: %s\n", out_file);
    printf("\n");

    FILE *fp = fopen(inp_file, "rb");
    if (fp == NULL)
    {
        printf("Error: File not found.\n");
        exit(1);
    }

    if (fseek(fp, 0, SEEK_END) != 0)
    {
        printf("Error: Seek Error.\n");
        exit(1);
    }

    long file_size = ftell(fp);
    long src_length = file_size / sizeof(int16_t);
    int16_t *src = malloc(file_size);
    fseek(fp, 0, SEEK_SET);
    if (fread(src, file_size, 1, fp) != 1)
    {
        printf("Error: File Read Error.\n");
        exit(1);
    }
    fclose(fp);
    printf("src: %ld samples\n", src_length);

    long dst_length = src_length * U / D;
    int16_t *dst = malloc(sizeof(int16_t) * dst_length);

    CIC *cic_i = CIC_new(U, N, zero_stuffing);
    CIC *cic_d = CIC_new(D, N, zero_stuffing);
    int rp = 0, wp = 0, tick = 0;

    while (rp < src_length)
    {
        int16_t sample = getInt16LE((uint8_t *)(src + rp++));
        CIC_interporate_put(cic_i, sample);
        for (int u = 0; u < U; u++)
        {
            CIC_decimate_put(cic_d, CIC_interporate_get(cic_i));
            if (tick % D == 0)
            {
                putInt16LE((uint8_t *)(dst + wp++), CIC_decimate_get(cic_d));
            }
            tick++;
        }
    }

    fp = fopen(out_file, "wb");
    if (fp == NULL)
    {
        printf("Error: cannnot create file.\n");
        exit(1);
    }
    fwrite(dst, sizeof(int16_t), dst_length, fp);

    CIC_free(cic_i);
    CIC_free(cic_d);

    free(dst);
    free(src);

    return 0;
}