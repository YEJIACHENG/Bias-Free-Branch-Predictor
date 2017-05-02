/* bpred.h - branch predictor interfaces */
  
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


#ifndef BPRED_H
#define BPRED_H

#define dassert(a) assert(a)

#include <stdio.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "stats.h"

/*
 * This module implements a number of branch predictor mechanisms.  The
 * following predictors are supported:
 *
 *  BPred2Level:  two level adaptive branch predictor
 *
 *    It can simulate many prediction mechanisms that have up to
 *    two levels of tables. Parameters are:
 *         N   # entries in first level (# of shift register(s))
 *         W   width of shift register(s)
 *         M   # entries in 2nd level (# of counters, or other FSM)
 *    One BTB entry per level-2 counter.
 *
 *    Configurations:   N, W, M
 *
 *        counter based: 1, 0, M
 *
 *        GAg          : 1, W, 2^W
 *        GAp          : 1, W, M (M > 2^W)
 *        PAg          : N, W, 2^W
 *        PAp          : N, W, M (M == 2^(N+W))
 *
 *  BPred2bit:  a simple direct mapped bimodal predictor
 *
 *    This predictor has a table of two bit saturating counters.
 *    Where counter states 0 & 1 are predict not taken and
 *    counter states 2 & 3 are predict taken, the per-branch counters
 *    are incremented on taken branches and decremented on
 *    no taken branches.  One BTB entry per counter.
 *
 *
 *  BPredPerceptron: a perceptron based predictor
 *    
 *    Parameters are:
 *    <nr_of_perceptrons> <nr_of_weight_bits> <history_length>
 * 
 *  BPredTaken:  static predict branch taken
 *
 *  BPredNotTaken:  static predict branch not taken
 *
 */

#include "stdint.h"
#include <stdbool.h>


#define REMOVE_DUP_METHOD_1
#define FOLDED_GHR_HASH 
#define RECENCY_STACK

#define GHL 4096     // maximum history length our predictor attempts tp correalte with
#define LOG_BST 24
#define  WT_REG 22
#define  WL_1 16
#define  WT_RS 26
#define  WL_2 72
#define  REMOVE_DUP_DIST 32
#define  NFOLDED_HIST 512
#define  TRAIN_TH 107

#define PHISTWIDTH 16   

#define LOGL 6      //64 entries loop predictor 4-way skewed associative
#define WIDTHNBITERLOOP 10  // we predict only loops with less than 1K iterations
#define LOOPTAG 10    //tag width in the loop predictor

/*----- histLen array is used in conjuntion with filtered history as defined below in the code and is used to boost accuracy and provide multiple instances of a branch present in the past global history to the prediction function if required ------*/
#define NHIST 15
// int histLen[NHIST] = {64, 80, 96, 112, 128, 160, 192, 256, 320, 416, 512, 768, 1024, 1536, 2048};

#define PHIST 65           // length of the unflitered path history
#define FHIST_LEN 128      // length of the filtered history; tracks filtered histories that appeared at least at depth of EXTRA_PHIST_START_DIST in the global history;used for boosting the accuracy
#define EXTRA_PHIST_START_DIST 64
#define EXTRA_PHIST_END_DIST 1024
#define PHIST_SET 36  // size of a set allocated below; This set is used to remove duplicate instances in filtered histories {filteredGHR, filteredHA, filteredHADist}


#define PA_SHIFT_HASH 2 

typedef uint32_t UINT32;
typedef int32_t INT32;

typedef struct bst_entry
{
  /* state 00 means -> Not Found, 
    * 01 means -> Taken, 
    * 02 means -> Not Taken, 
    * 03 means -> Non-biased */
  int8_t state;
} bst_entry;


/* branch predictor types */
enum bpred_class {
  BPredComb,                    /* combined predictor (McFarling) */
  BPred2Level,      /* 2-level correlating pred w/2-bit counters */
  BPred2bit,      /* 2-bit saturating cntr pred (dir mapped) */
  BPredTaken,     /* static predict taken */
  BPredNotTaken,    /* static predict not taken */
  BPredPerceptron,    /* perceptron predictor */
  BPred_NUM
};

/* an entry in a BTB */
struct bpred_btb_ent_t {
  md_addr_t addr;   /* address of branch being tracked */
  enum md_opcode op;    /* opcode of branch corresp. to addr */
  md_addr_t target;   /* last destination of branch when taken */
  struct bpred_btb_ent_t *prev, *next; /* lru chaining pointers */
};

/* direction predictor def */
struct bpred_dir_t {
  enum bpred_class class; /* type of predictor */
  union {
    struct {
      unsigned int size;  /* number of entries in direct-mapped table */
      unsigned char *table; /* prediction state table */
    } bimod;
    struct {
      int l1size;   /* level-1 size, number of history regs */
      int l2size;   /* level-2 size, number of pred states */
      int shift_width;    /* amount of history in level-1 shift regs */
      int xor;      /* history xor address flag */
      int *shiftregs;   /* level-1 history table */
      unsigned char *l2table; /* level-2 prediction state table */
    } two;
  
