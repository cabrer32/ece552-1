

#define OUTERLOOP1 50000
#define INNERLOOP1 3
#define OUTERLOOP2 20000
#define INNERLOOP2 7

int main()
{
	int i;
	int j;
	int a = 0;
	int b= 0;	
       
        //For a typical 2 bit saturating counter there will be one mispredict for each iteration of the outer loop (
        //(after the inner loop gets a not taken once j reaches it's counter)
        //however if we have a history we should be able to predict properly when the inner loop is about to finish
        //and predict the not taken of the inner loop 
	
        
	//The innerloop iterates 3 times which is less than the history bits (6). We should be able to track the entire
        //loop behaviour by this history.
	for(i=0; i< OUTERLOOP1; i++){
		for (j=0; j < INNERLOOP1; j++){
			a++;
			b++;
		}	
	}

	//The inner loop iterates 7 times per iteration of the outer loop which is more than the amount of history bits (6).
        //This should be as ineffective as the 2 bit
	//saturating counter and result in one mispredict per iteration of the outer loop (20000) 
	for(i=0; i< OUTERLOOP2; i++){
		for (j=0; j < INNERLOOP2; j++){
			a++;
			b++;
		}	
	}


	//we are expecting approximately 20000 mispredicts (coming from the second loop) since the first loop shouldn't
	//generate any mispredicts as the history should be able to track the behavior of the inner loop
	return i + j + a + b;


//if we look at the lower 3 bits of the PC on all the branches they are different (i.e no aliasing for the same history)
/*
  400250:	06 00 00 00 	bne $2,$0,400230 <main+0x40>
  400254:	f6 ff 00 02 
  400258:	43 00 00 00 	addiu $4,$4,1
  40025c:	01 00 04 04 
  400260:	5b 00 00 00 	slt $2,$7,$4
  400264:	00 02 04 07 
  400268:	05 00 00 00 	beq $2,$0,400228 <main+0x38>
  40026c:	ee ff 00 02 
  400270:	42 00 00 00 	addu $4,$0,$0
  400274:	00 04 00 00 
  400278:	42 00 00 00 	addu $3,$0,$0
  40027c:	00 03 00 00 
  400280:	43 00 00 00 	addiu $5,$5,1
  400284:	01 00 05 05 
  400288:	43 00 00 00 	addiu $6,$6,1
  40028c:	01 00 06 06 
  400290:	43 00 00 00 	addiu $3,$3,1
  400294:	01 00 03 03 
  400298:	5c 00 00 00 	slti $2,$3,7
  40029c:	07 00 02 03 
  4002a0:	06 00 00 00 	bne $2,$0,400280 <main+0x90>
  4002a4:	f6 ff 00 02 
  4002a8:	43 00 00 00 	addiu $4,$4,1
  4002ac:	01 00 04 04 
  4002b0:	5c 00 00 00 	slti $2,$4,20000
  4002b4:	20 4e 02 04 
  4002b8:	06 00 00 00 	bne $2,$0,400278 <main+0x88>
*/
}



