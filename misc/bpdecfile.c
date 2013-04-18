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

// Decode a file using byte-pair encoding, in the format output by bpencfile

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include <sys/time.h>

#include "bpdecode.h"
#include "flexints.h"

static inline int imin(int x, int y) {return (x < y)? x : y;}
static inline int imax(int x, int y) {return (x > y)? x : y;}

static inline double GetRealSeconds() {
    struct timeval newTime;
    gettimeofday(&newTime, NULL);
    return newTime.tv_sec + newTime.tv_usec/1e6;
}


int main(int argc, char * argv[])
{
    if(argc < 2)
        printf("Usage: bpdecfile INFILE [OUTFILE]\n");
    
    const char * finname = argv[1];
    const char * foutname = "<stdout>";
    
    uint8_t * fileData, * fileStart;
    FILE * fin = stdin, * fout = stdout;
    
    // Just read in whole file at once. This isn't going to be used for anything
    // big.
    fin = fopen(finname, "r");
    fseek(fin, 0L, SEEK_END);
    size_t fileSize = ftell(fin);
    fseek(fin, 0L, SEEK_SET);
    fileStart = fileData = (uint8_t *)malloc(fileSize);
    fread(fileData, 1, fileSize, fin);
    
    if(argc == 3) {
        foutname = argv[2];
        fout = fopen(foutname, "w");
    }
    
    uint8_t * block = (uint8_t *)malloc(1);
    size_t blockSize = 0, dsize, csize, numSubs;
    
    size_t blocks = 0;
    size_t totalCSize = 0, totalDSize = 0;
    double startT = GetRealSeconds(), endT;
    do {
        // FIXME: check that input data length is sufficient!
        fileData += BP_DecodeFlexInt(&dsize, fileData);
        
        if((fileData - fileStart) >= fileSize) {
            printf("Ran past end of file, bailing out\n");
            break;
        }
        
        fileData += BP_DecodeFlexInt(&csize, fileData);
        numSubs = *(fileData++);
        
        if((fileData - fileStart) >= fileSize) {
            printf("Ran past end of file, bailing out\n");
            break;
        }
        
        printf("Uncompressed size: %d, compressed: %d, numsubs: %d\n", (int)dsize, (int)csize, (int)numSubs);
        
        if(dsize > blockSize) {
            free(block);
            blockSize = dsize;
            block = (uint8_t *)malloc(blockSize);
        }
        
        // Copy compressed data
        memcpy(block, fileData, csize);
        fileData += csize;
        // Copy substitution records
        memcpy(block + dsize - numSubs*3, fileData, numSubs*3);
        fileData += numSubs*3;
        
        BP_Decode(block, csize, dsize, numSubs);
        fwrite(block, sizeof(uint8_t), dsize, fout);// decompressed data
        
        ++blocks;
        totalCSize += csize;
        totalDSize += dsize;
    } while((fileData - fileStart) < fileSize);
    
    endT = GetRealSeconds();
    
    printf("Uncompressed size: %d, number of blocks: %d\n", (int)totalDSize, (int)blocks);
    printf("Compressed size: %d, ratio %0.2f\n", (int)totalCSize, (float)totalCSize/fileSize);
    printf("Decompression Time: %f s\n", endT - startT);
    free(block);
    free(fileStart);
    
    return EXIT_SUCCESS;
}

//******************************************************************************
