
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

int get_int_RS_index (instruction_t* insn) {
  int i;
  for (i=0;i<RESERV_INT_SIZE;i++) {
	if (reservINT[i] == insn) 
		return i;
  }
  return -1;
}
int get_fp_RS_index (instruction_t* insn) {
  int i;
  for (i=0;i<RESERV_FP_SIZE;i++) {
	if (reservFP[i] == insn) 
		return i;
  }
  return -1;
}

bool is_in_int_fu(instruction_t* insn){
  int i;
  for(i=0;i<FU_INT_SIZE;i++){
  	if(fuINT[i] == insn) 
		return true;

  }
  return false;

}

bool is_in_fp_fu(instruction_t* insn){
  int i;
  for(i=0;i<FU_FP_SIZE;i++){
  	if(fuFP[i] == insn) 
		return true;

  }
  return false;

}
/*
goes through the integer reservation stations and returns the oldest issued RS who isnt waiting for RAW hazards
*/
int get_oldest_int_RS () {
  int j;
  int oldest_insn_available_index;
  int oldest_insn_available_issue_cycle = -1;	
  for (j=0; j<RESERV_INT_SIZE;j++) {
	//check that reservation station is allocated, check that the reservation station has already been issued, and then check that it has no RAW dependencies
	if(reservINT[j] != NULL && reservINT[j]->tom_issue_cycle != 0 && reservINT[j]->Q[0] == NULL && reservINT[j]->Q[1] == NULL && reservINT[j]->Q[2] == NULL && !is_in_int_fu(reservINT[j])) {
		if (oldest_insn_available_issue_cycle == -1) {
			oldest_insn_available_index = j;
			oldest_insn_available_issue_cycle = reservINT[j]->tom_issue_cycle;
		}
		else {
			if (reservINT[j]->tom_issue_cycle < oldest_insn_available_issue_cycle) {
				oldest_insn_available_index = j;
				oldest_insn_available_issue_cycle = reservINT[j]->tom_issue_cycle;
			}
		}
	}
  }
  if (oldest_insn_available_issue_cycle == -1)
	return -1;
  else 
	return oldest_insn_available_index;
}


/*
goes through the floating point reservation stations and returns the oldest issued RS who isnt waiting for RAW hazards
*/
int get_oldest_fp_RS () {
  int j;
  int oldest_insn_available_index;
  int oldest_insn_available_issue_cycle = -1;	
  for (j=0; j<RESERV_FP_SIZE;j++) {
	//check that reservation station is allocated, check that the reservation station has already been issued, and then check that it has no RAW dependencies
	if(reservFP[j] != NULL && reservFP[j]->tom_issue_cycle != 0 && reservFP[j]->Q[0] == NULL && reservFP[j]->Q[1] == NULL && reservFP[j]->Q[2] == NULL && !is_in_fp_fu(reservFP[j])) {
		if (oldest_insn_available_issue_cycle == -1) {
			oldest_insn_available_index = j;
			oldest_insn_available_issue_cycle = reservFP[j]->tom_issue_cycle;
		}
		else {
			if (reservFP[j]->tom_issue_cycle < oldest_insn_available_issue_cycle) {
				oldest_insn_available_index = j;
				oldest_insn_available_issue_cycle = reservFP[j]->tom_issue_cycle;
			}
		}
	}
  }
  if (oldest_insn_available_issue_cycle == -1)
	return -1;
  else 
	return oldest_insn_available_index;
}
/* Description
takes index which is index to an allocated reservation station
and current_cycle
It updates the Q fields of the reservation station by reading the map table 
and updates the map table to be the output register for this insn
*/
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
	reservINT[index]->tom_issue_cycle = current_cycle;
}

