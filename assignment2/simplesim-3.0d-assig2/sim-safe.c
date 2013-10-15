/* sim-safe.c - sample functional simulator implementation */

/* SimpleScalar(TM) Tool Suite
 * Copyright (C) 1994-2003 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 * All Rights Reserved. 
 * 
 * THIS IS A LEGAL DOCUMENT, BY USING SIMPLESCALAR,
 * YOU ARE AGREEING TO THESE TERMS AND CONDITIONS.
 * 
 * No portion of this work may be used by any commercial entity, or for any
 * commercial purpose, without the prior, written permission of SimpleScalar,
 * LLC (info@simplescalar.com). Nonprofit and noncommercial use is permitted
 * as described below.
 * 
 * 1. SimpleScalar is provided AS IS, with no warranty of any kind, express
 * or implied. The user of the program accepts full responsibility for the
 * application of the program and the use of any results.
 * 
 * 2. Nonprofit and noncommercial use is encouraged. SimpleScalar may be
 * downloaded, compiled, executed, copied, and modified solely for nonprofit,
 * educational, noncommercial research, and noncommercial scholarship
 * purposes provided that this notice in its entirety accompanies all copies.
 * Copies of the modified software can be delivered to persons who use it
 * solely for nonprofit, educational, noncommercial research, and
 * noncommercial scholarship purposes provided that this notice in its
 * entirety accompanies all copies.
 * 
 * 3. ALL COMMERCIAL USE, AND ALL USE BY FOR PROFIT ENTITIES, IS EXPRESSLY
 * PROHIBITED WITHOUT A LICENSE FROM SIMPLESCALAR, LLC (info@simplescalar.com).
 * 
 * 4. No nonprofit user may place any restrictions on the use of this software,
 * including as modified by the user, by any other authorized user.
 * 
 * 5. Noncommercial and nonprofit users may distribute copies of SimpleScalar
 * in compiled or executable form as set forth in Section 2, provided that
 * either: (A) it is accompanied by the corresponding machine-readable source
 * code, or (B) it is accompanied by a written offer, with no time limit, to
 * give anyone a machine-readable copy of the corresponding source code in
 * return for reimbursement of the cost of distribution. This written offer
 * must permit verbatim duplication by anyone, or (C) it is distributed by
 * someone who received only the executable form, and is accompanied by a
 * copy of the written offer of source code.
 * 
 * 6. SimpleScalar was developed by Todd M. Austin, Ph.D. The tool suite is
 * currently maintained by SimpleScalar LLC (info@simplescalar.com). US Mail:
 * 2395 Timbercrest Court, Ann Arbor, MI 48105.
 * 
 * Copyright (C) 1994-2003 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 */


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

/*
 * This file implements a functional simulator.  This functional simulator is
 * the simplest, most user-friendly simulator in the simplescalar tool set.
 * Unlike sim-fast, this functional simulator checks for all instruction
 * errors, and the implementation is crafted for clarity rather than speed.
 */


/* ECE552 Assignment 2 - STATS COUNTERS - BEGIN */ 

static counter_t sim_num_br = 0;		//total number of conditional branches
static counter_t sim_num_mispred_static = 0;	//number of mispredicted branches of the static predict-taken predictor
static counter_t sim_num_mispred_2bitsat = 0;	//number of mispredicted branches of the 2-bit saturating counter predictor
static counter_t sim_num_mispred_2level = 0;	//number of mispredicted branches of two level predictor
static counter_t sim_num_mispred_openend = 0;	//number of mispredicted branches of the open-ended predictor
static counter_t sim_num_mispred_gshare = 0;	//number of mispredicted branches of the open-ended predictor
static counter_t sim_num_mispred_pag  = 0;
#define NUM_SHIFT 3

/* ECE552 Assignment 2 - STATS COUNTERS - END */ 


 
/* simulated registers */
static struct regs_t regs;

