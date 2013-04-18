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

#include "bpdecode.h"

void BP_Decode(uint8_t * bfr, bp_size_t csize, bp_size_t dsize, uint8_t numSubs)
{
    uint8_t * subs = bfr + dsize - numSubs*3;
    // Perform substitutions in reverse order
    // Substitution is done alternating back to front and front to back, overwriting the substitution being
    // performed in the process. The last substitution done fills the data
    // buffer and overwrites the last of the substitution records.
    uint8_t * src, * dst, * stop;
    
    uint8_t s = 0;
	while(s < numSubs)
    {
//        printf("csize: %d, dsize: %d, numsubs: %d\n", (int)csize, (int)dsize, (int)numSubs);
        // Data is at front of buffer.
		// Substitute backward, moving data from front of buffer to back
        // Source is front of buffer to start + csize
        // Destination is memory immediately prior to the substitution records
        // Must remove two records up front to guarantee room for this and the
        // following substitution sweep.
		uint8_t keyA = *(subs++);
		uint8_t pair0A = *(subs++);
		uint8_t pair1A = *(subs++);
		uint8_t keyB = *(subs++);
		uint8_t pair0B = *(subs++);
		uint8_t pair1B = *(subs++);
        
        src = bfr + csize - 1;// last byte of input data
        dst = subs - 1;// last empty byte before subst records
        stop = bfr - 1;// byte before start of input data
        
//        printf("A: Replacing %02X with %c%c (%02X %02X)\n", keyA, pair0A, pair1A, pair0A, pair1A);
        while(src != stop) {
            if(*src == keyA) {
                *(dst--) = pair1A;
                *(dst--) = pair0A;
            }
            else *(dst--) = *src;
            --src;
        }
        
        if(++s == numSubs)
            break;
        
        // Compute new size of input
        csize = subs - dst - 1;
        
        // Data is at back of buffer, flush against subst records.
		// Substitute forward, moving data from back of buffer to front
        src = dst + 1;
        dst = bfr;
        stop = subs;
        
//        printf("B: Replacing %02X with %c%c (%02X %02X)\n", keyB, pair0B, pair1B, pair0B, pair1B);
        while(src != stop) {
            if(*src == keyB) {
                *(dst++) = pair0B;
                *(dst++) = pair1B;
            }
            else *(dst++) = *src;
            ++src;
        }
        
        csize = dst - bfr;
        ++s;
	}
}

//******************************************************************************
