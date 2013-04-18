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

#include "bpencode.h"

#include <string.h>


static uint8_t freeVals[32];

bp_size_t bp_csize; // compressed size
bp_size_t bp_dsize; // decompressed size (and end of substitution records)
uint8_t * bp_bfr; // Data buffer
uint8_t bp_numSubs; // Number of substitutions

#ifndef BP_MAX_PASSES
#define BP_MAX_PASSES  bp_maxPasses
uint8_t bp_maxPasses = 16;
#endif // BP_MAX_PASSES


#if(BP_PAIR_CACHE_SIZE > 1)
struct BP_CacheEntry {
    bp_size_t idx;
    bp_size_t count;
};

// Cache is sorted in increasing order of frequency.
// If using a pair does not invalidate others, the count is simply decremented.
static struct BP_CacheEntry bp_cache[BP_PAIR_CACHE_SIZE];
static bp_smallint_t bp_cached;
#endif // BP_PAIR_CACHE_SIZE


//******************************************************************************

// Return index of lowest set bit
// Assumes at least one bit is set. Will return as if bit 7 were set if given zero.
static inline uint8_t LowestBit(uint8_t b)
{
    if(b & 0x0F) {
        if(b & 0x03)
            return (b & 0x01)? 0 : 1;
        else
            return (b & 0x04)? 2 : 3;
    }
    else {
        if(b & 0x30)
            return (b & 0x10)? 4 : 5;
        else
            return (b & 0x40)? 6 : 7;
    }
} // LowestBit()

// There are 256 bits in the freeVals array, each corresponding to an 8-bit value.
// Set all, then clear the ones coresponding to each byte in bfr.
static void GetUsed(uint8_t * bfr, bp_size_t size)
{
    memset(freeVals, 0xFF, 32);
    for(bp_size_t j = 0; j < size; ++j, ++bfr)
        freeVals[*bfr & 0x1F] &= ~(1 << (*bfr >> 5));
} // GetUsed()


static bool PickUnused(uint8_t * val)
{
    for(bp_size_t j = 0; j < 32; ++j)
    {
        uint8_t bkt = freeVals[j];
        if(bkt) {
            uint8_t bit = LowestBit(bkt);
            freeVals[j] &= ~(1 << bit);
            *val = j | (bit << 5);
//            printf("Picked unused value: %02X\n", *val);
            return true;
        }
    }
//    printf("No available unused values!\n");
    return false;
} // PickUnused()


// Count instances of given pair in buffer.
// Side effects: none
static bp_size_t CountPairs(uint8_t * pair, uint8_t * data, bp_size_t size)
{
    const uint8_t pair0 = pair[0], pair1 = pair[1];
#if(BP_PAIR_CACHE_SIZE > 1)
    // If pair is already in cache, don't bother counting it
    const uint8_t * bfr = bp_bfr;
    for(bp_size_t j = 0; j < bp_cached; ++j)
        if(bfr[bp_cache[j].idx] == pair0 && bfr[bp_cache[j].idx + 1] == pair1)
            return 0;
#endif // BP_PAIR_CACHE_SIZE
    
//    printf("Counting occurrences of %c%c (%02X %02X)\n", pair[0], pair[1], pair[0], pair[1]);
//    printf("size: %d\n", (int)size);
    bp_size_t occurrences = 1;
    bp_size_t j = 2;
    while(j < size)
    {
        if(data[j] == pair0 && data[j + 1] == pair1) {
            ++occurrences;
            j += 2;
        }
        else {
            ++j;
        }
    }
//    if(occurrences > 2)
//        printf("Counted %d occurrences of %c%c (%02X %02X)\n", (int)occurrences, pair[0], pair[1], pair[0], pair[1]);
    return occurrences;
} // CountPairs()


