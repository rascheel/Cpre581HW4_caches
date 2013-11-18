#include<iostream>
#include<fstream>
#include<stdint.h>
#include<math.h>
#include<cstdlib>
using namespace std;

//Depending on which of these is uncommented the program will run 
//the scenario for a different homework problem.
#define PROBLEM_1
//#define PROBLEM_2
//#define PROBLEM_3
//#define PROBLEM_4

//These are the settings for Problem 1 from the hw
#ifdef PROBLEM_1
#define CACHE_SIZE_BYTES 256
#define ASSOCIATIVITY 1
#define BLOCK_SIZE_BYTES 8
#endif

//These are the settings for Problem 2 from the hw
#ifdef PROBLEM_2
#define CACHE_SIZE_BYTES 256
#define ASSOCIATIVITY 4
#define BLOCK_SIZE_BYTES 8
#endif

//These are the settings for Problem 3 from the hw
#ifdef PROBLEM_3
#define CACHE_SIZE_BYTES 256
#define ASSOCIATIVITY 4
#define BLOCK_SIZE_BYTES 16
#endif

//These are the settings for Problem 4 from the hw
#ifdef PROBLEM_4
#define CACHE_SIZE_BYTES 256
#define ASSOCIATIVITY 1
#define BLOCK_SIZE_BYTES 8
#endif

//These settings are always determined programmatically
#define BLOCK_INDEX_BITS log2(BLOCK_SIZE_BYTES)
#define SET_COUNT CACHE_SIZE_BYTES/(BLOCK_SIZE_BYTES*ASSOCIATIVITY)
#define SET_INDEX_BITS log2(SET_COUNT)
#define TAG_INDEX_BITS (32 - SET_INDEX_BITS - BLOCK_INDEX_BITS)

#define DEBUG 0
#define debugPrint(x) if(DEBUG) cout << hex << (x) << endl;

int log2( int x )
{
    return log(x)/log(2);
}

class Cache
{
    public:
    struct block_t
    {
        uint32_t data[BLOCK_SIZE_BYTES/4];
        uint32_t tag;
        uint32_t full_address;
        uint32_t lastAccessed; // 0 is most recent
    };

    struct set_t
    {
        block_t blocks[ASSOCIATIVITY];
    };

    set_t cache[SET_COUNT];

    unsigned long hits;
    unsigned long misses;

    ofstream outputCsv;

    Cache()
    {
        hits = 0;
        misses = 0;
        for(int i = 0; i < SET_COUNT; i++)
        {
            for( int j = 0; j < ASSOCIATIVITY; j++)
            {
                cache[i].blocks[j].lastAccessed = 0;
                cache[i].blocks[j].tag = 0;
                cache[i].blocks[j].full_address = 0;
            }
        }

        outputCsv.open("output.csv");
        outputCsv << "Address Accessed" << "," << "Hit or Miss" << ",";
        for(int i = 0; i < SET_COUNT; i++)
        {
            outputCsv << "Set: " << hex << i << ",";
        }
        outputCsv << endl;
    }

    uint32_t get(uint32_t address)
    {
        debugPrint( address );
        outputCsv << hex << address << ",";
        uint32_t allF = 0xFFFFFFFF;
        uint32_t tag_index = address & (allF << (32 - TAG_INDEX_BITS));

        uint32_t set_index = (address & (allF >> TAG_INDEX_BITS)) 
                                      & (allF << BLOCK_INDEX_BITS);
        set_index = set_index >> BLOCK_INDEX_BITS;

        uint32_t block_index = address & (allF >> (32-BLOCK_INDEX_BITS));

        for(int i = 0; i < ASSOCIATIVITY; i++)
        {
            if(cache[set_index].blocks[i].tag == tag_index)
            {
                hits += 1;
                outputCsv << "HIT" << ",";
                incrementAccesses( &(cache[set_index]) );
                cache[set_index].blocks[i].lastAccessed = 0;
                printCacheToCsv();
                return cache[set_index].blocks[i].data[0];//I don't actually care about the data so just return the first one
            }
        }

        //We missed so determine which set was least recently used
        misses += 1;
        outputCsv << "MISS" << ",";
        int leastRecentlyUsed = 0;
        for(int i = 0; i < ASSOCIATIVITY; i++)
        {
            if( cache[set_index].blocks[i].lastAccessed > cache[set_index].blocks[leastRecentlyUsed].lastAccessed )
            {
                leastRecentlyUsed = i;
            }
        }

        incrementAccesses( &(cache[set_index]) );
        cache[set_index].blocks[leastRecentlyUsed].tag = tag_index;
        cache[set_index].blocks[leastRecentlyUsed].full_address = address;
        cache[set_index].blocks[leastRecentlyUsed].lastAccessed = 0;
        printCacheToCsv();
        return cache[set_index].blocks[leastRecentlyUsed].data[0];//I don't actually care about the data so just return the first one
    }

    void incrementAccesses( set_t* set )
    {
        for(int i = 0; i < ASSOCIATIVITY; i++)
        {
            set->blocks[i].lastAccessed += 1;
        }
    }

    float getHitRate()
    {
        float totalAccesses = hits + misses;
        return ((float)hits)/totalAccesses;
    }

    void printCacheToCsv()
    {
        for(int i = 0; i < SET_COUNT; i++)
        {
            for(int j = 0; j < ASSOCIATIVITY; j++)
            {
                outputCsv << hex << cache[i].blocks[j].full_address;
                if(j != ASSOCIATIVITY - 1)
                {
                    outputCsv << ";";
                }
            }
            outputCsv << ",";
        }
        outputCsv << endl;
    }

};

int main()
{
    /* Initialize the address arrays */
    uint32_t A[10][10];
    uint32_t B[10][10];
    uint32_t C[10][10];
    int aStart = 1024;
    int bStart = 2048;
    int cStart = 3072;
    for(int i = 0; i < 10; i++ )
    {
        for( int j = 0; j < 10; j++)
        {
            A[i][j] = aStart;
            aStart += 4;

            B[i][j] = bStart;
            bStart += 4;

            C[i][j] = cStart;
            cStart += 4;
        }
    }
    /* Done initializing address arrays */

    Cache cache;

    //If Calculating for problem 4 then use the optimized matrix multiply.
#ifdef PROBLEM_4
    int block_size_int = BLOCK_SIZE_BYTES/4;

    for(int jj = 0; jj < 10; jj = jj + block_size_int)
    {
    for(int kk = 0; kk < 10; kk = kk + block_size_int)
    {
    for (int i=0; i < 10; i++)
    {
        for (int j=0; j < min(jj+block_size_int, 10); j++) 
        {
            int temp = 0;
            for (int k = 0; k < min(kk+block_size_int, 10); k++) 
            {
                temp = temp + cache.get(B[i][k])*cache.get(C[k][j]);
            }

            cache.get(A[i][j]); 
        }
    }
    }
    }
    //else stick to the simplified matrix multiply
#else
    for (int i=0; i<10; i++)
    {
        for (int j=0; j<10; j++) 
        {
            int temp = 0;
            for (int k = 0; k < 10; k++) 
            {
                temp = temp + cache.get(B[i][k])*cache.get(C[k][j]);
            }

            cache.get(A[i][j]); 
        }
    }
#endif

    cout << "Number of tag index bits: " << TAG_INDEX_BITS << endl;
    cout << "Number of set index bits: " << SET_INDEX_BITS << endl;
    cout << "Number of block index bits: " << BLOCK_INDEX_BITS << endl;
    cout << "Hit rate: " << cache.getHitRate() << endl;
    cache.outputCsv.close();
    return 0;
}
