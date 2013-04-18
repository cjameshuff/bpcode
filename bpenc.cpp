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

// Encode a file using byte-pair encoding.
//
// clang++ --std=c++11 -O3 bpenc.cpp -o bpenc
// 
// Rough overview of algorithm:
// Input is divided into blocks of the largest valid size that leaves NUMPASSES
// byte values unused. In each block, the most frequent pairs of bytes are
// replaced with bytes that do not occur in that block.
// 
// *****************************************************************************
// Type 1: independent blocks
// Each block has a list of the substitution pairs and values. If the input is
// of a form that produces large blocks, the space overhead is minimal, and each
// block can be decoded without having to refer to a separate pair table.
// If the input produces small blocks, overhead can be severe. In the worst case
// of a cycling 0x00-0xFF pattern, with 32 substitutions, compressed block size
// will be 192 B, while the header will be 3*32 = 99 B.
// Also, if the data is meant to be streamed, there is a higher latency from
// starting to read a block and producing decoded output.
// 
// Type 2: shared pair table
// Avoids the overhead of a pair table for each block by computing a pair table
// for the full input and reusing it for each block.
// Tradeoff is somewhat poorer compression. Compression will be worse if pair
// frequencies vary widely between different parts of the input.
// Optimization: if a pair doesn't exist in a given block, use an "ignored" key
// to avoid wasting a key?
// 
// Both types are supported by the same file format, with 2 bytes of overhead
// per block for type 1 (for the 0x0000 block size).
// -----------------------------------------------------------------------------
// Block size of 0 indicates a pair frequency table:
// Prefix is table of NUMPASSES most common pairs, sorted in order of decreasing
// frequency:
// (BLOCK_SIZE:2 == 0x0000) (NUM_SUBS:1 = NUMPASSES) (PAIRS:NUM_SUBS*8*2)
// 
// NUM_SUBS: number of substitutions
// 
// Compressed data blocks are of the form:
// (BLOCK_SIZE:2 != 0x0000) (KEYS:2*NUM_SUBS) (DATA:n)
// 
// COMP_SIZE: the compressed data size (not including substitution header)
// 
// KEYS: the substitution keys. 1 byte each.
// 
// DATA: the compressed data
// *****************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include <string>
#include <vector>
#include <algorithm>

#include <sys/time.h>

// #define NUMPASSES  (128)
// #define NUMPASSES  (64)
#define NUMPASSES  (32)
// #define NUMPASSES  (16)
// #define NUMPASSES  (8)

static inline int imin(int x, int y) {return (x < y)? x : y;}
static inline int imax(int x, int y) {return (x > y)? x : y;}

static inline double GetRealSeconds() {
    struct timeval newTime;
    gettimeofday(&newTime, NULL);
    return newTime.tv_sec + newTime.tv_usec/1e6;
}

struct Stats {
    size_t inputSize;
    size_t outputSize;
    size_t numBlocks;
    double avgSubs;
};


struct PairCount {
    size_t count;
    uint8_t first;
    uint8_t second;
};


struct Block {
    std::vector<uint8_t> data;// 
    std::vector<uint8_t> unused;
    std::vector<uint8_t> subs;
    Block(const uint8_t *& _data, const uint8_t * dataEnd);
    
    
    void CollectUnused();
    void DoSubs(int sub, uint8_t first, uint8_t second);
};

Block::Block(const uint8_t *& _data, const uint8_t * dataEnd)
{
    // Variable-size blocks
    // Grow block until we run out of data, reach the maximum allowable block size, or
    // number of unused byte values drops to NUMPASSES.
    bool usedTbl[256];
    for(int j = 0; j < 256; ++j)
        usedTbl[j] = false;
    
    int usedCount = 0;
    int rawSize = 0;
    const uint8_t * b = _data;
    while((b != dataEnd) && (rawSize < 65535) && (256 - usedCount != NUMPASSES))
    {
        if(!usedTbl[*b]) {
            usedTbl[*b] = true;
            ++usedCount;
        }
        ++b;
        ++rawSize;
    }
    printf("block size: %d\n", rawSize);
    if(rawSize == 0)
        exit(-1);
    data.assign(_data, _data + rawSize);
    _data += rawSize;
    
    for(int j = 0; j < 256; ++j)
        if(!usedTbl[j])
            unused.push_back(j);
    // printf("unused words: %lu\n", blocks.back()->unused.size());
}

void Block::CollectUnused()
{
    bool usedTbl[256];
    for(int j = 0; j < 256; ++j)
        usedTbl[j] = false;
    
    for(auto b : data)
        usedTbl[b] = true;
    
    unused.clear();
    for(int j = 0; j < 256; ++j)
        if(!usedTbl[j])
            unused.push_back(j);
}