/* simulated memory */
static struct mem_t *mem = NULL;

/* track number of refs */
static counter_t sim_num_refs = 0;

/* maximum number of inst's to execute */
static unsigned int max_insts;

/* register simulator-specific options */
void
sim_reg_options(struct opt_odb_t *odb)
{
  opt_reg_header(odb, 
"sim-safe: This simulator implements a functional simulator.  This\n"
"functional simulator is the simplest, most user-friendly simulator in the\n"
"simplescalar tool set.  Unlike sim-fast, this functional simulator checks\n"
"for all instruction errors, and the implementation is crafted for clarity\n"
"rather than speed.\n"
		 );

  /* instruction limit */
  opt_reg_uint(odb, "-max:inst", "maximum number of inst's to execute",
	       &max_insts, /* default */0,
	       /* print */TRUE, /* format */NULL);

}

/* check simulator-specific option values */
void
sim_check_options(struct opt_odb_t *odb, int argc, char **argv)
{
  /* nada */
}

/* register simulator-specific statistics */
void
sim_reg_stats(struct stat_sdb_t *sdb)
{
  stat_reg_counter(sdb, "sim_num_insn",
		   "total number of instructions executed",
		   &sim_num_insn, sim_num_insn, NULL);
  stat_reg_counter(sdb, "sim_num_refs",
		   "total number of loads and stores executed",
		   &sim_num_refs, 0, NULL);
  stat_reg_int(sdb, "sim_elapsed_time",
	       "total simulation time in seconds",
	       &sim_elapsed_time, 0, NULL);
  stat_reg_formula(sdb, "sim_inst_rate",
		   "simulation speed (in insts/sec)",
		   "sim_num_insn / sim_elapsed_time", NULL);

  ld_reg_stats(sdb);
  mem_reg_stats(mem, sdb);

		   
  /* ECE552 Assignment 2 - STATS COUNTERS/FORMULAS - BEGIN */ 
  stat_reg_counter(sdb, "sim_num_br", "total number of conditional branches", &sim_num_br, sim_num_br, NULL);

  stat_reg_counter(sdb, "sim_num_mispred_static", "Static preditor: number of mispredicted branches", &sim_num_mispred_static, sim_num_mispred_static, NULL);
  stat_reg_formula(sdb, "sim_br_static_ratio", "Static predictor: branch misprediction rate", "sim_num_mispred_static / sim_num_br", NULL);	

  stat_reg_counter(sdb, "sim_num_mispred_2bitsat", "2-bit sat. counter preditor: number of mispred. branches", &sim_num_mispred_2bitsat, sim_num_mispred_2bitsat, NULL);
  stat_reg_formula(sdb, "sim_br_2bitsat_ratio", "2-bit sat. counter predictor: branch misprediction rate", "sim_num_mispred_2bitsat / sim_num_br", NULL);	

  stat_reg_counter(sdb, "sim_num_mispred_2level", "Two level preditor: number of mispredicted branches", &sim_num_mispred_2level, sim_num_mispred_2level, NULL);
  stat_reg_formula(sdb, "sim_br_2level_ratio", "Two level predictor: branch misprediction rate", "sim_num_mispred_2level / sim_num_br", NULL);	

  stat_reg_counter(sdb, "sim_num_mispred_openend", "Open-ended preditor: number of mispredicted branches", &sim_num_mispred_openend, sim_num_mispred_openend, NULL);
  stat_reg_formula(sdb, "sim_br_openend_ratio", "Open-ended predictor: branch misprediction rate", "sim_num_mispred_openend / sim_num_br", NULL);	
  
  stat_reg_counter(sdb, "sim_num_mispred_gshare", "gshare preditor: number of mispredicted branches", &sim_num_mispred_gshare, sim_num_mispred_gshare, NULL);
  stat_reg_formula(sdb, "sim_br_gshare_ratio", "gshare predictor: branch misprediction rate", "sim_num_mispred_gshare / sim_num_br", NULL);	
  
 stat_reg_counter(sdb, "sim_num_mispred_pag", "pag preditor: number of mispredicted branches", &sim_num_mispred_pag, sim_num_mispred_pag, NULL);
  stat_reg_formula(sdb, "sim_br_pag_ratio", "pag predictor: branch misprediction rate", "sim_num_mispred_pag / sim_num_br", NULL);	
  /* ECE552 Assignment 2 - STATS COUNTERS/FORMULAS - END */ 

}

