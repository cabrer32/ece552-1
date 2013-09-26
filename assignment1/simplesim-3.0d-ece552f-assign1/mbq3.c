#define MAX_1CYCLE 250000
#define MAX_1CYCLE_LOAD 500000
#define MAX_2CYCLE_LOAD 100000
int main(){
	
	//volatile int load_val = 9;
	
	int index;
	int a=1;
	int b=1;
	volatile int c=1;
	volatile int d=1;
	volatile int e=5;
	volatile int f=5;
        int g=9;
	int h=7;	
	int i=8;	
	int j=2;
	int k=1;
	int l=6;
	int m=3;
	int n=7;	
        int o=4;
        
	index=0;
	//mem, alu, alu (stw, add, bne)
	while(index != MAX_1CYCLE_LOAD)
	{
		f = h+2;
		index+= b;
	}
	index =0;
	while(index != MAX_2CYCLE_LOAD){
//		//c is volatile here
		c = g+3;
		d = h+2;
		index +=b;
	}
	
//	return a+b+c+d+e+f+g+h+i;
	return a+index +e+f + h + g +i + j +l + m+ n +o;
	
}
