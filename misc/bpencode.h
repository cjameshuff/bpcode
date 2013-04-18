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

#ifndef BPENCODE_H
#define BPENCODE_H

#include "bptypes.h"

// Byte pair encoding:
// Find unused bytes in a given input block
// Find common byte pairs in said input block. Replace each byte pair with one
// of the unused bytes.
// Recording a substitution takes 3 bytes, so it is only worthwhile to replace
// byte pairs with 4 or more instances. Thus, making a subsitution frees up
// enough room in the input buffer to store the substitution. The substitutions
// are therefore stored at the end of the input buffer, with only their count
// needing to be stored outside it. The output buffer is the input buffer with
// the input compressed and the substitution list hanging from the end, it is
// left to the user to concatenate it to the data if this is desired.

// TODO: investigate use of escape sequences to allow substitutions. Escaping
// a rarely used byte and using it to perform a substitution on a more frequent
// byte pair would allow better compression of large blocks of binary data.

// Searching for the most frequent byte pair takes considerable time.
// The optional cache reduces the number of times such searches are done.
// An excessively large cache consumes memory, one that's too small consumes
// RAM.
// The cache approach is only useful for extremely memory-constrained systems.
// For desktop and large embedded systems, more extensive data structures can
// be used to reduce compression time.

// Global variable bp_maxPasses is used if this is not defined
//#define BP_MAX_PASSES  (16)

// Byte pair cache takes n*2*sizeof(bp_size_t)
// Larger caches allow much faster compression but may lead to useful byte pairs
// being overlooked (meaning less compression), and increase consumption of RAM.
// The cache may be disabled completely by setting the size to 0.
#define BP_PAIR_CACHE_SIZE  (0)
//#define BP_PAIR_CACHE_SIZE  (16)
//#define BP_PAIR_CACHE_SIZE  (32)
//#define BP_PAIR_CACHE_SIZE  (64)
//#define BP_PAIR_CACHE_SIZE  (128)
//#define BP_PAIR_CACHE_SIZE  (256)


extern bp_size_t bp_csize; // compressed size
extern bp_size_t bp_dsize; // decompressed size (and end of substitution records)
extern uint8_t * bp_bfr; // Data buffer
extern uint8_t bp_numSubs; // Number of substitutions

#ifndef BP_MAX_PASSES
extern uint8_t bp_maxPasses;
#endif // BP_MAX_PASSES

void BP_Encode();

//******************************************************************************
#endif // BPENCODE_H
