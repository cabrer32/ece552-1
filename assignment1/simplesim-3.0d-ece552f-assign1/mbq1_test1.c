#define MAX_2CYCLE 20000
#define MAX_1CYCLE 30000


//COMPILED WITH O1 OPTIMIZATION FLAG

int main(){
	int index;
//	int var[7]={5} ;
	int a=1;
	int b=2;
	int c=3;
	int d=4;
	int e=5;
	int f=5;
        int g=9;
	int h=7;	
	int i=8;	
/*
STATISTICS FROM OUTPUT RUN 1
OUR 2 CYCLE STALL LOOP  RUNS FOR 50000000 ITERATIONS AND OUR SIMULATOR COUNTS 50004057 (4057 OVERHEAD)
OUR 1 CYCLE STALL LOOP  RUNS FOR 10000000 ITERATIONS AND OUR SIMULATOR COUNTS 10000089 (89 OVERHEAD)
 
sim_num_RAW_hazard_q1      60004146 # total number of RAW hazards (q1)
sim_num_RAW_hazard_q1_stall_1     10000089 # total number of RAW hazards with 1 cycle stall (q1)
sim_num_RAW_hazard_q1_stall_2     50004057 # total number of RAW hazards with 2 cycle stall(q1)

STATISTICS FROM OUTPUT RUN 2
OUR 2 CYCLE STALL LOOP  RUNS FOR 20000000 ITERATIONS AND OUR SIMULATOR COUNTS 20004057 (4057 OVERHEAD)
OUR 1 CYCLE STALL LOOP  RUNS FOR 30000000 ITERATIONS AND OUR SIMULATOR COUNTS 30000089 (89 OVERHEAD)

sim_num_RAW_hazard_q1      50004146 # total number of RAW hazards (q1)
sim_num_RAW_hazard_q1_stall_1     30000089 # total number of RAW hazards with 1 cycle stall (q1)
sim_num_RAW_hazard_q1_stall_2     20004057 # total number of RAW hazards with 2 cycle stall(q1)

NOTICE THE OVERHEAD STAYS THE SAME
*/

	//2 CYCLE STALL
/*
$L4:
	addu	$2,$8,$4  #reg 2 is written to
	addu	$12,$2,$9 #reg 2 is read causing the 2 cycle stall
	addu	$3,$3,1   #loop increment
	addu	$4,$4,3   #these 2 instructions are padding to avoid
	addu	$5,$5,2   #a 2 cycle stall due to increment and branch
	bne	$3,$6,$L4
*/

//	index=0;
//	while(index!=MAX_2CYCLE){
//		a=b+e; //a causes the 2 cycle stall
//		d=a+c;
//		index++;
//		e+=3; //padding to avoid stall due to increment and 
//		f+=2; //branch
//	}
        //1 CYCLE STALL

/*$L8:
	addu	$6,$8,$4   #reg 6 is written to
	addu	$7,$7,2    #padding to make it a 1 cycle stall instead of 2
	addu	$10,$6,$9  #reg 6 is read causing 1 cycle stall
	addu	$3,$3,1    #increment
	addu	$4,$4,3    #2 instruction padding
	addu	$5,$5,2
	bne	$3,$11,$L8
*/
	index=0;
	while(index!=MAX_1CYCLE){
		h=b+e;  //h is written to
		g+=2;   //padding (to make 2 cycle stall to 1)
		i=h+c;  //h is read causing 1 cycle stall
		index++;
		e+=3;   //padding
		f+=2;
	}

	return a+b+c+d+e+f+g+h+i;
	//return all the variables to avoid the compiler optimizing them out

}