/* initialize the simulator */
void
sim_init(void)
{
  sim_num_refs = 0;

  /* allocate and initialize register file */
  regs_init(&regs);

  /* allocate and initialize memory space */
  mem = mem_create("mem");
  mem_init(mem);
}

/* load program into simulated state */
void
sim_load_prog(char *fname,		/* program to load */
	      int argc, char **argv,	/* program arguments */
	      char **envp)		/* program environment */
{
  /* load program text and data, set up environment, memory, and regs */
  ld_load_prog(fname, argc, argv, envp, &regs, mem, TRUE);

  /* initialize the DLite debugger */
  dlite_init(md_reg_obj, dlite_mem_obj, dlite_mstate_obj);
}

/* print simulator-specific configuration information */
void
sim_aux_config(FILE *stream)		/* output stream */
{
  /* nothing currently */
}

/* dump simulator-specific auxiliary simulator statistics */
void
sim_aux_stats(FILE *stream)		/* output stream */
{
  /* nada */
}

/* un-initialize simulator-specific state */
void
sim_uninit(void)
{
  /* nada */
}


/*
 * configure the execution engine
 */

/*
 * precise architected register accessors
 */

/* next program counter */
#define SET_NPC(EXPR)		(regs.regs_NPC = (EXPR))

/* current program counter */
#define CPC			(regs.regs_PC)

/* general purpose registers */
#define GPR(N)			(regs.regs_R[N])
#define SET_GPR(N,EXPR)		(regs.regs_R[N] = (EXPR))

#if defined(TARGET_PISA)

/* floating point registers, L->word, F->single-prec, D->double-prec */
#define FPR_L(N)		(regs.regs_F.l[(N)])
#define SET_FPR_L(N,EXPR)	(regs.regs_F.l[(N)] = (EXPR))
#define FPR_F(N)		(regs.regs_F.f[(N)])
#define SET_FPR_F(N,EXPR)	(regs.regs_F.f[(N)] = (EXPR))
#define FPR_D(N)		(regs.regs_F.d[(N) >> 1])
#define SET_FPR_D(N,EXPR)	(regs.regs_F.d[(N) >> 1] = (EXPR))

/* miscellaneous register accessors */
#define SET_HI(EXPR)		(regs.regs_C.hi = (EXPR))
#define HI			(regs.regs_C.hi)
#define SET_LO(EXPR)		(regs.regs_C.lo = (EXPR))
#define LO			(regs.regs_C.lo)
#define FCC			(regs.regs_C.fcc)
#define SET_FCC(EXPR)		(regs.regs_C.fcc = (EXPR))

#elif defined(TARGET_ALPHA)

/* floating point registers, L->word, F->single-prec, D->double-prec */
#define FPR_Q(N)		(regs.regs_F.q[N])
#define SET_FPR_Q(N,EXPR)	(regs.regs_F.q[N] = (EXPR))
#define FPR(N)			(regs.regs_F.d[(N)])
#define SET_FPR(N,EXPR)		(regs.regs_F.d[(N)] = (EXPR))

/* miscellaneous register accessors */
#define FPCR			(regs.regs_C.fpcr)
#define SET_FPCR(EXPR)		(regs.regs_C.fpcr = (EXPR))
#define UNIQ			(regs.regs_C.uniq)
#define SET_UNIQ(EXPR)		(regs.regs_C.uniq = (EXPR))

