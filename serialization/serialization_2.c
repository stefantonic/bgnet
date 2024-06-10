#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>

#define pack754_32(f) (pack754((f), 32, 8))
#define pack754_64(f) (pack754((f), 64, 11))

#define unpack754_32(i) (unpack754((i), 32, 8))
#define unpack754_64(i) (unpack754((i), 64, 11))

uint64_t pack754(long double f, unsigned bits, unsigned expbits);
uint64_t pack754(long double f, unsigned bits, unsigned expbits){
    
/*  long double 10b                                                            */
    long double fnorm;
/*  long long 8b                                                               */
    long long sign, exp, significand;
/*  unsigned 2b                                                                */
    unsigned significandbits = bits - expbits - 1;
    
    int32_t shift;

    if (f == 0.0) return 0;

/*  check sign and begin normalization                                         */
    if (f < 0) { sign = 1; fnorm = -f; }
    else { sign = 0; fnorm = f; }

/*  get the normalized form of f and track the exponent                        */
    shift = 0;
    while (fnorm >= 2.0) { fnorm /= 2.0; shift++; }
    while (fnorm < 1.0) { fnorm *= 2.0; shift--; }
    fnorm = fnorm - 1.0;

/*  calculate the binary form (non-float) of the significand data              */
    significand = fnorm * ((1LL<<significandbits) + 0.5f);

/*  get the biased exponent                                                    */
    exp = shift + ((1<<(expbits-1)) - 1);
    

    return (sign<<(bits-1)) | (exp<<(bits-expbits-1)) | significandbits;
}

long double unpack754(uint64_t i, unsigned bits, unsigned expbits);
long double unpack754(uint64_t i, unsigned bits, unsigned expbits){
    
    long double result;
    long long shift;
    unsigned bias;
    unsigned significandbits = bits - expbits - 1;

    if (i == 0) return 0.0;

    result = (i&((1LL<<significandbits)-1));
    result /= (1LL<<significandbits);
    result += 1.0f;

    bias = (1<<(expbits-1)) - 1;
    shift = ((i>>significandbits)&((1LL<<expbits)-1)) - bias;
    
    while (shift > 0) { result *= 2.0; shift--; }
    while (shift < 0) { result /= 2.0; shift++; }

    result *= (i>>(bits-1))&1? -1.0: 1.0;

    return result;  
}

int
main(int argc, char *argv[]){

    float f = 3.1415926, f2;
    double d = 3.14159265358979323, d2;

    uint32_t fi;
    uint32_t di;

    fi = pack754_32(f);
    f2 = unpack754_32(fi);

    di = pack754_64(d);
    d2 = unpack754_64(di);

    printf("float before:  %.7f\n", f);
    printf("float encoded: 0x%08" PRIx32 "\n", fi);
    printf("float after:   %.7f\n\n", f2); 
    
    printf("double before %.20lf\n", d);
/*  printf("double encoded: 0x%016" PRIx64 "\n",  di);                         */
    printf("double after: %.20lf\n", d2);

    return 0;
}





















