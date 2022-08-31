#include <stdint.h>

typedef struct _CIC
{
    int64_t *dout;
    int64_t *dbuf;
    int64_t *iout;
    int64_t *ibuf;    
    int N;
    int M;
    int zero_stuffing;
    int cycle;
    int64_t range;
    double gain;
} CIC;

CIC *CIC_new(int M, int N, int zero_stuffing);

void CIC_reset(CIC *cic);

void CIC_interporate_put(CIC *cic, int16_t sample);
int16_t CIC_interporate_get(CIC *cic);

void CIC_decimate_put(CIC *cic, int16_t sample);
int16_t CIC_decimate_get(CIC *cic);

void CIC_free(CIC *cic);