#else
#error No ISA target defined...
#endif

/* precise architected memory state accessor macros */
#define READ_BYTE(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_BYTE(mem, addr))
#define READ_HALF(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_HALF(mem, addr))
#define READ_WORD(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_WORD(mem, addr))
#ifdef HOST_HAS_QWORD
#define READ_QWORD(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_QWORD(mem, addr))
#endif /* HOST_HAS_QWORD */

#define WRITE_BYTE(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_BYTE(mem, addr, (SRC)))
#define WRITE_HALF(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_HALF(mem, addr, (SRC)))
#define WRITE_WORD(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_WORD(mem, addr, (SRC)))
#ifdef HOST_HAS_QWORD
#define WRITE_QWORD(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_QWORD(mem, addr, (SRC)))
#endif /* HOST_HAS_QWORD */

/* system call handler macro */
#define SYSCALL(INST)	sys_syscall(&regs, mem_access, mem, INST, TRUE)

/* start simulation, program loaded, processor precise state initialized */


/* ECE552 Assignment 2 - BEGIN  CODE */

//this function reverses the 12 pc bits (msb to lsb) so that we can reduce aliasing when we xor with the branch history register
int gshare_index_calculator(int pc){
  
  pc = pc >>NUM_SHIFT & (0xfff);
  int old_pc = pc;
  pc = old_pc << 11 | old_pc <<9 | old_pc <<7| old_pc <<5 | old_pc << 3 | old_pc<< 1||((old_pc&0x040)>>1)|((old_pc&0x080)>>3)|
 	((old_pc&0x0100)>>5)|((old_pc&0x0200)>>7)|((old_pc&0x0400)>>9)|((old_pc&0x0800)>>11);
  pc = pc & (0xfff);
  return pc;

}
/* ECE552 Assignment 2 - END CODE */      


