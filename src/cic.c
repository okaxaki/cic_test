#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include "cic.h"

/**
 * @param M Up/Down Sampling Ratio.
 * @param N Number of CIC Stages.
 */
CIC *CIC_new(int M, int N, int zero_stuffing)
{
    int bits = 16 + ceil(N * log2(M));
    printf("register width: %d bits\n", bits);
    if (bits > 64)
    {
        printf("Error: too long buffer bits\n");
        return NULL;
    }

    CIC *cic = calloc(1, sizeof(CIC));
    cic->ibuf = calloc(N, sizeof(*(cic->ibuf)));
    cic->iout = calloc(N, sizeof(*(cic->ibuf)));
    cic->dbuf = calloc(N, sizeof(*(cic->ibuf)));
    cic->dout = calloc(N, sizeof(*(cic->ibuf)));
    cic->N = N;
    cic->M = M;
    cic->range = pow(M, N);
    cic->cycle = 0;
    cic->gain = zero_stuffing == 0 ? 1.0 : M * 0.8;
    cic->zero_stuffing = zero_stuffing;
    return cic;
}

void CIC_reset(CIC *cic)
{
    memset(cic->ibuf, 0, sizeof(*(cic->ibuf)) * cic->N);
    memset(cic->iout, 0, sizeof(*(cic->iout)) * cic->N);
    memset(cic->dbuf, 0, sizeof(*(cic->dbuf)) * cic->N);
    memset(cic->dout, 0, sizeof(*(cic->dout)) * cic->N);
    cic->cycle = 0;
}

void CIC_interporate_put(CIC *cic, int16_t sample)
{
    cic->dout[0] = sample - cic->dbuf[0];
    cic->dbuf[0] = sample;
    for (int n = 1; n < cic->N; n++)
    {
        cic->dout[n] = cic->dout[n - 1] - cic->dbuf[n];
        cic->dbuf[n] = cic->dout[n - 1];
    }
}

int16_t CIC_interporate_get(CIC *cic)
{
    int64_t inp = (cic->zero_stuffing == 0 || cic->cycle % cic->M == 0) ? cic->dout[cic->N - 1] : 0;
    cic->cycle++;

    cic->iout[0] = inp + cic->ibuf[0];
    cic->ibuf[0] = cic->iout[0];
    for (int n = 1; n < cic->N; n++)
    {
        cic->iout[n] = cic->iout[n - 1] + cic->ibuf[n];
        cic->ibuf[n] = cic->iout[n];
    }
    int64_t res = cic->iout[cic->N - 1] / cic->range * cic->gain;
    if (res < -32768 || 32767 < res)
    {
        printf("Error: overflow value=%ld\n", (long)res);
        exit(1);
    }
    return res;
}

void CIC_decimate_put(CIC *cic, int16_t sample)
{
    cic->iout[0] = sample + cic->ibuf[0];
    cic->ibuf[0] = cic->iout[0];
    for (int n = 1; n < cic->N; n++)
    {
        cic->iout[n] = cic->iout[n - 1] + cic->ibuf[n];
        cic->ibuf[n] = cic->iout[n];
    }

    if (cic->cycle % cic->M == 0)
    {
        cic->dout[0] = cic->iout[cic->N - 1] - cic->dbuf[0];
        cic->dbuf[0] = cic->iout[cic->N - 1];
        for (int n = 1; n < cic->N; n++)
        {
            cic->dout[n] = cic->dout[n - 1] - cic->dbuf[n];
            cic->dbuf[n] = cic->dout[n - 1];
        }
    }
    cic->cycle++;
}

int16_t CIC_decimate_get(CIC *cic)
{
    return cic->dout[cic->N - 1] / cic->range;
}

void CIC_free(CIC *cic)
{
    free(cic->ibuf);
    free(cic->iout);
    free(cic->dbuf);
    free(cic->dout);
    free(cic);
}