
#include <limits.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "syscall.h"
#include "dlite.h"
#include "options.h"
#include "stats.h"
#include "sim.h"
#include "decode.def"

#include "instr.h"

/* PARAMETERS OF THE TOMASULO'S ALGORITHM */

#define INSTR_QUEUE_SIZE         10

#define RESERV_INT_SIZE    4
#define RESERV_FP_SIZE     2
#define FU_INT_SIZE        2
#define FU_FP_SIZE         1

#define FU_INT_LATENCY     4
#define FU_FP_LATENCY      9
#define NUM_INPUT_REGS	   3
#define NUM_OUTPUT_REGS	   2

/* IDENTIFYING INSTRUCTIONS */

//unconditional branch, jump or call
#define IS_UNCOND_CTRL(op) (MD_OP_FLAGS(op) & F_CALL || \
                         MD_OP_FLAGS(op) & F_UNCOND)

//conditional branch instruction
#define IS_COND_CTRL(op) (MD_OP_FLAGS(op) & F_COND)

//floating-point computation
#define IS_FCOMP(op) (MD_OP_FLAGS(op) & F_FCOMP)

//integer computation
#define IS_ICOMP(op) (MD_OP_FLAGS(op) & F_ICOMP)

//load instruction
#define IS_LOAD(op)  (MD_OP_FLAGS(op) & F_LOAD)

//store instruction
#define IS_STORE(op) (MD_OP_FLAGS(op) & F_STORE)

//trap instruction
#define IS_TRAP(op) (MD_OP_FLAGS(op) & F_TRAP) 

#define USES_INT_FU(op) (IS_ICOMP(op) || IS_LOAD(op) || IS_STORE(op))
#define USES_FP_FU(op) (IS_FCOMP(op))

#define WRITES_CDB(op) (IS_ICOMP(op) || IS_LOAD(op) || IS_FCOMP(op))

/* FOR DEBUGGING */

//prints info about an instruction
#define PRINT_INST(out,instr,str,cycle)	\
  myfprintf(out, "%d: %s", cycle, str);		\
  md_print_insn(instr->inst, instr->pc, out); \
  myfprintf(stdout, "(%d)\n",instr->index);

#define PRINT_REG(out,reg,str,instr) \
  myfprintf(out, "reg#%d %s ", reg, str);	\
  md_print_insn(instr->inst, instr->pc, out); \
  myfprintf(stdout, "(%d)\n",instr->index);

/* VARIABLES */

//instruction queue for tomasulo
static instruction_t* instr_queue[INSTR_QUEUE_SIZE];
//number of instructions in the instruction queue
static int instr_queue_size = 0;
static int instr_queue_head = 0;
static int instr_queue_tail = 0;

//reservation stations (each reservation station entry contains a pointer to an instruction)
static instruction_t* reservINT[RESERV_INT_SIZE];
static instruction_t* reservFP[RESERV_FP_SIZE];

//functional units
static instruction_t* fuINT[FU_INT_SIZE];
static instruction_t* fuFP[FU_FP_SIZE];

//common data bus
static instruction_t* commonDataBus = NULL;

//The map table keeps track of which instruction produces the value for each register
static instruction_t* map_table[MD_TOTAL_REGS];

//the index of the last instruction fetched
//static int fetch_index = 0;
static int trace_index = 0;
/* FUNCTIONAL UNITS */


/* RESERVATION STATIONS */
void push_to_int_RS (int index, int current_cycle) {
	int i;
	//check inputs in map table and update reservation station instruction Q field
	for (i = 0;i<NUM_INPUT_REGS;i++){
		if(reservINT[index]->r_in[i]==DNA);
		else 
			reservINT[index]->Q[i] = map_table[reservINT[index]->r_in[i]];
	}
	//update map table to point to correct reservation station insn
	for (i = 0;i<NUM_OUTPUT_REGS;i++) {
		if(reservINT[index]->r_out[i]==DNA);
                else 
			map_table[reservINT[index]->r_out[i]] = reservINT[index];
	}
	reservINT[index]->tom_dispatch_cycle = current_cycle;
}
void push_to_fp_RS (int index, int current_cycle) {
        int i;
        //check inputs in map table and update reservation station instruction Q field
        for (i = 0;i<NUM_INPUT_REGS;i++){
                if(reservFP[index]->r_in[i]==DNA);
                else 
                        reservFP[index]->Q[i] = map_table[reservFP[index]->r_in[i]];
        }
        //update map table to point to correct reservation station insn
        for (i = 0;i<NUM_OUTPUT_REGS;i++) {
                if(reservFP[index]->r_out[i]==DNA);
                else
                        map_table[reservFP[index]->r_out[i]] = reservFP[index];
        }
	reservFP[index]->tom_dispatch_cycle = current_cycle;
}

/* 
 * Description: 
 * 	Checks if simulation is done by finishing the very last instruction
 *      Remember that simulation is done only if the entire pipeline is empty
 * Inputs:
 * 	sim_insn: the total number of instructions simulated
 * Returns:
 * 	True: if simulation is finished
 */
static bool is_simulation_done() {

  /* ECE552: YOUR CODE GOES HERE */
  if (trace_index == sim_num_insn+1)
  	return true; //ECE552: you can change this as needed; we've added this so the code provided to you compiles
  else 
	return false;
}
/* 
 * Description: 
 * 	Retires the instruction from writing to the Common Data Bus
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void CDB_To_retire(int current_cycle) {

  /* ECE552: YOUR CODE GOES HERE */

}


/* 
 * Description: 
 * 	Moves an instruction from the execution stage to common data bus (if possible)
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void execute_To_CDB(int current_cycle) {

  /* ECE552: YOUR CODE GOES HERE */

}