void Block::DoSubs(int sub, uint8_t first, uint8_t second)
{
    if(!unused.empty())
    {
        subs.push_back(unused.back());
        unused.pop_back();
        
        uint8_t * dataInEnd = &data[0] + data.size();
        uint8_t * dataIn = &data[0];
        uint8_t * dataOut = &data[0];
        
        while(dataIn != dataInEnd)
        {
            if(dataIn + 1 != dataInEnd && *dataIn == first && *(dataIn + 1) == second)
            {
                *dataOut++ = subs[sub];
                dataIn += 2;
            }
            else {
                *dataOut++ = *dataIn++;
            }
        }
        // printf("%d %d -> %d\n", first, second, subs[sub]);
        // printf("compressed block from: %lu to %lu\n", data.size(), dataOut - &data[0]);
        data.resize(dataOut - &data[0]);
        
        // Substitutions may have freed up some more substitution values, do another search when we run out
        // Not necessary with current setup, blocks are guaranteed to have available byte values.
        // if(unused.empty())
        //     CollectUnused();
    }
}


void GetBestPair(std::vector<Block *> & blocks, PairCount & bestPair)
{
    std::vector<size_t> pairCounts(65536, 0);
    // table index is concatenation of bytes, first byte being the high byte
    
    for(auto & blk : blocks)
    {
        if(!blk->unused.empty()) {
            uint8_t * data = &(blk->data[0]);
            for(int j = 0; j < blk->data.size() - 1; ++j) {
                int first = *data, second = *(data + 1);
                ++(pairCounts[(first << 8) | second]);
                ++data;
            }
        }
    }
    
    int bestPairIdx = 0;
    size_t bestPairCount = pairCounts[0];
    for(int j = 1; j < 65536; ++j)
    {
        if(pairCounts[j] > bestPairCount) {
            bestPairIdx = j;
            bestPairCount = pairCounts[j];
        }
    }
    
    // for(int j = 0; j < 65536; ++j)
    //         if(pairCounts[j])
    //             printf("count: %d, %d: %lu\n", (int)j >> 8, (int)j & 0xFF, pairCounts[j]);
    
    
    bestPair.first = bestPairIdx >> 8;
    bestPair.second = bestPairIdx & 0xFF;
    
    // printf("best count: %d, %d: %lu\n", (int)bestPairIdx >> 8, (int)bestPairIdx & 0xFF, pairCounts[bestPairIdx]);
}

void GetBestPair(const Block * block, PairCount & bestPair)
{
    std::vector<size_t> pairCounts(65536, 0);
    // table index is concatenation of bytes, first byte being the high byte
    
    const uint8_t * data = &(block->data[0]);
    for(int j = 0; j < block->data.size() - 1; ++j) {
        int first = *data, second = *(data + 1);
        ++(pairCounts[(first << 8) | second]);
        ++data;
    }
    
    int bestPairIdx = 0;
    size_t bestPairCount = pairCounts[0];
    for(int j = 1; j < 65536; ++j)
    {
        if(pairCounts[j] > bestPairCount) {
            bestPairIdx = j;
            bestPairCount = pairCounts[j];
        }
    }
    
    bestPair.first = bestPairIdx >> 8;
    bestPair.second = bestPairIdx & 0xFF;
    
    // printf("best count: %d, %d: %lu\n", (int)bestPairIdx >> 8, (int)bestPairIdx & 0xFF, pairCounts[bestPairIdx]);
}



void BP_Encode1(FILE * fout, std::vector<Block *> & blocks, Stats & stats);
void BP_Encode2(FILE * fout, std::vector<Block *> & blocks, Stats & stats);

void BP_Encode1(FILE * fout, std::vector<Block *> & blocks, Stats & stats)
{
    stats.avgSubs = 0;
    uint8_t writeBuf[4];
    
    for(auto & blk : blocks)
    {
        std::vector<PairCount> pairs;
        for(int sub = 0; sub < NUMPASSES; ++sub)
        {
            PairCount bestPair;
            GetBestPair(blk, bestPair);
            pairs.push_back(bestPair);
            blk->DoSubs(sub, bestPair.first, bestPair.second);
        }
        
        // Write pair table
        writeBuf[0] = 0x00;
        writeBuf[1] = 0x00;
        fwrite(writeBuf, sizeof(uint8_t), 2, fout);
        
        writeBuf[0] = NUMPASSES;
        fwrite(writeBuf, sizeof(uint8_t), 1, fout);
        
        for(int sub = 0; sub < NUMPASSES; ++sub)
        {
            // printf("pair: %d, %d\n", (int)pairs[sub].first, (int)pairs[sub].second);
            fwrite(&(pairs[sub].first), sizeof(uint8_t), 1, fout);
            fwrite(&(pairs[sub].second), sizeof(uint8_t), 1, fout);
        }
        
        
        // Write block
        int blockSize = blk->data.size();
        int numSubs = blk->subs.size();
        if(numSubs != NUMPASSES)
        {
            printf("Block had %d substitutions, %d expected\n", numSubs, NUMPASSES);
            exit(EXIT_FAILURE);
        }
            
        // printf("Block size: %d, num subs: %d\n", blockSize, numSubs);
        
        writeBuf[0] = (blockSize >> 8) & 0xFF;
        writeBuf[1] = blockSize & 0xFF;
        fwrite(writeBuf, sizeof(uint8_t), 2, fout);
        
        for(int sub = 0; sub < numSubs; ++sub)
            fwrite(&(blk->subs[sub]), sizeof(uint8_t), 1, fout);
        
        fwrite(&(blk->data[0]), sizeof(uint8_t), blockSize, fout);
        
        stats.avgSubs += numSubs;
    }
    
    stats.avgSubs /= stats.numBlocks;
}

