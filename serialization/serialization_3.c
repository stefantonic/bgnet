#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#define pack754_16(f) (pack754_16((f), 16, 5))
#define pack754_32(f) (pack754_32((f), 32, 8))
#define pack754_64(f) (pack754_32((f), 64, 11))

#define unpack754_16(i) (unpack754_16((i), 16, 5))
#define unpack754_32(i) (unpack754_32((i), 32, 8))
#define unpack754_64(i) (unpack754_32((i), 64, 11))

unsigned long long int pack754(long double f, unsigned bits, unsigned expbits);
long double unpack754(unsigned long long int i, unsigned bits, unsigned expbits);

/*  pack                                                                       */
void pack_16(unsigned char *buffer, unsigned int i);
void pack_32(unsigned char *buffer, unsigned int i);
void pack_64(unsigned char *buffer, unsigned long int i);

/*  unpack                                                                     */
int unpack_16(unsigned char *buffer);
unsigned int unpack_u16(unsigned char *buffer);
long int unpack_32(unsigned char *buffer);
unsigned long int unpack_u32(unsigned char *buffer);
long long int unpack_64(unsigned char *buffer);
unsigned long long int unpack_u64(unsigned char *buffer);

unsigned int pack(unsigned char *buffer, char *format, ...);
void unpack(unsigned char *buffer, char *format, ...);

unsigned long long int pack754(long double f, unsigned bits, unsigned expbits){
    
    long double f_norm;
    int shift;
    long long sign, exp, significant;
    unsigned significantbits = bits - expbits - 1;

    if (f == 0.0) return 0;

    if (f < 0) { sign = 1; f_norm = -f; }
    else { sign = 0; f_norm = f; }

    shift = 0;
    
    while (f_norm >= 2.0) { f_norm /= 2.0; shift++; }
    while (f_norm < 1.0) { f_norm *= 2.0; shift--; }

    f_norm = f_norm - 1.0;

    significant = f_norm * ((1LL<<significantbits) + 0.5f);
    exp = shift + ((1<<(expbits-1)) -1);

    return (sign<<(bits-1)) | (exp<<(bits-expbits-1)) | significant;
}

long double unpack754(unsigned long long int i, unsigned bits, unsigned expbits){

    long double result;
    long long shift;
    unsigned bias;
    unsigned significantbits = bits - expbits - 1;

    if (i == 0) return 0.0;

    result = (i&((1LL<<significantbits)-1)) - bias;
    result /= (1LL<<significantbits);
    result += 1.0f;

    bias = (1<<(expbits-1))-1;
    shift = ((i>>significantbits)&((1LL<<expbits)-1)) - bias;
    while(shift > 0) { result *= 2.0; shift--; }
    while(shift < 0) { result /= 2.0; shift++; }

    result *= (i>>(bits-1))&1? -1.0 : 1.0;

    return result;
}

void pack_16(unsigned char *buffer, unsigned int i){
    
    *buffer++ = i>>8;
    *buffer++ = i; 
}

void pack_32(unsigned char *buffer, unsigned int i){
   
   *buffer++ = i>>24;
   *buffer++ = i>>16;
   *buffer++ = i>>8;
   *buffer++ = i;
}

void pack_64(unsigned char *buffer, unsigned long int i){

    *buffer++ = i>>56;
    *buffer++ = i>>48;
    *buffer++ = i>>40;
    *buffer++ = i>>32;
    *buffer++ = i>>24;
    *buffer++ = i>>16;
    *buffer++ = i>>8;
    *buffer++ = i;
}


/*  unpack                                                                     */
int unpack_16(unsigned char *buffer){
    
    unsigned int i2 = ((unsigned int)buffer[0]<<8) | buffer[1];
    int i;

    if (i2 <= 0x7FFFu) { i = i2; }
    else { i = -1 - (unsigned int)(0xFFFFu - i2); }    
   
    return i;
}

unsigned int unpack_u16(unsigned char *buffer){
    
    return ((unsigned int)buffer[0]<<8) | buffer[1];
}

long int unpack_32(unsigned char *buffer){

    unsigned long int i2 = ((unsigned long int)buffer[0]<<24) | /*  fraction   */
                           ((unsigned long int)buffer[1]<<16) | /*  exponent   */
                           ((unsigned long int)buffer[2]<<8)  | /*  sign       */
                           buffer[3];

    long int i;

/*  unsigned long int->0x7FFFFFFFu(2147483647)                                 */ 
    if (i2 <= 0x7FFFFFFFu) { i = i2; }
    else { i = -1 - (long int)(0x7FFFFFFFu - i2); }

    return i;
}