void
sim_main(void)
{
  md_inst_t inst;
  register md_addr_t addr;
  enum md_opcode op;
  register int is_write;
  enum md_fault_type fault;

/* ECE552 Assignment 2 - BEGIN  CODE */
  //referenced by 12 bits of PC
  int i, j;

  //2 bit saturating counter indexed by 12 bits of pc
  int saturating_counter[4096];
  int saturating_counter_index;
  int saturating_counter_prediction;
  //2 level bht (branch history table) ppt (private predictor table)
  int bht[512];
  int ppt[64][8];
  int bht_index;
  int ppt_col_index;
  int ppt_row_index;
  int two_level_prediction;
  
  //For open-ended tournament gshare vs pag
  //gshare history 15 bytes (xor with 12 PC bits (reversed))
  int gshare_ghr = 0;
  //bit usage 32768 * 2 bit saturating counter = 64 k
  int gshare_pt[32768];
  int gshare_pt_index;
  int gshare_prediction;
 
  //choose array, each element has 8 bits to represent which predictor won the tournament 
  //in the last 4 trys
  int choose_last_correct[2048];
  int choose_prediction;
  int choose_index;
  int choose_temp;
  int choose_temp_masked;
  int choose_sat_sum = 0;
  int choose_gshare_sum = 0;

  //PAG: private history table indexed by 12 pc bits (history of 10 bits). 10 bits refers into global two bit counter.
  int pag_global_index;
  int pag_history_table[4096];
  int pag_two_bit_counter[1024];
  int pag_prediction; 
 
  //set history to 0 (all not taken) 
  for(i=0;i<4096;i++)
	pag_history_table[i]=0;

  for(i=0; i<512; i++)
	bht[i]=0;
 // initialize all 2 bit counter to weakly not taken (1)  
 // Strongly not taken 0, weakly not taken 1, weakly taken 2, strongly taken 3
  
  for(i=0;i<1024;i++)
	pag_two_bit_counter[i]=1;

  for(i=0;i<4096;i++) {
	saturating_counter[i]=1;
  }
 //initialize the ppt to weakly not taken 
  for(i=0;i<64;i++){
  	for(j=0;j<8;j++)
		ppt[i][j]=1;
  }
 //initialize the gshare pt to weakly not taken 
 for(i=0;i<32768;i++) {
	gshare_pt[i] = 1;
  }
  //initalize the choose to be 0 (not taken for both)
  for(i=0;i<2048;i++) 
	choose_last_correct[i] = 0;
/* ECE552 Assignment 2 - END CODE */      
  
  fprintf(stderr, "sim: ** starting functional simulation **\n");

  /* set up initial default next PC */
  regs.regs_NPC = regs.regs_PC + sizeof(md_inst_t);

  /* check for DLite debugger entry condition */
  if (dlite_check_break(regs.regs_PC, /* !access */0, /* addr */0, 0, 0))
    dlite_main(regs.regs_PC - sizeof(md_inst_t),
	       regs.regs_PC, sim_num_insn, &regs, mem);
  while (TRUE)
    {
      /* maintain $r0 semantics */
      regs.regs_R[MD_REG_ZERO] = 0;
#ifdef TARGET_ALPHA
      regs.regs_F.d[MD_REG_ZERO] = 0.0;
#endif /* TARGET_ALPHA */

      /* get the next instruction to execute */
      MD_FETCH_INST(inst, mem, regs.regs_PC);

      /* keep an instruction count */
      sim_num_insn++;
	  
      /* set default reference address and access mode */
      addr = 0; is_write = FALSE;

      /* set default fault - none */
      fault = md_fault_none;

      /* decode the instruction */
      MD_SET_OPCODE(op, inst);

      /* execute the instruction */
      switch (op)
	{
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3)		\
	case OP:							\
          SYMCAT(OP,_IMPL);						\
          break;
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT)					\
        case OP:							\
          panic("attempted to execute a linking opcode");
#define CONNECT(OP)
#define DECLARE_FAULT(FAULT)						\
	  { fault = (FAULT); break; }
#include "machine.def"
	default:
	  panic("attempted to execute a bogus opcode");
      }

      if (fault != md_fault_none)
	fatal("fault (%d) detected @ 0x%08p", fault, regs.regs_PC);

      if (verbose)
	{
	  myfprintf(stderr, "%10n [xor: 0x%08x] @ 0x%08p: ",
		    sim_num_insn, md_xor_regs(&regs), regs.regs_PC);
	  md_print_insn(inst, regs.regs_PC, stderr);
	  if (MD_OP_FLAGS(op) & F_MEM)
	    myfprintf(stderr, "  mem: 0x%08p", addr);
	  fprintf(stderr, "\n");
	  /* fflush(stderr); */
	}

      if (MD_OP_FLAGS(op) & F_MEM)
	{
	  sim_num_refs++;
	  if (MD_OP_FLAGS(op) & F_STORE)
	    is_write = TRUE;
	}
