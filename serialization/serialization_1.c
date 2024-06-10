#include <stdint.h>
#include <stdio.h>


uint32_t htonf(float f);
uint32_t htonf(float f){
   
   uint32_t ptr;
   uint32_t sign; 
   
   if (f < 0) { sign = 1; f = -f; } 
   else { sign = 0; }

/* 0x7FFF(32767) int16_t   
   0xFFFF(65535) uint16_t                                                      */
   ptr = ((((uint32_t)f)&0x7FFF)<<16) | (sign << 31);
   ptr |= (uint32_t)(((f - (int)f) * 65536.0f))&0xFFFF;
       
   return ptr; 
}

float ntohf(uint32_t ptr);
float ntohf(uint32_t ptr) {
    
    float f = ((ptr>>16)&0x7FFFF);
    f += (ptr&0xFFFF) / 65536.0f;

    if (((ptr>>31)&0x1) == 0x1) { f = -f; }
    
    return f;
}


int
main(int argc, char *argv[]){

    float f = 3.1415926, f2;
    uint32_t netf;

    netf = htonf(f);
    f2 = ntohf(netf);
    
    printf("original: %f\n", f);
    printf("network: 0x%08X\n", netf);
    printf("unpacked: %f\n", f2);

    return 0;
}
