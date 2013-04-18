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

// Encode a file using byte-pair encoding.
//
// gcc --std=c99 bpencode.c bpdecode.c bpencfile.c -o bpencfile
// 
// Encoded format is a concatenated sequence of blocks of the form:
// DEC_SIZE:n COMP_SIZE:n NUM_SUBS:8 SUBS:n DATA:n
//
// COMP_SIZE: the compressed data size
//
// DEC_SIZE: the uncompressed size (for buffer allocation or early rejection of
// overly large messages)
//
// NUM_SUBS: an 8-bit unsigned integer indicating the number of substitutions
//
// DATA: the compressed data
//
// SUBS: the substitution records. 3 bytes each.
//
// Could shave off a byte by folding DEC_SIZE and NUM_SUBS into one byte.
// 5-bit NUM_SUBS: up to 32 subs
// 3-bit power-of-2 DEC_SIZE: 1-256, 2-512, 4-1024, etc.
//
// Could compile a table of frequent pairs for the full input, and only store the substitutions for each block.
//
// variable width parameters:
// 0x80 bit indicates another less-significant 7 bits follow.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include <sys/time.h>

#include "bpencode.h"
#include "flexints.h"

//#define BLOCKSIZE  (4096)
#define BLOCKSIZE  (1024)
//#define BLOCKSIZE  (240)
#define MAXPASSES  (128)
//#define MAXPASSES  (64)
//#define MAXPASSES  (16)

static inline int imin(int x, int y) {return (x < y)? x : y;}
static inline int imax(int x, int y) {return (x > y)? x : y;}

static inline double GetRealSeconds() {
    struct timeval newTime;
    gettimeofday(&newTime, NULL);
    return newTime.tv_sec + newTime.tv_usec/1e6;
}

int main(int argc, char * argv[])
{
    bp_maxPasses = MAXPASSES;
    if(argc < 2)
        printf("Usage: bpencfile INFILE [OUTFILE]\n");
    
    const char * finname = argv[1];
    const char * foutname = "<stdout>";
    
    uint8_t * fileData;
    FILE * fin = stdin, * fout = stdout;
    
    // Just read in whole file at once. This isn't going to be used for anything big.
    fin = fopen(finname, "r");
    fseek(fin, 0L, SEEK_END);
    size_t fileSize = ftell(fin);
    fseek(fin, 0L, SEEK_SET);
    fileData = (uint8_t *)malloc(fileSize);
    fread(fileData, 1, fileSize, fin);
    
    if(argc == 3) {
        foutname = argv[2];
        fout = fopen(foutname, "w");
    }
    
    
    bp_bfr = (uint8_t *)malloc(BLOCKSIZE);
    uint8_t sizeBfr[10];
    size_t sizeofSize;
    size_t blocks = 0;
    size_t totalCSize = 0;
    double startT = GetRealSeconds(), endT;
    do {
        bp_dsize = imin(BLOCKSIZE, fileSize - blocks*BLOCKSIZE);
        memcpy(bp_bfr, fileData + blocks*BLOCKSIZE, bp_dsize);
        BP_Encode();
        
        // DEC_SIZE:n COMP_SIZE:n NUM_SUBS:8 SUBS:n DATA:n
        sizeofSize = BP_EncodeFlexInt(bp_dsize, sizeBfr);// decompressed data size
        fwrite(&sizeBfr, sizeof(uint8_t), sizeofSize, fout);
        sizeofSize = BP_EncodeFlexInt(bp_csize, sizeBfr);// compressed size
        fwrite(&sizeBfr, sizeof(uint8_t), sizeofSize, fout);
        fwrite(&bp_numSubs, sizeof(uint8_t), 1, fout);// number of substitutions
        fwrite(bp_bfr + bp_dsize - bp_numSubs*3, sizeof(uint8_t), bp_numSubs*3, fout);// substitutions
        fwrite(bp_bfr, sizeof(uint8_t), bp_csize, fout);// compressed data
        
        ++blocks;
        totalCSize += bp_csize;
    } while(blocks*BLOCKSIZE < fileSize);
    
    endT = GetRealSeconds();
    
    printf("Uncompressed size: %d, number of blocks: %d\n", (int)fileSize, (int)blocks);
    printf("Compressed size: %d, ratio %0.2f bpc\n", (int)totalCSize, (float)totalCSize/fileSize*8.0);
    printf("Compression Time: %f s\n", endT - startT);
    free(fileData);
    
    return EXIT_SUCCESS;
}

//******************************************************************************