/* ECE552 Assignment 2 - BEGIN  CODE */
//read the prediction from our branch predictors here and we will compare it later when we figure 
//out when the branch is actually taken or not taken 
//branch instruction
      if(MD_OP_FLAGS(op)&F_COND){
	sim_num_br++;
        //saturating_counter_index = bits 14 to 3 of PC
        saturating_counter_index= (regs.regs_PC>>NUM_SHIFT)&(0x0fff); 
       saturating_counter_prediction = saturating_counter[saturating_counter_index];
        //bht_index= bits 14 to 6 of PC
        bht_index =  ((regs.regs_PC>>NUM_SHIFT)&(0x0fff)) >> 3;

        //ppt_col_index = bits 5 to 3 of PC
        ppt_col_index =  (regs.regs_PC>>NUM_SHIFT) & 0x07; 
        ppt_row_index = bht[bht_index];	
        two_level_prediction = ppt[ppt_row_index][ppt_col_index];
	
	//gshare index into history table with 15 bits of ghr xor with 12 bits of PC (reversed)(bits 3 to 14), reversing reduces aliasing
	gshare_pt_index = (gshare_index_calculator(regs.regs_PC))^(gshare_ghr&0x07fff);
        gshare_prediction = gshare_pt[gshare_pt_index]; 
	choose_index = (regs.regs_PC>>NUM_SHIFT)&(0x07ff);
	choose_temp = choose_last_correct[choose_index];
       
        //pag use 12 bits of PC (bits 14 to 3)
	pag_global_index= (regs.regs_PC>>NUM_SHIFT)&(0x0fff);
	pag_prediction = pag_two_bit_counter[pag_history_table[pag_global_index]];

 	//Goes through the last 4 tournament comparisons, if no clear winner then default to gshare
	//on each iteration
	// if 00 (both wrong, check next iteration until we find a winner or till reach 4 histories, no winner then resort to gshare)
	// if 01 (pag right, gshare wrong) go with pag
	// if 10 (gshare right, pag wrong) go with gshare
	// if 11 no clear winner (both right, same logic as 00)	
	for (i=0;i<4;i++) {
      		choose_temp_masked = choose_temp & 0x0ff;
      		choose_prediction = gshare_prediction;
      		if (choose_temp_masked == 0x01 ){
      			choose_prediction = pag_prediction;
				break;
        	}	
      		else if (choose_temp_masked == 0x10 ){
      			choose_prediction = gshare_prediction;
				break;
      		}
      		choose_temp = choose_temp >> 8;	
	}

	choose_last_correct[choose_index]=choose_last_correct[choose_index] << 8;

	
        pag_global_index= (regs.regs_PC>>NUM_SHIFT)&(0x0fff);
	pag_prediction = pag_two_bit_counter[pag_history_table[pag_global_index]];
	
      }        	

