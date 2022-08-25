/* CIC Filter */
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include <unistd.h>

static int U = 192;
static int D = 125;
static int N = 3;
static int zero_stuffing = 1;
static char out_file[FILENAME_MAX] = "output.wav";

static int16_t *upsample(int16_t *samples, long length, int M)
{
    int16_t *result = malloc(sizeof(int16_t) * length * M);
    for (long i = 0; i < length; i++)
    {
        result[i * M] = samples[i];
        for (int j = 1; j < M; j++)
        {
            result[i * M + j] = (zero_stuffing != 0) ? 0 : samples[i];
        }
    }
    return result;
}

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

static int16_t *convert(int16_t *samples, long length, int D, int N)
{
    // デシメーション比 通常はダウンサンプリング比と同じ
    int M = D;

    // バッファ。
    // 振幅は最大で M^N 倍まで格納できる必要があるので（入力ビット幅 + N*Log2(M)）ビット必要
    int bits = 16 + ceil(N * log2(M));

    // 振幅は最大で M^N 倍になる（最終出力をこれで割る）
    int64_t range = pow(M, N);

    int gain = (zero_stuffing != 0) ? U : 1;

    printf("register width: %d bits\n", bits);
    if (bits > 64)
    {
        printf("Error: too long buffer bits\n");
        exit(1);
    }

    int64_t ibuf[N], iout[N]; // integrator
    int64_t dbuf[N], dout[N]; // diffrentiator

    memset(ibuf, 0x00, sizeof(int64_t) * N);
    memset(iout, 0x00, sizeof(int64_t) * N);
    memset(dbuf, 0x00, sizeof(int64_t) * N);
    memset(dout, 0x00, sizeof(int64_t) * N);

    int16_t *result = calloc(length / M, sizeof(int16_t));

    long ridx = 0, widx = 0;

    while (ridx < length)
    {
        int16_t x = getInt16LE((uint8_t *)(samples + ridx));
        ridx++;
        iout[0] = x + ibuf[0];
        ibuf[0] = iout[0];
        for (int n = 1; n < N; n++)
        {
            iout[n] = iout[n - 1] + ibuf[n];
            ibuf[n] = iout[n];
        }

        if (ridx % M == 0) // decimation for CIC flow
        {
            dout[0] = iout[N - 1] - dbuf[0];
            dbuf[0] = iout[N - 1];
            for (int n = 1; n < N; n++)
            {
                dout[n] = dout[n - 1] - dbuf[n];
                dbuf[n] = dout[n - 1];
            }
        }

        if (ridx % D == 0) // decimation for Output
        {
            int64_t o = dout[N - 1] / range * gain;
            if (o < -32768 || 32767 < o)
            {
                printf("Error: overflow at index=%ld, value=%ld\n", (long)ridx, (long)o);
                exit(1);
            }
            putInt16LE((uint8_t *)(result + widx), o);
            widx++;
        }
    }

    return result;
}

#define HLPMSG                                                                              \
    "Usage: cic [options] <file.raw>\n"                                                     \
    "\n"                                                                                    \
    "Options: \n"                                                                           \
    "  -u<num>   Upsampling factor (default: %d)\n"                                         \
    "  -d<num>   Downsampling factor (default: %d)\n"                                       \
    "  -n<num>   Number of stages (default: %d)\n"                                          \
    "  -z<num>   Padding zero in upsampling interporation. 0:off 1:on (default: %d).\n" \
    "  -o<file>  Output filename (default: %s)\n"                                           \
    "  -h        Print this help.\n"                                                        \
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

    printf("input: %s\n", inp_file);
    printf("Upsampling factor: %d\n", U);
    printf("Downsampling factor: %d\n", D);
    printf("Number of Cascading: %d\n", N);
    printf("output: %s\n", out_file);
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

    int16_t *mid = upsample(src, src_length, U);
    long mid_length = src_length * U;
    printf("mid: %ld samples\n", mid_length);

    int16_t *dst = convert(mid, mid_length, D, N);
    long dst_length = mid_length / D;
    printf("dst: %ld samples\n", dst_length);

    fp = fopen(out_file, "wb");
    if (fp == NULL)
    {
        printf("Error: cannnot create file.\n");
        exit(1);
    }
    fwrite(dst, sizeof(int16_t), dst_length, fp);

    free(dst);
    free(mid);
    free(src);

    return 0;
}