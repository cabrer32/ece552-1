
#define LOOPS 800000
#define LOOPS_2 32000
#define LOOPS_3 1000

int main(){
    
    int i;
    int array[LOOPS];
    int array_2[LOOPS_2];
    int sum = 0;
   int j=0;
   int k; 
   int iterations = 0; 
    
    //since we are accessing all elements of this array with a stride size of 32 ints (2 cache lines)
    //the prefetching will not help and we should have a cache miss every 1/32 times
    for(i=0;i<LOOPS;i+=32){
        array[i] = i; 

    }
    
    
   //After the first iteration of the outer loop the markov table should know the history of these misses 
   //Our stride will not be able to catch this however, and will miss on every iteration of the outer loop
   // but our open-ended will only miss the first time
   for(k=0;k<LOOPS_3;k++){
      iterations = 0;
      //half of these (1000/2000) will cold miss. 
      for(i=0;i<LOOPS_2;i+=j){
          j = (j+32) % 64;
          array_2[i] = i + k;
          iterations++;
      }
    } 
   
    //expected total misses, the first loop 0 misses, the second loop misses 1000 times (on the first iteration of k) 
    // and then since after that everything is stored in it's history it can now prefetch correctly
    printf("iterations %d\n", iterations);    
   return array[LOOPS -1] + array_2[LOOPS_2 - 1];

}