#if(BP_PAIR_CACHE_SIZE > 1)
static void BuildCache(void)
{
    // Byte pair cache:
    // If the chosen pair has no bytes in common with another pair, the substitution will
    // not change that pair's match count, so that pair's match count may be reused.
    // Also, performing the substitution can only add as many matches as there are instances
    // to substitute, so if two byte pairs have identical match counts, it is faster to 
    // just substitute both without recounting.
    //
    // The start indices of the highest N match counts would also be of use in accelerating
    // later searches...weeding out a large number of searches that'll only result in low match
    // counts.
    // TODO: store index of first pair with >=4 matches
    bp_size_t size = bp_csize;
    uint8_t * bfr = bp_bfr;
    
    // Cache is is empty, refill
    bp_size_t j = 0, end = size - 8;
    
//    printf("BuildCache(): size = %d\n", (int)size);
    while((j < end) && (bp_cached < BP_PAIR_CACHE_SIZE))
    {
        uint8_t * pair = bfr + j;
        bp_size_t count = CountPairs(pair, pair + 2, size - 2 - j);
//        printf("count = %d\n", (int)count);
        // If pair count < 4, subsituting only costs CPU and space
        if(count < 4) {
            ++j;
            continue;
        }
        
        // Locate insertion point. ipt is index of worst entry with higher pair count,
        // or first empty slot
        bp_size_t ipt = 0;
        while(ipt < bp_cached && bp_cache[ipt].count < count)
            ++ipt;
//        printf("bp_cached = %d, ipt = %d\n", (int)bp_cached, (int)ipt);
        
        if(ipt < bp_cached) {
            // Move higher count entries up to make room
            for(bp_size_t c = bp_cached; c > ipt; --c)
                bp_cache[c] = bp_cache[c - 1];
        }
//        printf("Inserting\n");
        
        // insert into empty slot
        bp_cache[ipt].idx = j;
        bp_cache[ipt].count = count;
        ++bp_cached;
        ++j;
    }
    
    // Cache filled, find better pairs
//    printf("Cache filled\n");
    while(j < end)
    {
        uint8_t * pair = bfr + j;
        bp_size_t count = CountPairs(pair, pair + 2, size - 2 - j);
        if(count > 4 && bp_cache[0].count < count)
        {
            // Cache full, but this pair is better than worst pair in cache.
            // Insert, moving lower entries down and dropping lowest.
            // Lowest entry is going to be dropped no matter what, so start with 1.
            bp_size_t ipt = 1;
            while(ipt < BP_PAIR_CACHE_SIZE && bp_cache[ipt].count < count) {
                bp_cache[ipt - 1] = bp_cache[ipt];
                ++ipt;
            }
            --ipt;
        
            // insert into vacated slot
            bp_cache[ipt].idx = j;
            bp_cache[ipt].count = count;
        }
        ++j;
    }
//    printf("Cache built\n");
} // BuildCache()
#endif // BP_PAIR_CACHE_SIZE