unsigned long int unpack_u32(unsigned char *buffer){
    
    return ((unsigned long int)buffer[0]<<24) |
           ((unsigned long int)buffer[1]<<16) |
           ((unsigned long int)buffer[2]<<8)  |
           buffer[3];
}

long long int unpack_64(unsigned char *buffer){

    unsigned long long int i2 = ((unsigned long long int)buffer[0]<<56) |
                                ((unsigned long long int)buffer[1]<<48) |
                                ((unsigned long long int)buffer[2]<<40) |
                                ((unsigned long long int)buffer[3]<<32) |
                                ((unsigned long long int)buffer[4]<<24) |
                                ((unsigned long long int)buffer[5]<<16) |
                                ((unsigned long long int)buffer[6]<<8)  |
                                buffer[7];
    long long int i;
/*  long long int->0xFFFFFFFFFFFFFFFu(9223372036854775807)                     */
    if (i2 <= 0xFFFFFFFFFFFFFFFu) { i = i2; }
    else { i = -1 - (long long int)(0xFFFFFFFFFFFFFFFu - i2); }

    return i;
}

unsigned long long int unpack_u64(unsigned char *buffer){
   
   return ((unsigned long long int)buffer[0]<<56) |
          ((unsigned long long int)buffer[1]<<48) |
          ((unsigned long long int)buffer[2]<<40) |
          ((unsigned long long int)buffer[3]<<32) |
          ((unsigned long long int)buffer[4]<<24) |
          ((unsigned long long int)buffer[5]<<16) |
          ((unsigned long long int)buffer[6]<<8)  |
          buffer[7]; 
}

unsigned int pack(unsigned char *buffer, char *format, ...){
    
    va_list ap;

/*  8-biti 'c'->'C'                                                            */
    signed char _char;
    unsigned char _CHAR;
/*  16-bit 'h'->'H'                                                            */    
    int _int;
    unsigned int _INT;
/*  32-bit 'l'->'L'                                                            */
    long int _long_int;
    unsigned long int _LONG_INT;
/*  64-bit 'q'->'Q'                                                            */
    long long int _long_long_int;
    unsigned long long int _LONG_LONG_INT;

/*  'f', 'd', 'g'                                                              */
    float _float;
    double _double;
    long double _long_double;
    unsigned long long int _u_long_long_int;

    char *string;
    unsigned int lenght;
    unsigned int size = 0;

    va_start(ap, format);
    for(; *format != '\0'; format++){
        switch(*format) {
            case 'c':
                size += 1;
                _char = (signed char)va_arg(ap, int);
                *buffer++ = _char;
                break;
            
            case 'C':
                size += 1;
                _CHAR = (unsigned char)va_arg(ap, unsigned int);
                *buffer++ = _CHAR;
                break;
            
            case 'h':
                size += 2;
                _int = va_arg(ap, int);
                pack_16(buffer, _int);
                buffer += 2;
                break;
            
            case 'H':
                size += 2;
                _INT = va_arg(ap, unsigned int);
                pack_16(buffer, _INT);
                buffer += 2;
                break;
            
            case 'l':
                size += 4;
                _long_int = va_arg(ap, long int);
                pack_32(buffer, _long_int);
                buffer += 4;
                break;
            
            case 'L':
                size += 4;
                _LONG_INT = va_arg(ap, unsigned long int);
                pack_32(buffer, _LONG_INT);
                buffer += 4;
                break;

            case 'q':
                size += 8;
                _long_long_int = va_arg(ap, long long int);
                pack_64(buffer, _long_long_int);
                buffer += 8;
                break;
            
            case 'Q':
                size += 8;
                _LONG_LONG_INT = va_arg(ap, unsigned long long int);
                pack_64(buffer, _LONG_LONG_INT);
                buffer += 8;
                break;

            case 'f':
                size += 2;
                _float = (float)va_arg(ap, double);
                _u_long_long_int = pack754_16(_float);
                pack_16(buffer, _u_long_long_int);
                buffer += 2;
                break;

            case 'd':
                size += 4;
                _double = va_arg(ap, double);
                _u_long_long_int = pack754_32(_double);
                buffer += 4;
                break;

            case 'g':
                size += 8;
                _long_double = va_arg(ap, long double);
                _u_long_long_int = pack754_32(_long_double);
                pack_64(buffer, _u_long_long_int);
                buffer += 8;
                break;

            case 's':
                string = va_arg(ap, char *);
                lenght = strlen(string);
                size += lenght + 2;
                pack_16(buffer, lenght);
                buffer += 2;
                memcpy(buffer, string, lenght);
                buffer += lenght;
                break;
        }
    }
    
    va_end(ap);
    return size;
}