  struct {
    int num_prcpt;    /* number   of perceptrons */
    int weight_bits;        /* number of bits per weight */
    int prcpt_history;    /* history length for the global history shift register */
    int *weights_table;   /* every entry is an array of weights, a perceptron */  
    int *masks_table;   /* only the masked weigths are calculated */
    unsigned long long *counters_table; /* how many times each perceptron is accessed */
    unsigned long long glbl_history;  /* global history of branches */
    unsigned long long spc_glbl_history;  /* speculative history of branches */

    /* --- weight tables --- */
    int8_t     **weight_m;          //1-dimensional bias weight table
    INT32     *weight_b;         //2-dimensional conventional perceptron predictor weight table
    int8_t     *weight_rs;         //1-dimensional weight table as proposed in the writeup
    /* --- Shift registers --- */
    bool HTrain; // A register indicate if training is needed 
    bool *GHR;  // Unfiltered history shift register used for prediction and updates

    bool *isBrStable;// A shift register/circular buffer indicating if a branch in PA was stable the time it was inserted into that

    UINT32 *HA; // Path address register containing the address of past branches without any filtering

    UINT32 *HAConsd;  // Set containing some path addresses used later in the code to remove duplicate instances of a branch in the prediction computation

    UINT32 *HistConsd;  // Set containing the absolute distances of branches, used later in the code to remove duplicate instances of a branch in the prediction computation

    /*-----Folloing three variables are used to capture filtered histories that appeared at least at depth of EXTRA_PHIST_START_DIST in the global history;used for boosting the accuracy -----*/
    bool *filteredGHR;// outcome of the branch in filtered history
    UINT32 *filteredHA; // address of the brancg in filtered history 
    UINT32 *filteredHADist; // absolute distance/depth of the branch in the global history


    UINT32 *folded_hist;// folded history array; n-th bit in this array computes folded history for the history bits from the n-th positon in the global unfiltered history to the current branch; 
    /*-----Folloing four variables are used to avoid the recomputation of perceptron table index during predictor retire/update, so they are merely used for speeding up the simulation, hence some space allocated for them are not included in the storage budget shown in the writeup -----*/
    UINT32 *OUTPOINT;
    UINT32 *idxConsd;
    bool *dirConsd;
    UINT32 corrFoundCurCycle;

    /*-----Folloing three variables are the three fields in Recency Stack (RS) defined in the writeup -----*/
    bool *nonDupFilteredGHR1;           //used for RS[].H to contain the latest outcome of a branch

    UINT32 *nonDupFilteredHA1;          //used for RS[].A to contain branch address/tag (hashed down to required number of bits)

    UINT32 *nonDupFilteredHADist1;      //used for RS[].P to contain absolute distance of the latest occurrence of a branch in global history


    INT32 GHRHeadPtr;//pointer to the start of the circular buffer GHR
    INT32 PHISTHeadPtr;       //pointer to the start of the circular buffer PA and isBrStable
    INT32 filteredGHRHeadPtr;//pointer to the start of the circular buffer filteredGHR, filteredGHRHA and filteredGHRDist

    INT32 threshold; // dynamic threshold value as in O-GEHL
    INT32 TC; //threshold counter as in O-GEHL

    UINT32 numFetchedCondBr;

    int Fetch_phist;    //path history
    int Retire_phist;   //path history
    int Seed;     // for the pseudo-random number generator

    bst_entry *bst_table;     //branch status table
    bool prcpt_pred;      // prcpt prediction


    UINT32 * PA;
    UINT32 * PAFound;
    UINT32 * PADistConsd;
  } BF_neural;
  } config;
};

/* branch predictor def */
struct bpred_t {
  enum bpred_class class; /* type of predictor */
  struct {
    struct bpred_dir_t *bimod;    /* first direction predictor */
    struct bpred_dir_t *twolev;   /* second direction predictor */
    struct bpred_dir_t *meta;   /* meta predictor */
  } dirpred;

  struct {
    int sets;     /* num BTB sets */
    int assoc;      /* BTB associativity */
    struct bpred_btb_ent_t *btb_data; /* BTB addr-prediction table */
  } btb;

  struct {
    int size;     /* return-address stack size */
    int tos;      /* top-of-stack */
    struct bpred_btb_ent_t *stack; /* return-address stack */
  } retstack;

  /* stats */
  counter_t addr_hits;    /* num correct addr-predictions */
  counter_t dir_hits;   /* num correct dir-predictions (incl addr) */
  counter_t used_ras;   /* num RAS predictions used */
  counter_t used_bimod;   /* num bimodal predictions used (BPredComb) */
  counter_t used_2lev;    /* num 2-level predictions used (BPredComb) */
  counter_t jr_hits;    /* num correct addr-predictions for JR's */
  counter_t jr_seen;    /* num JR's seen */
  counter_t jr_non_ras_hits;  /* num correct addr-preds for non-RAS JR's */
  counter_t jr_non_ras_seen;  /* num non-RAS JR's seen */
  counter_t misses;   /* num incorrect predictions */