//branch is not taken
      if ((regs.regs_PC + sizeof(md_inst_t))== regs.regs_NPC && (MD_OP_FLAGS(op)&F_COND)){
	//if branch is not taken then static is wrong
	sim_num_mispred_static++;
        
	//2 BIT SATURATING
	//state machine for two bit counter
	// 0 strongly not taken, this value  will stay in strongly not taken
	// 1 weakly not taken, this value will turn into strongly not taken (0)
        // 2 weakly taken, this value will turn into weakly not taken (1) 
	// 3 strongly taken, this value will turn into weakly not taken (2)
        if(saturating_counter_prediction==0);
        else if(saturating_counter_prediction==1){
		saturating_counter[saturating_counter_index]=0;
        }
        else if(saturating_counter_prediction==2){
		saturating_counter[saturating_counter_index]=1;
		sim_num_mispred_2bitsat++;
        }
        else if(saturating_counter_prediction==3){
		saturating_counter[saturating_counter_index]=2;
		sim_num_mispred_2bitsat++;
        }
        //2 LEVEL
	//update the history being stored for the two level branch history table by shifting to the left
	//(shifts in a 0 which represents not taken) 
													
        bht[bht_index] = (bht[bht_index]<<1)& 0x03f;

	//each index of the private predictor table in the two level predictor is a 2 bit counter
	//refer to the 2 bit saturating description above for details
	//the indices into the ppt is calculated above (when we figure out it's a branch instruction)
	if(two_level_prediction==0);
        else if(two_level_prediction==1){
		ppt[ppt_row_index][ppt_col_index]=0;
        }
        else if(two_level_prediction==2){
		ppt[ppt_row_index][ppt_col_index]=1;
		sim_num_mispred_2level++;
        }
        else if(two_level_prediction==3){
		ppt[ppt_row_index][ppt_col_index]=2;
		sim_num_mispred_2level++;
        }
       
	//OPEN-ENDED
	 //gshare   
	//Global history register, shift in a not taken same way as history in two-level     
        gshare_ghr = (gshare_ghr<<1)& 0x07fff;

	//each index into gshare_pt is a 2 bit saturating counter, refer to description above
	//we also want to update if the gshare predict was correct (used by our tournament to decide between gshare and pag)
        //choose_last_correct stores whether
	// gshare and pag were correct in this cycle
	// when we or with 0x10 we are setting upper bit to 1 (gshare portion)
        // when we and with 0x01 we are setting upper bit to 0 (gshare portion, while preserving the pag portion)
	if(gshare_prediction==0)	
		choose_last_correct[choose_index] = choose_last_correct[choose_index] | 0x10;
        else if(gshare_prediction==1){
		gshare_pt[gshare_pt_index]=0;
		choose_last_correct[choose_index] = choose_last_correct[choose_index] | 0x10;
        }
        else if(gshare_prediction==2){
		gshare_pt[gshare_pt_index]=1;
		sim_num_mispred_gshare++;
		choose_last_correct[choose_index] = choose_last_correct[choose_index] & 0x01;
        }
        else if(gshare_prediction==3){
		gshare_pt[gshare_pt_index]=2;
		sim_num_mispred_gshare++;
		choose_last_correct[choose_index] = choose_last_correct[choose_index] & 0x01;
        }


        
	//choose
	//if the openended predictor predicted a 2 or a 3 (a weakly taken or strongly taken) we have a mispredict
	if (choose_prediction > 1) 
		sim_num_mispred_openend++;


	//each index into pag_two_bit_counter is a 2 bit saturating counter, refer to description above
	// also updating choose_last_correct refer to description above gshare
	// when we or with 0x01 we are setting lower bit to 1 (pag portion)
        // when we and with 0x10 we are setting lower bit to 0 (pag portion, while preserving the gshare portion)
	if(pag_prediction==0)			
		choose_last_correct[choose_index] = choose_last_correct[choose_index] | 0x01;
	else if(pag_prediction==1){
		pag_two_bit_counter[pag_history_table[pag_global_index]] = 0;
		choose_last_correct[choose_index] = choose_last_correct[choose_index] | 0x01;
	}
	else if(pag_prediction==2){
		pag_two_bit_counter[pag_history_table[pag_global_index]] = 1;
		sim_num_mispred_pag++;
		choose_last_correct[choose_index] = choose_last_correct[choose_index] & 0x10;
	}
	else if(pag_prediction==3){
		pag_two_bit_counter[pag_history_table[pag_global_index]] = 2;
		sim_num_mispred_pag++;
		choose_last_correct[choose_index] = choose_last_correct[choose_index] & 0x10;
	}
	pag_history_table[pag_global_index] = (pag_history_table[pag_global_index] << 1)&(0x03ff);        
	



		
      }

