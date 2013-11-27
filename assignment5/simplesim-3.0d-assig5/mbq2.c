#define LOOPS 800000
//#define LOOPS_2 320000
#define LOOPS_2 32000000
#include <stdio.h>
int main(){
    
    int i;
    int array[LOOPS];
    int array_2[LOOPS_2];
    int sum = 0;
    int j=0;
    int iterations = 0;    
    
    //since we are accessing all elements of this array with a stride size of 32 ints (2 cache lines)
    //the prefetching will prefetch every access since we have a constant stride of 32 * 4(sizeofint) 
    for(i=0;i<LOOPS;i+=32){
        array[i] = i; 
    }
   
    //since we are constantly changing the stride incrementing j, i will follow the pattern of 0,16,48,96, etc
    //because the stride is always changing, the stride prefetcher will constantly be wrong, and be in the NO_PRED state
    //since it is in the NO_PRED state no prefetches will be generated and we will miss on every iteration
    for(i=0;i<LOOPS_2;i+=j){
        j+=16;
        array_2[i] = i;
        iterations++; 
    } 

    //expected total misses, the first loop 0 misses, the second loop 2000 misses
    //why this loop iterates 2000 times is because of the weird incrementation of both i and j and we found this from the counter itself
    printf("expected number of misses = iterations of second loop = %d\n",iterations);
   return array[LOOPS -1] + array_2[LOOPS_2 - 1];

}

