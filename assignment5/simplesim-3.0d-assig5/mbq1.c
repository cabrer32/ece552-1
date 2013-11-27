#define LOOPS 800000
#define LOOPS_2 320000

int main(){
    
    int i;
    int array[LOOPS];
    int array_2[LOOPS_2];
    int sum = 0;
    
    
    //since we are accessing all elements of this array contigiously
    //the prefetching should allow for a 0 percent miss rate in our cache
    for(i=0;i<LOOPS;i++){
        array[i]=i;
    }
    
    //since we are accessing all elements of this array with a stride size of 32 ints (2 cache lines)
    //the prefetching will not help and we should have a cache miss every 1/32 times
    for(i=0;i<LOOPS_2;i+=32){
        array_2[i] = i; 

    }
    

    //expected total misses, the first loop 0 misses, the second loop 1/32*320000 = 10000 misses
    
    return array[LOOPS -1] + array_2[LOOPS_2 - 1];

}