  counter_t lookups;    /* num lookups */
  counter_t retstack_pops;  /* number of times a value was popped */
  counter_t retstack_pushes;  /* number of times a value was pushed */
  counter_t ras_hits;   /* num correct return-address predictions */
};

/* branch predictor update information */
struct bpred_update_t {
  char *pdir1;    /* direction-1 predictor counter */
  char *pdir2;    /* direction-2 predictor counter */
  char *pmeta;    /* meta predictor counter */
  struct {    /* predicted directions */
    unsigned int ras    : 1;  /* RAS used */
    unsigned int bimod  : 1;    /* bimodal predictor */
    unsigned int twolev : 1;    /* 2-level predictor */
    unsigned int meta   : 1;    /* meta predictor (0..bimod / 1..2lev) */
  } dir;
};

/* create a branch predictor */
struct bpred_t *      /* branch predictory instance */
bpred_create(enum bpred_class class,  /* type of predictor to create */
       unsigned int bimod_size, /* bimod table size */
       unsigned int l1size, /* level-1 table size */
       unsigned int l2size, /* level-2 table size */
       unsigned int meta_size,  /* meta predictor table size */
       unsigned int shift_width,  /* history register width */
       unsigned int xor,    /* history xor address flag */
       unsigned int btb_sets, /* number of sets in BTB */ 
       unsigned int btb_assoc,  /* BTB associativity */
       unsigned int retstack_size);/* num entries in ret-addr stack */

/* create a branch direction predictor */
struct bpred_dir_t *    /* branch direction predictor instance */
bpred_dir_create (
  enum bpred_class class, /* type of predictor to create */
  unsigned int l1size,    /* level-1 table size */
  unsigned int l2size,    /* level-2 table size (if relevant) */
  unsigned int shift_width, /* history register width */
  unsigned int xor);      /* history xor address flag */

/* print branch predictor configuration */
void
bpred_config(struct bpred_t *pred,  /* branch predictor instance */
       FILE *stream);   /* output stream */

/* print predictor stats */
void
bpred_stats(struct bpred_t *pred, /* branch predictor instance */
      FILE *stream);    /* output stream */

/* register branch predictor stats */
void
bpred_reg_stats(struct bpred_t *pred, /* branch predictor instance */
    struct stat_sdb_t *sdb);/* stats database */

/* reset stats after priming, if appropriate */
void bpred_after_priming(struct bpred_t *bpred);

/* probe a predictor for a next fetch address, the predictor is probed
   with branch address BADDR, the branch target is BTARGET (used for
   static predictors), and OP is the instruction opcode (used to simulate
   predecode bits; a pointer to the predictor state entry (or null for jumps)
   is returned in *DIR_UPDATE_PTR (used for updating predictor state),
   and the non-speculative top-of-stack is returned in stack_recover_idx 
   (used for recovering ret-addr stack after mis-predict).  */
md_addr_t       /* predicted branch target addr */
bpred_lookup(struct bpred_t *pred,  /* branch predictor instance */
       md_addr_t baddr,   /* branch address */
       md_addr_t btarget,   /* branch target if taken */
       enum md_opcode op,   /* opcode of instruction */
       int is_call,   /* non-zero if inst is fn call */
       int is_return,   /* non-zero if inst is fn return */
       struct bpred_update_t *dir_update_ptr, /* pred state pointer */
       int *stack_recover_idx); /* Non-speculative top-of-stack;
           * used on mispredict recovery */

/* Speculative execution can corrupt the ret-addr stack.  So for each
 * lookup we return the top-of-stack (TOS) at that point; a mispredicted
 * branch, as part of its recovery, restores the TOS using this value --
 * hopefully this uncorrupts the stack. */
void
bpred_recover(struct bpred_t *pred, /* branch predictor instance */
        md_addr_t baddr,    /* branch address */
        int stack_recover_idx); /* Non-speculative top-of-stack;
           * used on mispredict recovery */

/* update the branch predictor, only useful for stateful predictors; updates
   entry for instruction type OP at address BADDR.  BTB only gets updated
   for branches which are taken.  Inst was determined to jump to
   address BTARGET and was taken if TAKEN is non-zero.  Predictor 
   statistics are updated with result of prediction, indicated by CORRECT and 
   PRED_TAKEN, predictor state to be updated is indicated by *DIR_UPDATE_PTR 
   (may be NULL for jumps, which shouldn't modify state bits).  Note if
   bpred_update is done speculatively, branch-prediction may get polluted. */
void
bpred_update(struct bpred_t *pred,  /* branch predictor instance */
       md_addr_t baddr,   /* branch address */
       md_addr_t btarget,   /* resolved branch target */
       int taken,     /* non-zero if branch was taken */
       int pred_taken,    /* non-zero if branch was pred taken */
       int correct,   /* was earlier prediction correct? */
       enum md_opcode op,   /* opcode of instruction */
       struct bpred_update_t *dir_update_ptr); /* pred state pointer */


#ifdef foo0
/* OBSOLETE */
/* dump branch predictor state (for debug) */
void
bpred_dump(struct bpred_t *pred,  /* branch predictor instance */
     FILE *stream);   /* output stream */
#endif

#endif /* BPRED_H */