/* Description
takes index which is index to an allocated reservation station
and current_cycle
It updates the Q fields of the reservation station by reading the map table 
and updates the map table to be the output register for this insn
*/
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
	reservFP[index]->tom_issue_cycle = current_cycle;
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
  //check that everything is empty and therefore we can be done
  int i;  
  for (i=0;i<RESERV_INT_SIZE;i++) {
	if (reservINT[i] != NULL)
		return false;
  } 
  for (i=0;i<RESERV_FP_SIZE;i++) {
	if (reservFP[i] != NULL)
		return false;
  } 
  for (i=0;i<FU_INT_SIZE;i++) {
	if (fuINT[i] != NULL)
		return false;
  }
  for (i=0;i<FU_FP_SIZE;i++) {
	if (fuFP[i] != NULL)
		return false;
  }
  if (instr_queue_size != 0)
	return false;
  if (commonDataBus != NULL)
	return false;
  
  return true;

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
  int i;
  if (commonDataBus == NULL);
  else {
	//go through all the reservation stations and clear the Q bits that point to the same insn as the insn currently in CDB
	for (i=0;i<RESERV_INT_SIZE;i++) {
                if(reservINT[i] == NULL) continue;
		if (reservINT[i]->Q[0] == commonDataBus)
			reservINT[i]->Q[0] = NULL;
		if (reservINT[i]->Q[1] == commonDataBus)
			reservINT[i]->Q[1] = NULL;
		if (reservINT[i]->Q[2] == commonDataBus)
			reservINT[i]->Q[2] = NULL;
	}
	for (i=0;i<RESERV_FP_SIZE;i++) {
                if(reservFP[i] == NULL) continue;
		if (reservFP[i]->Q[0] == commonDataBus)
			reservFP[i]->Q[0] = NULL;
		if (reservFP[i]->Q[1] == commonDataBus)
			reservFP[i]->Q[1] = NULL;
		if (reservFP[i]->Q[2] == commonDataBus)
			reservFP[i]->Q[2] = NULL;
	}
	for (i=0;i<NUM_OUTPUT_REGS;i++) {
		if (map_table[commonDataBus->r_out[i]] == commonDataBus) 
			map_table[commonDataBus->r_out[i]] = NULL;
	}
	commonDataBus = NULL;	
  }
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
  int j;
  int oldest_fp_issue_cycle;
  int oldest_fp_fu_index = -1; 
  int oldest_int_issue_cycle;
  int oldest_int_fu_index = -1;
  int index;
 
  for (j=0; j<FU_FP_SIZE;j++) {
	//check that the functional unit is in use and it is done, then try to get the oldest of the done insns
	if(fuFP[j] != NULL && ((fuFP[j]->tom_execute_cycle + FU_FP_LATENCY) <= current_cycle)) {
		if (oldest_fp_fu_index == -1) {
			oldest_fp_fu_index = j;
			oldest_fp_issue_cycle = fuFP[j]->tom_issue_cycle;
		
		}
		else {
			if (fuFP[j]->tom_issue_cycle < oldest_fp_issue_cycle) {
				oldest_fp_fu_index = j;
				oldest_fp_issue_cycle = fuFP[j]->tom_issue_cycle;
			}
		}
	}
  }
   
  for (j=0; j<FU_INT_SIZE;j++) {
	//check that the functional unit is in use and it is done, then try to get the oldest of the done insns
	if(fuINT[j] != NULL && ((fuINT[j]->tom_execute_cycle + FU_INT_LATENCY) <= current_cycle)) {
		if (!WRITES_CDB(fuINT[j]->op)) {
			//if it is a store instruction, free everything
			fuINT[j]-> tom_cdb_cycle = current_cycle;
      			index = get_int_RS_index(fuINT[j]);
        		assert(index!=-1);                   
      			reservINT[index] = NULL;
			fuINT[j] = NULL;	
		}
		else if (oldest_int_fu_index == -1) {
			oldest_int_fu_index = j;
			oldest_int_issue_cycle = fuINT[j]->tom_issue_cycle;
		}
		else {
			if (fuINT[j]->tom_issue_cycle < oldest_int_issue_cycle) {
				oldest_int_fu_index = j;
				oldest_int_issue_cycle = fuINT[j]->tom_issue_cycle;
			}
		}
	}
  }
  //at this point we have the oldest floating point and the oldest integer units
  //if no functional unit completes this cycle and uses the CDB do nothing
  if (oldest_fp_fu_index == -1 && oldest_int_fu_index == -1);
  //if no floating point functional unit is writing to CDB but integer functional unit is use the oldest int
  else if(oldest_fp_fu_index == -1) {
	fuINT[oldest_int_fu_index]-> tom_cdb_cycle = current_cycle;
	commonDataBus = fuINT[oldest_int_fu_index];
      	index = get_int_RS_index(fuINT[oldest_int_fu_index]);
        assert(index!=-1);                   
	reservINT[index] = NULL;
	fuINT[oldest_int_fu_index] = NULL;	
  }
  //if no integer functional unit is writing to CDB but floating point functional unit is use the oldest fp
  else if(oldest_int_fu_index == -1) {
	fuFP[oldest_fp_fu_index]-> tom_cdb_cycle = current_cycle;
	commonDataBus = fuFP[oldest_fp_fu_index];
      	index = get_fp_RS_index(fuFP[oldest_fp_fu_index]);
        assert(index!=-1);                   
	reservFP[index] = NULL;
	fuFP[oldest_fp_fu_index] = NULL;	
  }
  //both an integer and floating point instruction both finish on this cycle choose the oldest between them to get the CDB
  else {
  	if(oldest_int_issue_cycle < oldest_fp_issue_cycle) {
		fuINT[oldest_int_fu_index]-> tom_cdb_cycle = current_cycle;
		commonDataBus = fuINT[oldest_int_fu_index];
      		index = get_int_RS_index(fuINT[oldest_int_fu_index]);
        	assert(index!=-1);                   
		reservINT[index] = NULL;
		fuINT[oldest_int_fu_index] = NULL;	
	}
	else {
		fuFP[oldest_fp_fu_index]-> tom_cdb_cycle = current_cycle;
		commonDataBus = fuFP[oldest_fp_fu_index];
      		index = get_fp_RS_index(fuFP[oldest_fp_fu_index]);
        	assert(index!=-1);                   
		reservFP[index] = NULL;
		fuFP[oldest_fp_fu_index] = NULL;	
	}
  }
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
  int oldest_insn_available_index;
  int i;
  //find a free functional unit and then try to issue an insn from a reservation station
  for (i=0; i<FU_INT_SIZE;i++) {
	if(fuINT[i]==NULL) {
		oldest_insn_available_index = get_oldest_int_RS();
		if (oldest_insn_available_index == -1) 
			break;
		else {
			fuINT[i] = reservINT[oldest_insn_available_index];
			fuINT[i]->tom_execute_cycle = current_cycle;
		}
	}
  }		
  for (i=0; i<FU_FP_SIZE;i++) {
	if(fuFP[i]==NULL) {
		oldest_insn_available_index = get_oldest_fp_RS();
		if (oldest_insn_available_index == -1) 
			break;
		else {
			fuFP[i] = reservFP[oldest_insn_available_index];
			fuFP[i]->tom_execute_cycle = current_cycle;
	  	}
  	}
  }
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
  int i;
  
  if (instr_queue[instr_queue_head] == NULL);
  //if it's a branch (conditional and uncoditional) go straight to dispatch, without checking a reservation
  //station
  //update the head since we pop off the top of the queue
  //decrease the queue size
  else if (IS_UNCOND_CTRL(instr_queue[instr_queue_head]->op)||IS_COND_CTRL(instr_queue[instr_queue_head]->op)){
	instr_queue[instr_queue_head]->tom_issue_cycle = current_cycle;
	instr_queue[instr_queue_head] = NULL;
	instr_queue_head = (instr_queue_head + 1)%(INSTR_QUEUE_SIZE);
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
			instr_queue[instr_queue_head] = NULL;
			instr_queue_head = (instr_queue_head + 1)%(INSTR_QUEUE_SIZE);
        		instr_queue_size--;
			break;	
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
			instr_queue[instr_queue_head] = NULL;
			instr_queue_head = (instr_queue_head + 1)%(INSTR_QUEUE_SIZE);
        		instr_queue_size--;
			break;	
                } 
        }
  }
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
  if (trace_index >= sim_num_insn);
	//dont attempt to fetch if there are no insns left in trace
  else if (instr_queue_size == INSTR_QUEUE_SIZE); 
	//stalling so do not increment trace_index
  else {
	//room in ifq
	//get instruction from the trace (using trace_index)
        trace_index++;
	insn = get_instr(trace, trace_index);
        //if trap skip to next instruction in the trace until we get a non trap instruction
	while (IS_TRAP(insn->op)) {
		trace_index++;
		insn = get_instr(trace, trace_index);
	}

 	//put the non trap instruction in the queue by inserting at tail and moving tail down
	instr_queue[instr_queue_tail] = insn;
	
	instr_queue_size++;
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
  /* ECE552: YOUR CODE GOES HERE */
  int size = instr_queue_size;
  fetch(trace);
  if (instr_queue_size > size) {
	instr_queue[instr_queue_tail]->tom_dispatch_cycle = current_cycle;	
  	instr_queue_tail = (instr_queue_tail + 1)%(INSTR_QUEUE_SIZE);
        assert((instr_queue_head + instr_queue_size)% INSTR_QUEUE_SIZE == instr_queue_tail); 
  }
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
     CDB_To_retire(cycle);
     execute_To_CDB(cycle);
     issue_To_execute(cycle);
     dispatch_To_issue(cycle);
     fetch_To_dispatch(trace, cycle);
     cycle++;
     
     if (is_simulation_done())
        break;
  }
  print_all_instr(trace, sim_num_insn);
  return cycle;
}
