

#define OUTERLOOP 5000
#define INNERLOOP 5
int main()
{
	int i;
	int j;
	int a = 0;
	int b= 0;	


	for(i=0; i< OUTERLOOP; i++){
		for(j=0;j<6;j++){
			if (j >= 3){
			   a+=1;
			}		
		}
			

	}
	

	return i + j + a + b;



}



