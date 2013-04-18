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

#ifndef BPDECODE_H
#define BPDECODE_H

#include "bptypes.h"

// Input is the number of substitutions to perform, compressed and decompressed
// sizes, and a buffer with sufficient size to hold the decompressed data. The
// compressed data must be at the front and the substitution records at the
// back, separated by empty space:
//  CCCCCCCCCCxxxxxxxSSSSSS
// 
// Substitution records are of the form key:byte0:byte1. The substitution
// records are in reverse order: the first one in the buffer is for the last
// substitution performed during compression.
//
// Output is the same buffer with the decompressed data:
//  DDDDDDDDDDDDDDDDDDDDDDD
//
// csize: input, compressed size
// dsize: input, decompressed size (and offset of substitution records)
//
// NOTE: no validation is done of the input. A corrupted or maliciously crafted
// input could expand beyond the boundaries of the buffer area.

void BP_Decode(uint8_t * bfr, bp_size_t csize, bp_size_t dsize, uint8_t numSubs);

//******************************************************************************
#endif // BPDECODE_H