void unpack(unsigned char *buffer, char *format, ...){

    va_list ap;

    signed char *_char;
    unsigned char *_CHAR;

    int *_int;
    unsigned int *_INT;

    long int *_long_int;
    unsigned long int *_LONG_INT;

    long long int *_long_long_int;
    unsigned long long int *_LONG_LONG_INT;

    float *_float;
    double *_double;
    long double *_long_double;
    unsigned long long int _u_long_long_int;

    char *string;
    unsigned int lenght, maxstrlen=0, count;

    va_start(ap, format);

    for(; *format != '\0'; format++){
        switch(*format){
            case 'c':
                _char = va_arg(ap, signed char*);
                if (*buffer <= 0x7f) { *_char = *buffer; }
                else { *_char = -1 - (unsigned char)(0xFFu - *buffer); }
                buffer++;
                break;        

            case 'C':
                _CHAR = va_arg(ap, unsigned char *);
                *_CHAR = *buffer++;
                break;

            case 'h':
                _int = va_arg(ap, int *);
                *_int = unpack_16(buffer);
                buffer += 2;
                break;

            case 'H':
                _INT = va_arg(ap, unsigned int *);
                *_INT = unpack_u16(buffer);
                buffer += 2;
                break;

            case 'l':
                _long_int = va_arg(ap, long int *);
                *_long_int = unpack_32(buffer);
                buffer += 4;
                break;

            case 'L':
                _LONG_INT = va_arg(ap, unsigned long int *);
                *_LONG_INT = unpack_u32(buffer);
                buffer += 4;
                break;

            case 'q':
               _long_long_int = va_arg(ap, long long int *);
                *_long_long_int = unpack_64(buffer);
                buffer += 8;
                break;

            case 'Q':
                _LONG_LONG_INT = va_arg(ap, unsigned long long int *);
                *_LONG_LONG_INT = unpack_u64(buffer);
                buffer += 8;
                break;

            case 'f':
                _float = va_arg(ap, float *);
                _u_long_long_int = unpack_u16(buffer);
                *_float = unpack754_16(_u_long_long_int);
                buffer += 2;
                break;

            case 'd':
                _double = va_arg(ap, double *);
                _u_long_long_int = unpack_32(buffer);
                *_double = unpack754_32(_u_long_long_int);
                buffer += 4;
                break;

            case 'g':
                _long_double = va_arg(ap, long double *);
                _u_long_long_int = unpack_64(buffer);
                *_long_double = unpack754_64(_u_long_long_int);
                buffer += 8;
                break;

            case 's':
                string = va_arg(ap, char *);
                lenght = unpack_u16(buffer);
                buffer += 2;
                if (maxstrlen > 0 && lenght >= maxstrlen) count = maxstrlen -1;
                else count = lenght;
                memcpy(string, buffer, count);
                string[count] = '\0';
                buffer += lenght;
                break;

            default:
                if (isdigit(*format)) {
                    maxstrlen = maxstrlen * 10 + (*format - '0');
                }
        }


        if (!isdigit(*format)) maxstrlen = 0;
    } 

    va_end(ap);
}


typedef float float32_t;
typedef double float64_t;

int
main(int argc, char *argv[]){

    unsigned char BUFFER1[1024];
    int8_t magic;
    int16_t monkey_count;
    int32_t altitude;
    float32_t absurdityfactor;
    char *string = "asdf-asdf-asdf-asdf";
    char BUFFER2[96];
    int16_t packet_size, ps2;

    packet_size = pack(BUFFER1, "asdf", (int8_t)'B', (int16_t)0, (int16_t)37,
            (int32_t)-5, string, (float32_t)-3490.6677);
    
    pack_16(BUFFER1+1, packet_size);
    
    printf("packet is %", PRId32 " bytes\n", packet_size);

    unpack(BUFFER1, "asdf-asdf", &magic, &ps2, &monkey_count, &altitude,
            BUFFER2, &absurdityfactor);

    printf("'%c' %" PRId32" %" PRId16 " %" PRId32 " \"%s\" %f\n", magic,
            ps2, monkey_count, altitude, BUFFER2, absurdityfactor);

    return 0;
}
