#define LOOPS 800000

int main(){
    
    int i;
    int array[LOOPS];
    int sum = 0;
    array[0]=0;
    for(i=1;i<LOOPS;i++){
        array[i]=i;
    }

  //  for(i=0;i<LOOPS;i++){
  //      sum+=array[i];
  //  }

    return array[LOOPS -1];

}