//branch taken
      else if(MD_OP_FLAGS(op)&F_COND){
        //2-bit SATURATING
	//2 BIT SATURATING
	//state machine for two bit counter
	// 0 strongly not taken, this value  will stay in strongly not taken
	// 1 weakly not taken, this value will turn into strongly not taken (0)
        // 2 weakly taken, this value will turn into weakly not taken (1) 
	
	if(saturating_counter_prediction==0){
	        saturating_counter[saturating_counter_index]=1;
		sim_num_mispred_2bitsat++;
	}        
        else if(saturating_counter_prediction==1){
		saturating_counter[saturating_counter_index]=2;
		sim_num_mispred_2bitsat++;
        }
        else if(saturating_counter_prediction==2){
		saturating_counter[saturating_counter_index]=3;
        }
        else if(saturating_counter_prediction==3);

        //2 LEVEL
	//update the history being stored for the two level branch history table by shifting to the left and or with 1 (introduces lsb of 1)
	//(shifts in a 1 which represents not taken) 
        bht[bht_index] = ((bht[bht_index]<<1)& 0x03f)|0x01;
	if(two_level_prediction==0){
		ppt[ppt_row_index][ppt_col_index]=1;
		sim_num_mispred_2level++;
        }
        else if(two_level_prediction==1){
		ppt[ppt_row_index][ppt_col_index]=2;
		sim_num_mispred_2level++;
        }
        else if(two_level_prediction==2){
		ppt[ppt_row_index][ppt_col_index]=3;
        }
        else if(two_level_prediction==3);
	
	//OPEN-ENDED
	 //gshare   
	//Global history register, shift in a not taken same way as history in two-level     
        gshare_ghr = ((gshare_ghr<<1)& 0x07fff) | 0x01;
	//each index into gshare_pt is a 2 bit saturating counter, refer to description above
	//we also want to update if the gshare predict was correct (used by our tournament to decide between gshare and pag)
        //choose_last_correct stores whether
	// gshare and pag were correct in this cycle
        // when we and with 0x01 we are setting upper bit to 0 (gshare portion, while preserving the pag portion)
	// when we or with 0x10 we are setting upper bit to 1 (gshare portion)
	if(gshare_prediction==0){
		gshare_pt[gshare_pt_index]=1;
		sim_num_mispred_gshare++;
		choose_last_correct[choose_index] = choose_last_correct[choose_index] & 0x01;	
	}
        else if(gshare_prediction==1){
		gshare_pt[gshare_pt_index]=2;
		sim_num_mispred_gshare++;
		choose_last_correct[choose_index] = choose_last_correct[choose_index] & 0x01;	
        }
        else if(gshare_prediction==2){
		gshare_pt[gshare_pt_index]=3;
		choose_last_correct[choose_index] = choose_last_correct[choose_index] | 0x10;
        }
        else if(gshare_prediction==3)
		choose_last_correct[choose_index] = choose_last_correct[choose_index] | 0x10;
	//choose
	//if the openended predictor predicted a 0 or a 1 (a weakly  not taken or strongly not  taken) we have a mispredict
	if (choose_prediction < 2) 
		sim_num_mispred_openend++;
	
	//each index into pag_two_bit_counter is a 2 bit saturating counter, refer to description above
	// also updating choose_last_correct refer to description above gshare
        // when we and with 0x10 we are setting lower bit to 0 (pag portion, while preserving the gshare portion)
	// when we or with 0x01 we are setting lower bit to 1 (pag portion)
	if(pag_prediction==0){
		sim_num_mispred_pag++;
		pag_two_bit_counter[pag_history_table[pag_global_index]] = 1;
		choose_last_correct[choose_index] = choose_last_correct[choose_index] & 0x10;
	}			
	else if(pag_prediction==1){
		sim_num_mispred_pag++;
		pag_two_bit_counter[pag_history_table[pag_global_index]] = 2;
		choose_last_correct[choose_index] = choose_last_correct[choose_index] & 0x10;
	}
	else if(pag_prediction==2){
		pag_two_bit_counter[pag_history_table[pag_global_index]] = 3;
		choose_last_correct[choose_index] = choose_last_correct[choose_index] | 0x01;
	}
	else if(pag_prediction==3)
		choose_last_correct[choose_index] = choose_last_correct[choose_index] | 0x01;
	pag_history_table[pag_global_index] = ((pag_history_table[pag_global_index] << 1)&(0x03ff))|(0x01);        
	
      }
      

      

/* ECE552 Assignment 2 - END CODE */      
      /* check for DLite debugger entry condition */
      if (dlite_check_break(regs.regs_NPC,
			    is_write ? ACCESS_WRITE : ACCESS_READ,
			    addr, sim_num_insn, sim_num_insn))
	dlite_main(regs.regs_PC, regs.regs_NPC, sim_num_insn, &regs, mem);

      /* go to the next instruction */
      regs.regs_PC = regs.regs_NPC;
      regs.regs_NPC += sizeof(md_inst_t);

      /* finish early? */
      if (max_insts && sim_num_insn >= max_insts)
	return;
    }
}