/* 
 * Description: 
 * 	Moves instruction(s) from the issue to the execute stage (if possible). We prioritize old instructions
 *      (in program order) over new ones, if they both contend for the same functional unit.
 *      All RAW dependences need to have been resolved with stalls before an instruction enters execute.
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void issue_To_execute(int current_cycle) {

  /* ECE552: YOUR CODE GOES HERE */
}

/* 
 * Description: 
 * 	Moves instruction(s) from the dispatch stage to the issue stage
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void dispatch_To_issue(int current_cycle) {

  /* ECE552: YOUR CODE GOES HERE */
}

/* 
 * Description: 
 * 	Grabs an instruction from the instruction trace (if possible)
 * Inputs:
 *      trace: instruction trace with all the instructions executed
 * Returns:
 * 	None
 */

void fetch(instruction_trace_t* trace) {

  /* ECE552: YOUR CODE GOES HERE */
  //check if ifq full
  instruction_t* insn;
  //checks to see if ifq is full
  if (instr_queue_size == INSTR_QUEUE_SIZE); 
	//stalling so do not increment trace_index
  else {
	//room in ifq
	//get instruction from the trace (using trace_index)
	insn = get_instr(trace, trace_index);
        //if trap skip to next instruction in the trace until we get a non trap instruction
	while (IS_TRAP(insn->op)) {
		trace_index++;
		insn = get_instr(trace, trace_index);
	}

 	//put the non trap instruction in the queue by inserting at tail and moving tail down
	instr_queue[instr_queue_tail] = insn;
	instr_queue_tail = (instr_queue_tail + 1)%(INSTR_QUEUE_SIZE - 1); 
	

	//point to the next instruction in the trace
	trace_index++;
  }  
}

/* 
 * Description: 
 * 	Calls fetch and dispatches an instruction at the same cycle (if possible)
 * Inputs:
 *      trace: instruction trace with all the instructions executed
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void fetch_To_dispatch(instruction_trace_t* trace, int current_cycle) {
  int i;


  fetch(trace);

  //if it's a branch (conditional and uncoditional) go straight to dispatch, without checking a reservation
  //station
  //update the head since we pop off the top of the queue
  //decrease the queue size
  if (IS_UNCOND_CTRL(instr_queue[instr_queue_head]->op)||IS_COND_CTRL(instr_queue[instr_queue_head]->op)){
	instr_queue[instr_queue_head]->tom_dispatch_cycle = current_cycle;
	instr_queue_head = (instr_queue_head + 1)%(INSTR_QUEUE_SIZE -1);
	instr_queue_size--;
  }

  //if instruction uses integer fu (load, store, int alu insns)
  else if(USES_INT_FU(instr_queue[instr_queue_head]->op)){
  //check integer reservation stations to see if any available	
 	for(i=0; i <RESERV_INT_SIZE;i++){
        	if(reservINT[i]==NULL){
                        //empty reservation station
                        //have the reservation station point to the head of the ifq (allocating reservation station)
			reservINT[i] = instr_queue[instr_queue_head];

			//updates the q fields in the rs according to the map table indexed by the r_in fields of the insn
 			//updates the map table indexed by the r_out fields of the insn
			push_to_int_RS(i, current_cycle);
			//pop off the ifq
			instr_queue_head = (instr_queue_head + 1)%(INSTR_QUEUE_SIZE -1);
        		instr_queue_size--;	
		}	
	}
  }
  else if(USES_FP_FU(instr_queue[instr_queue_head]->op)){
  //check floating point reservation stations to see if any available  
        for(i=0; i <RESERV_FP_SIZE;i++){
                if(reservFP[i]==NULL){
                        //empty reservation station
                        //have the reservation station point to the head of the ifq (allocating reservation station)
                        reservFP[i] = instr_queue[instr_queue_head];
			//updates the q fields in the rs according to the map table indexed by the r_in fields of the insn
 			//updates the map table indexed by the r_out fields of the insn
                        push_to_fp_RS(i, current_cycle);
			//pop off the ifq
			instr_queue_head = (instr_queue_head + 1)%(INSTR_QUEUE_SIZE -1);
        		instr_queue_size--;	
                } 
        }
  }

  /* ECE552: YOUR CODE GOES HERE */
}

/* 
 * Description: 
 * 	Performs a cycle-by-cycle simulation of the 4-stage pipeline
 * Inputs:
 *      trace: instruction trace with all the instructions executed
 * 	sim_num_insn: the number of instructions in the trace
 * Returns:
 * 	The total number of cycles it takes to execute the instructions.
 */
counter_t runTomasulo(instruction_trace_t* trace)
{
  //initialize instruction queue
  int i;
  for (i = 0; i < INSTR_QUEUE_SIZE; i++) {
    instr_queue[i] = NULL;
  }

  //initialize reservation stations
  for (i = 0; i < RESERV_INT_SIZE; i++) {
      reservINT[i] = NULL;
  }

  for(i = 0; i < RESERV_FP_SIZE; i++) {
      reservFP[i] = NULL;
  }

  //initialize functional units
  for (i = 0; i < FU_INT_SIZE; i++) {
    fuINT[i] = NULL;
  }

  for (i = 0; i < FU_FP_SIZE; i++) {
    fuFP[i] = NULL;
  }

  //initialize map_table to no producers
  int reg;
  for (reg = 0; reg < MD_TOTAL_REGS; reg++) {
    map_table[reg] = NULL;
  }
  
  int cycle = 1;

  while (true) {

     /* ECE552: YOUR CODE GOES HERE */

     fetch_To_dispatch(trace, cycle);
     
     cycle++;
     
     if (is_simulation_done())
        break;
  }
  
  return cycle;
}