void BP_Encode2(FILE * fout, std::vector<Block *> & blocks, Stats & stats)
{
    stats.avgSubs = 0;
    
    std::vector<PairCount> pairs;
    for(int sub = 0; sub < NUMPASSES; ++sub)
    {
        // find best pair across all blocks
        PairCount bestPair;
        GetBestPair(blocks, bestPair);
        pairs.push_back(bestPair);
        
        // Do substitution
        for(auto & blk : blocks)
            blk->DoSubs(sub, bestPair.first, bestPair.second);
    }
    
    uint8_t writeBuf[4];
    
    // write pair table
    writeBuf[0] = 0x00;
    writeBuf[1] = 0x00;
    fwrite(writeBuf, sizeof(uint8_t), 2, fout);
    
    writeBuf[0] = NUMPASSES;
    fwrite(writeBuf, sizeof(uint8_t), 1, fout);
    
    for(int sub = 0; sub < NUMPASSES; ++sub)
    {
        // printf("pair: %d, %d\n", (int)pairs[sub].first, (int)pairs[sub].second);
        fwrite(&(pairs[sub].first), sizeof(uint8_t), 1, fout);
        fwrite(&(pairs[sub].second), sizeof(uint8_t), 1, fout);
    }
    
    // Write blocks
    for(auto & blk : blocks)
    {
        int blockSize = blk->data.size();
        int numSubs = blk->subs.size();
        if(numSubs != NUMPASSES)
        {
            printf("Block had %d substitutions, %d expected\n", numSubs, NUMPASSES);
            exit(EXIT_FAILURE);
        }
            
        // printf("Block size: %d, num subs: %d\n", blockSize, numSubs);
        
        writeBuf[0] = (blockSize >> 8) & 0xFF;
        writeBuf[1] = blockSize & 0xFF;
        fwrite(writeBuf, sizeof(uint8_t), 2, fout);
        
        for(int sub = 0; sub < numSubs; ++sub)
            fwrite(&(blk->subs[sub]), sizeof(uint8_t), 1, fout);
        
        fwrite(&(blk->data[0]), sizeof(uint8_t), blockSize, fout);
        
        stats.avgSubs += numSubs;
    }
    
    stats.avgSubs /= stats.numBlocks;
}
// *****************************************************************************

int main(int argc, char * argv[])
{
    if(argc < 2) {
        printf("Usage: bpenc INFILE [OUTFILE]\n");
        exit(EXIT_FAILURE);
    }
    
    const char * finname = argv[1];
    const char * foutname = "<stdout>";
    
    FILE * fin = stdin, * fout = stdout;
    
    // Just read in whole file at once. This isn't going to be used for anything big.
    fin = fopen(finname, "rb");
    fseek(fin, 0L, SEEK_END);
    size_t fileSize = ftell(fin);
    fseek(fin, 0L, SEEK_SET);
    
    uint8_t * fileData = new uint8_t[fileSize];
    fread(fileData, 1, fileSize, fin);
    
    if(argc == 3) {
        foutname = argv[2];
        fout = fopen(foutname, "wb");
    }
    
    
    double startT = GetRealSeconds(), endT;
    
    Stats stats;
    std::vector<Block *> blocks;
    
    {
        const uint8_t * data = fileData;
        const uint8_t * dataEnd = fileData + fileSize;
        while(data < dataEnd)
            blocks.push_back(new Block(data, dataEnd));
    }
    
    stats.inputSize = fileSize;
    stats.numBlocks = blocks.size();
    
    BP_Encode1(fout, blocks, stats);
    // BP_Encode2(fout, blocks, stats);
    
    stats.outputSize = ftell(fout);
    
    endT = GetRealSeconds();
    
    printf("Uncompressed size: %d, number of blocks: %d\n", (int)stats.inputSize, (int)stats.numBlocks);
    printf("Compressed size: %d, ratio %0.2f %%\n", (int)stats.outputSize, (float)stats.outputSize*100.0/stats.inputSize);
    printf("Average subs/block: %f\n", stats.avgSubs);
    printf("Compression Time: %f s\n", endT - startT);
    delete fileData;
    
    if(fin != stdout)
        fclose(fin);
    if(fout != stdout)
        fclose(fout);
    
    return EXIT_SUCCESS;
}


