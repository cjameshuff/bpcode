//******************************************************************************
//    Copyright (c) 2010, Christopher James Huff
//    All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//  * Redistributions of source code must retain the above copyright
//  notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//  notice, this list of conditions and the following disclaimer in the
//  documentation and/or other materials provided with the distribution.
//  * Neither the name of the copyright holders nor the names of contributors
//  may be used to endorse or promote products derived from this software
//  without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//******************************************************************************

#ifndef FLEXINTS_H
#define FLEXINTS_H

#include <stdint.h>
// Largest size representable with a 64 bit int will take 10 bytes,
// so it is safe to use a fixed-size buffer with that length.

// Decode variable-size integer, return number of bytes used
static inline size_t BP_DecodeFlexInt(size_t * val, uint8_t * bfr)
{
    size_t size = 1;
	*val = (*bfr & 0x7F);
	while(*bfr & 0x80) {
		++bfr;
		*val = (*val << 7) | (*bfr & 0x7F);
        ++size;
	}
    return size;
}

// Encode variable-size integer, return number of bytes used
static inline size_t BP_EncodeFlexInt(size_t val, uint8_t * bfr)
{
    size_t size = 1, c;
    while(val >> size*7)
        ++size;
    
    c = size - 1;
	*bfr = ((val >> c*7) & 0x7F);
	while(c) {
        *(bfr++) |= 0x80;
        --c;
        *bfr = ((val >> c*7) & 0x7F);
	}
//    size_t size = 1;
//	*bfr = (val & 0x7F);
//    val >>= 7;
//	while(val) {
//        *(bfr++) |= 0x80;
//        *bfr = (val & 0x7F);
//        val >>= 7;
//        ++size;
//	}
    return size;
}

/*void FlexIntTest(void)
{
    uint8_t bfr[10];
    size_t esize = 0, dsize = 0;
    
    for(int j = 0; j < 0x10000; ++j)
    {
        size_t val = j, dval;
        esize = BP_EncodeFlexInt(val, bfr);
        dsize = BP_DecodeFlexInt(&dval, bfr);
//        printf("%d\n", (int)val);
        if(esize != dsize)
            printf("Encoding size mismatch: %d, %d\n", (int)esize, (int)dsize);
        if(val != dval)
            printf("Encoding value mismatch: %d, %d\n", (int)val, (int)dval);
    }
}*/

#endif // FLEXINTS_H
