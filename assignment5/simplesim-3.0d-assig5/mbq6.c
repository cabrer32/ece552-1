
#define LOOPS 32000000
#define LOOPS_2 32000
#define LOOPS_3 15
#include <stdio.h>

int main(){
    
   int i;
   int array[LOOPS];
   int array_2[LOOPS];
   int sum = 0;
   int j=0;
   int k; 
   int iterations = 0; 
   int iterations2 = 0; 

   //the same fail case as our stride fail case, iterates 2000 times with constantly increasing stride
   //will miss on each iteration
   for(i=0;i<LOOPS;i+=j){
            j+=16;
            array[i] = i + j;
            iterations++;
   }
   
   //After the first iteration of the outer loop the markov table should know the history of these misses 
   //Our stride will not be able to catch this however, and will miss on every iteration of the outer loop = 15*2000 times
   // but our open-ended will always miss the same amount regardless of the number of iterations of the outer loop
   for (k=0; k<LOOPS_3;k++) {
       iterations2 =0;
       j=0;
       for(i=0;i<LOOPS;i+=j){
            j+= 16;
            array_2[i] = i + k;
            iterations2++;
       }
   } 
   
   //expected total misses, the first loop 2000 misses, the second loop misses 4000 times (on the first two iterations of k) 
   // and then since after that everything is stored in it's history it can now prefetch correctly
//   printf("iterations = %d iterations 2 = %d\n",iterations, iterations2);    
   return array[LOOPS -1] + array_2[LOOPS - 1];
}