static bool DoSubstitution()
{
    // Pick a pair
    // Scan forward, count n copies of said pair
    // if n > previously chosen pair, pick new pair
    // Repeat until pair with most occurrences is found
    // (optionally cache rejected pairs for later use?)
    
    bp_size_t size = bp_csize;
    uint8_t * bfr = bp_bfr;
//    printf("Doing substitution pass on block of size %d\n", (int)size);
    
    if(size <= 8) {
//        printf("Block too short!\n");
        return false;// Can't compress, block too small
    }
    
    uint8_t key;
    if(!PickUnused(&key)) {
//        printf("No substitution values possible!\n");
        return false;// Can't compress, no substitution values possible
    }
    
    // TODO:
    // This is very non-optimal in terms of CPU. A bitmap with a bit for each pair could be
    // used to quickly discard pairs that have already been encountered, instead of re-counting
    // ones that have already been fully counted. Unfortunately, such a bitmap would require 8192
    // bytes. Make optional, for PC/large embedded system builds.
    // A Bloom filter might be useful on small embedded systems. Accelerate compression, at a
    // cost in missing some pairs. A simple implementation would be to just check n bits of a pair.
#if(BP_PAIR_CACHE_SIZE > 1)
    if(bp_cached == 0) {
        BuildCache();
        if(bp_cached == 0)
            return false; // can't compress, no useful pairs to substitute
    }
    
    bp_size_t idx = bp_cache[bp_cached - 1].idx;
    uint8_t pair0 = bfr[idx], pair1 = bfr[idx + 1];
    
    // Check for pairs whose counts may be invalidated by this substitution, remove from cache
    // Such pairs are those whose second character matches the current substitution's first character,
    // or whose first character matches the current subsitution's second.
    // (Possible enhancement: double check them. They aren't certain to be invalidated.)
    // (Another possible enhancement: check when first searching for pairs.)
    bp_size_t n = 0;
    // Stop a bit short of the end. Not enough pairs left to be worthwhile.
    for(bp_size_t j = 0; j + 1 < bp_cached; ++j)
    {
        if(bfr[bp_cache[j].idx] != pair1 && bfr[bp_cache[j].idx + 1] != pair0) {
            // no possible conflict, keep
            if(j != n)
                bp_cache[n] = bp_cache[j];
            ++n;
        }
    }
    bp_cached = n;// also removes the pair just substituted
#else // BP_PAIR_CACHE_SIZE <= 1
//    printf("Counting pairs\n");
    bp_size_t pairCount = CountPairs(bfr, bfr + 2, size - 2);
//    printf("Counting more pairs\n");
    // Stop a bit short of the end. Not enough pairs left to be worthwhile.
    for(bp_size_t j = 1; (j + 8 + 2) < size; ++j)
    {
        uint8_t * newPair = bp_bfr + j;
        bp_size_t newPairCount = CountPairs(newPair, newPair + 2, size - 2 - j);
        if(newPairCount > pairCount) {
            bfr = newPair;
            pairCount = newPairCount;
        }
    }
    if(pairCount == 0) {
//        printf("No duplicate pairs to substitute!\n");
        return false;// No duplicate pairs to substitute
    }
    
    uint8_t pair0 = bfr[0], pair1 = bfr[1];// bfr is set to start of best pair
#endif // BP_PAIR_CACHE_SIZE
    
    // Replace each occurrence of pair with key
//    printf("Replacing %c%c (%02X %02X) with %02X\n", pair0, pair1, pair0, pair1, key);
    // bfr has been moved forward to the current pair
    uint8_t * bfrEnd = bp_bfr + size - 1;// stop 0-1 bytes short of the end
    uint8_t * bfrOut = bfr;
    while(bfr < bfrEnd)
    {
        if(*bfr == pair0 && *(bfr + 1) == pair1)
        {
            *(bfrOut++) = key;
            bfr += 2;
        }
        else
        {
            *(bfrOut++) = *(bfr++);
        }
    }
    // Clean up last byte if not covered by substitutions
    if(bfr < (bfrEnd + 1))
        *(bfrOut++) = *(bfr++);
    // Replace each occurrence of pair with key
//    printf("Replacement done\n");
    
    bp_csize = bfrOut - bp_bfr;
    
//    printf("Recording substitution\n");
    // Record substitution
    ++(bp_numSubs);
    bp_size_t subIdx = bp_dsize - bp_numSubs*3;
    bp_bfr[subIdx++] = key;
    bp_bfr[subIdx++] = pair0;
    bp_bfr[subIdx] = pair1;
    
    return true;
} // DoSubstitution()


void BP_Encode(void)
{
    bp_numSubs = 0;
    bp_csize = bp_dsize;
#if(BP_PAIR_CACHE_SIZE > 1)
    bp_cached = 0;
#endif
    
    // Collect used values
    GetUsed(bp_bfr, bp_dsize);
    
    for(uint8_t j = 0; j < BP_MAX_PASSES; ++j) {
        if(!DoSubstitution())
            break;// Can't compress, no more duplicate byte pairs
    }
} // BP_Encode()

//******************************************************************************
