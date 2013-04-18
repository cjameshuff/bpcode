//******************************************************************************
//    Copyright (c) 2013, Christopher James Huff
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

// Decode a file encoded using byte-pair encoding.
//
// clang++ --std=c++11 -O3 bpdec.cpp -o bpdec

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include <string>
#include <vector>
#include <algorithm>

#include <sys/time.h>

static inline int imin(int x, int y) {return (x < y)? x : y;}
static inline int imax(int x, int y) {return (x > y)? x : y;}

static inline double GetRealSeconds() {
    struct timeval newTime;
    gettimeofday(&newTime, NULL);
    return newTime.tv_sec + newTime.tv_usec/1e6;
}

void BP_Decode(FILE * fout, const uint8_t * data, size_t size);

int main(int argc, char * argv[])
{
    if(argc < 2) {
        printf("Usage: bpdec INFILE [OUTFILE]\n");
        exit(EXIT_FAILURE);
    }
    
    const char * finname = argv[1];
    const char * foutname = "<stdout>";
    
    FILE * fin = stdin, * fout = stdout;
    
    // Just read in whole file at once. This isn't going to be used for anything big.
    fin = fopen(finname, "rb");
    fseek(fin, 0L, SEEK_END);
    size_t inputFileSize = ftell(fin);
    fseek(fin, 0L, SEEK_SET);
    
    uint8_t * fileData = new uint8_t[inputFileSize];
    fread(fileData, 1, inputFileSize, fin);
    
    if(argc == 3) {
        foutname = argv[2];
        fout = fopen(foutname, "wb");
    }
    
    
    double startT = GetRealSeconds(), endT;
    
    BP_Decode(fout, fileData, inputFileSize);
    
    endT = GetRealSeconds();
    
    size_t outputFileSize = ftell(fout);
    printf("Input size: %lu s\n", inputFileSize);
    printf("Output size: %lu s\n", outputFileSize);
    printf("Decompression Time: %f s\n", endT - startT);
    delete fileData;
    
    if(fin != stdout)
        fclose(fin);
    if(fout != stdout)
        fclose(fout);
    
    return EXIT_SUCCESS;
}


void BP_Decode(FILE * fout, const uint8_t * data, size_t size)
{
    const uint8_t * dataEnd = data + size;
    
    if(*data != 0x00 || *(data + 1) != 0x00)
    {
        printf("Bad input, expected init block\n");
        exit(EXIT_FAILURE);
    }
    
    int numSubs;
    const uint8_t * pairs;
    size_t numBlocks = 0;
    size_t outputSize = 0;
    while(data < dataEnd)
    {
        int blockSize = (((int)(*data)) << 8) | *(data + 1);
        data += 2;
        
        if(blockSize == 0)
        {
            numSubs = *data;
            data += 1;
            
            pairs = data;
            data += 2*numSubs;
        }
        else
        {
            // printf("Block size: %d, num subs: %d\n", blockSize, numSubs);
            const uint8_t * subs = data;
            data += numSubs;
            
            std::vector<uint8_t> bufa, bufb;
            std::vector<uint8_t> * srcbuf = &bufa, * dstbuf = &bufb;
            srcbuf->assign(data, data + blockSize);
            data += blockSize;
            ++numBlocks;
            
            for(int sub = numSubs - 1; sub >= 0; --sub)
            {
                dstbuf->clear();
                for(auto b : *srcbuf)
                {
                    if(b == subs[sub]) {
                        dstbuf->push_back(pairs[sub*2]);
                        dstbuf->push_back(pairs[sub*2 + 1]);
                    }
                    else {
                        dstbuf->push_back(b);
                    }
                }
                // printf("%d -> %d %d\n", subs[sub], pairs[sub*2], pairs[sub*2 + 1]);
                std::swap(srcbuf, dstbuf);
            }
            // printf("Decompressed size: %lu\n", srcbuf->size());
            
            fwrite(&((*srcbuf)[0]), sizeof(uint8_t), srcbuf->size(), fout);
            outputSize += srcbuf->size();
        }
    }
    printf("Num blocks: %lu\n", numBlocks);
}
// *****************************************************************************
