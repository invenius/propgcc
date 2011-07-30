/* Definitions of target machine for GNU compiler, Propeller architecture.
   Contributed by Eric Smith <ersmith@totalspectrum.ca>

   Copyright (C) 2011 Parallax, Inc.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 3, or (at your
   option) any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

#ifndef GCC_PROPELLER_H
#define GCC_PROPELLER_H

/*-------------------------------*/
/* Run-time Target Specification */
/*-------------------------------*/

/* Print subsidiary information on the compiler version in use.  */
#ifndef TARGET_VERSION
#define TARGET_VERSION fprintf (stderr, " (Propeller)")
#endif

/* Target CPU builtins.  */
#define TARGET_CPU_CPP_BUILTINS()                       \
  do                                                    \
    {                                                   \
      builtin_define ("__propeller__");                      \
      builtin_assert ("cpu=propeller");                      \
      builtin_assert ("machine=propeller");                  \
    }                                                   \
  while (0)

/*---------------------------------*/
/* Target machine storage layout.  */
/*---------------------------------*/

#define BITS_BIG_ENDIAN 0
#define BYTES_BIG_ENDIAN 0
#define WORDS_BIG_ENDIAN 0

#define BITS_PER_UNIT 8
#define BITS_PER_WORD 32
#define UNITS_PER_WORD 4

#define POINTER_SIZE 32

#define PROMOTE_MODE(MODE,UNSIGNEDP,TYPE)               \
do {                                                    \
  if (GET_MODE_CLASS (MODE) == MODE_INT                 \
      && GET_MODE_SIZE (MODE) < UNITS_PER_WORD)         \
    (MODE) = word_mode;                                 \
} while (0)

#define PARM_BOUNDARY 32

#define STACK_BOUNDARY 32

#define BIGGEST_ALIGNMENT 64

#define FUNCTION_BOUNDARY  32

#define EMPTY_FIELD_BOUNDARY 32

#define STRICT_ALIGNMENT 1

#define TARGET_FLOAT_FORMAT IEEE_FLOAT_FORMAT

/* Make strings word-aligned so strcpy from constants will be faster.  */
#define CONSTANT_ALIGNMENT(EXP, ALIGN)  \
  (TREE_CODE (EXP) == STRING_CST	\
   && (ALIGN) < BITS_PER_WORD ? BITS_PER_WORD : (ALIGN))

/* Make arrays and structures word-aligned to allow faster copying etc.  */
#define DATA_ALIGNMENT(TYPE, ALIGN)					\
  ((((ALIGN) < BITS_PER_WORD)						\
    && (TREE_CODE (TYPE) == ARRAY_TYPE					\
	|| TREE_CODE (TYPE) == UNION_TYPE				\
	|| TREE_CODE (TYPE) == RECORD_TYPE)) ? BITS_PER_WORD : (ALIGN))

/* We need this for the same reason as DATA_ALIGNMENT, namely to cause
   character arrays to be word-aligned so that `strcpy' calls that copy
   constants to character arrays can be done inline, and 'strcmp' can be
   optimised to use word loads.  */
#define LOCAL_ALIGNMENT(TYPE, ALIGN) \
  DATA_ALIGNMENT (TYPE, ALIGN)

/* Nonzero if access to memory by bytes is slow and undesirable.  */
#define SLOW_BYTE_ACCESS 0

#define SLOW_UNALIGNED_ACCESS(MODE, ALIGN) 1

/* Define if operations between registers always perform the operation
   on the full register even if a narrower mode is specified.  */
#define WORD_REGISTER_OPERATIONS

/*-------------*/
/* Profiling.  */
/*-------------*/

#define FUNCTION_PROFILER(FILE, LABELNO)

/*---------------*/
/* Trampolines.  */
/*---------------*/

#define TRAMPOLINE_SIZE		0

/*----------------------------------------*/
/* Layout of source language data types.  */
/*----------------------------------------*/
#define INT_TYPE_SIZE		    32
#define SHORT_TYPE_SIZE		    16
#define LONG_TYPE_SIZE		    32
#define LONG_LONG_TYPE_SIZE	    64

#define FLOAT_TYPE_SIZE		    32
#define DOUBLE_TYPE_SIZE	    64
#define LONG_DOUBLE_TYPE_SIZE       64

#define DEFAULT_SIGNED_CHAR         0

#define SIZE_TYPE "unsigned int"

#define PTRDIFF_TYPE "int"

/* An alias for the machine mode for pointers.  */
#define Pmode         SImode

/* An alias for the machine mode used for memory references to
   functions being called, in `call' RTL expressions.  */
#define FUNCTION_MODE Pmode

/* Specify the machine mode that this machine uses
   for the index in the tablejump instruction.  */
#define CASE_VECTOR_MODE Pmode


/*---------------------------*/
/* Standard register usage.  */
/*---------------------------*/

/* The propeller is an unusual machine in that it has 512 registers;
 * but the code (at least in non-LMM mode) is actually stored in
 * the register space, taking up some registers. In LMM mode the
 * registers hold a simple interpreter that reads instructions from
 * external RAM and executes them.
 * To simplify things we reserve some registers for traditional uses:
 * 16 general purposes registers (r0-r15), a stack pointer sp, and
 * a program counter pc (which is only used in LMM mode).
 * Our registers are numbered:
 *  0-15:  r0-r15
 *  16:    sp
 *  17:    pc
 *  18:    cc (not really accessible)
 *  19:    fake argument pointer (eliminated after register allocation)
 *  20:    fake frame pointer    (eliminated after register allocation)
 * register usage in the ABI is:
 *  r0-r7: available for parameters and not saved
 *  r8-r14: saved across function calls
 *  r15 is used to save return addresses (link register)
 *  r14 is used as the frame pointer
 */
#define REGISTER_NAMES {	\
  "r0", "r1", "r2", "r3",   \
  "r4", "r5", "r6", "r7",   \
  "r8", "r9", "r10", "r11",   \
  "r12", "r13", "r14", "lr",   \
  "sp", "pc", "?cc" }

#define PROP_R0        0
#define PROP_R1        1
#define PROP_FP_REGNUM 14
#define PROP_LR_REGNUM 15
#define PROP_SP_REGNUM 16
#define PROP_PC_REGNUM 17
#define PROP_CC_REGNUM 18

#define FIRST_PSEUDO_REGISTER 19

enum reg_class
{
  NO_REGS,
  GENERAL_REGS,
  SPECIAL_REGS,
  CC_REGS,
  ALL_REGS,
  LIM_REG_CLASSES
};

#define REG_CLASS_CONTENTS \
{ { 0x00000000 }, /* Empty */			   \
  { 0x0001FFFF }, /* r0-r15, sp, */        \
  { 0x00020000 }, /* pc */	                   \
  { 0x00040000 }, /* cc */                        \
  { 0x0007FFFF }  /* All registers */              \
}

#define N_REG_CLASSES LIM_REG_CLASSES

#define REG_CLASS_NAMES {\
    "NO_REGS", \
    "GENERAL_REGS", \
    "SPECIAL_REGS", \
    "CC_REGS", \
    "ALL_REGS" }

#define FIXED_REGISTERS \
{                       \
  0,0,0,0,0,0,0,0,      \
  0,0,0,0,0,0,0,0,      \
  1,1,1,        \
}

/* 1 for registers not available across function calls
 * these must include the FIXED_REGISTERS and also any registers
 * that can be used without being saved
 */
#define CALL_USED_REGISTERS \
{                       \
  1,1,1,1,1,1,1,1,      \
  0,0,0,0,0,0,1,1,      \
  1,1,1,        \
}

/* we can't really copy to/from the CC */
#define AVOID_CCMODE_COPIES 1

/* The number of argument registers available */
#define NUM_ARG_REGS 6

/* The register number of the stack pointer register, which must also
   be a fixed register according to `FIXED_REGISTERS'.  */
#define STACK_POINTER_REGNUM PROP_SP_REGNUM

/* The register number of the frame pointer register, which is used to
   access automatic variables in the stack frame.  */
#define FRAME_POINTER_REGNUM PROP_FP_REGNUM

/* The register number of the arg pointer register, which is used to
   access the function's argument list.  */
#define ARG_POINTER_REGNUM PROP_FP_REGNUM


/* Definitions for register eliminations.

   This is an array of structures.  Each structure initializes one pair
   of eliminable registers.  The "from" register number is given first,
   followed by "to".  Eliminations of the same "from" register are listed
   in order of preference.
*/

#define ELIMINABLE_REGS							\
{{ FRAME_POINTER_REGNUM, STACK_POINTER_REGNUM },                        \
 { ARG_POINTER_REGNUM, STACK_POINTER_REGNUM },                          \
}



/* This macro is similar to `INITIAL_FRAME_POINTER_OFFSET'.  It
   specifies the initial difference between the specified pair of
   registers.  This macro must be defined if `ELIMINABLE_REGS' is
   defined.  */
#define INITIAL_ELIMINATION_OFFSET(FROM, TO, OFFSET)			\
  do {									\
    (OFFSET) = propeller_initial_elimination_offset ((FROM), (TO));		\
  } while (0)

/* A C expression that is nonzero if REGNO is the number of a hard
   register in which function arguments are sometimes passed.  */
#define FUNCTION_ARG_REGNO_P(r) (r >= 0 && r <= 5)

/* A macro whose definition is the name of the class to which a valid
   base register must belong.  A base register is one used in an
   address which is the register value plus a displacement.  */
#define BASE_REG_CLASS GENERAL_REGS

#define INDEX_REG_CLASS NO_REGS

#define HARD_REGNO_OK_FOR_BASE_P(NUM) \
  ((unsigned) (NUM) < FIRST_PSEUDO_REGISTER \
   && (REGNO_REG_CLASS(NUM) == GENERAL_REGS \
       || (NUM) == HARD_FRAME_POINTER_REGNUM))

#define MAX_REGS_PER_ADDRESS 1

/* A C expression which is nonzero if register number NUM is suitable
   for use as a base register in operand addresses.  */
#ifdef REG_OK_STRICT
#define REG_STRICT_P 1
#define REGNO_OK_FOR_BASE_P(NUM)		 \
  (HARD_REGNO_OK_FOR_BASE_P(NUM) 		 \
   || HARD_REGNO_OK_FOR_BASE_P(reg_renumber[(NUM)]))
#else
#define REG_STRICT_P 0
#define REGNO_OK_FOR_BASE_P(NUM)		 \
  ((NUM) >= FIRST_PSEUDO_REGISTER || HARD_REGNO_OK_FOR_BASE_P(NUM))
#endif

/* A C expression which is nonzero if register number NUM is suitable
   for use as an index register in operand addresses.  */
#define REGNO_OK_FOR_INDEX_P(NUM) 0

/* A C expression that is nonzero if it is permissible to store a
   value of mode MODE in hard register number REGNO (or in several
   registers starting with that one).  All gstore registers are 
   equivalent, so we can set this to 1.  */
#define HARD_REGNO_MODE_OK(R,M) 1

/* A C expression whose value is a register class containing hard
   register REGNO.  */
#define REGNO_REG_CLASS(R) ((R < PROP_PC_REGNUM) ? GENERAL_REGS :		\
                            (R == PROP_CC_REGNUM ? CC_REGS : SPECIAL_REGS))

/* A C expression for the number of consecutive hard registers,
   starting at register number REGNO, required to hold a value of mode
   MODE.  */
#define HARD_REGNO_NREGS(REGNO, MODE)			   \
  ((GET_MODE_SIZE (MODE) + UNITS_PER_WORD - 1)		   \
   / UNITS_PER_WORD)

/* A C expression that is nonzero if a value of mode MODE1 is
   accessible in mode MODE2 without copying.  */
#define MODES_TIEABLE_P(MODE1, MODE2) 1

/* A C expression for the maximum number of consecutive registers of
   class CLASS needed to hold a value of mode MODE.  */
#define CLASS_MAX_NREGS(CLASS, MODE) \
  ((GET_MODE_SIZE (MODE) + UNITS_PER_WORD - 1) / UNITS_PER_WORD)

/* The maximum number of bytes that a single instruction can move
   quickly between memory and registers or between two memory
   locations.  */
#define MOVE_MAX 4
#define TRULY_NOOP_TRUNCATION(op,ip) 1

/* All load operations zero extend.  */
#define LOAD_EXTEND_OP(MEM) ZERO_EXTEND

#define SELECT_CC_MODE(OP, X, Y) propeller_select_cc_mode(OP, X, Y)

#define LEGITIMATE_CONSTANT_P(X) propeller_legitimate_constant_p (X)

/* GO_IF_LEGITIMATE_ADDRESS recognizes an RTL expression
   that is a valid memory address for an instruction.
   The MODE argument is the machine mode for the MEM expression
   that wants to use this address.  */
#define GO_IF_LEGITIMATE_ADDRESS(MODE, X, WIN) \
  { \
    if (propeller_legitimate_address_p (MODE, X, REG_STRICT_P)) goto WIN; \
  }

/* Passing Arguments in Registers */

/* A C type for declaring a variable that is used as the first
   argument of `FUNCTION_ARG' and other related values.  */
typedef unsigned int CUMULATIVE_ARGS;

#define FUNCTION_ARG_PADDING(MODE, TYPE) upward
#define BLOCK_REG_PADDING(MODE, TYPE, FIRST) upward

/* If defined, the maximum amount of space required for outgoing arguments
   will be computed and placed into the variable
   `current_function_outgoing_args_size'.  No space will be pushed
   onto the stack for each call; instead, the function prologue should
   increase the stack frame size by this amount.  */
#define ACCUMULATE_OUTGOING_ARGS 1

/* A C statement (sans semicolon) for initializing the variable CUM
   for the state at the beginning of the argument list.  
   For moxie, the first arg is passed in register 2 (aka $r0).  */
#define INIT_CUMULATIVE_ARGS(CUM,FNTYPE,LIBNAME,FNDECL,N_NAMED_ARGS) \
    (CUM = PROP_R0)

/* How Scalar Function Values Are Returned */

/* STACK AND CALLING */

/* Define this macro if pushing a word onto the stack moves the stack
   pointer to a smaller address.  */
#define STACK_GROWS_DOWNWARD

#define INITIAL_FRAME_POINTER_OFFSET(DEPTH) (DEPTH) = 0

/* Offset from the frame pointer to the first local variable slot to
   be allocated.  */
#define STARTING_FRAME_OFFSET (UNITS_PER_WORD)
#define STACK_POINTER_OFFSET  (UNITS_PER_WORD)

/* Define this if the above stack space is to be considered part of the
   space allocated by the caller.  */
#define OUTGOING_REG_PARM_STACK_SPACE(FNTYPE) 1
#define STACK_PARMS_IN_REG_PARM_AREA

/* Offset from the argument pointer register to the first argument's
   address.  On some machines it may depend on the data type of the
   function.  */
#define FIRST_PARM_OFFSET(F) (UNITS_PER_WORD)

/* A C expression whose value is RTL representing the location of the
   incoming return address at the beginning of any function, before
   the prologue.  */
#define INCOMING_RETURN_ADDR_RTX  gen_rtx_REG( SImode, PROP_LR_REGNUM)

/*
 * function results
 */
#define FUNCTION_VALUE(VALTYPE, FUNC)                                   \
   gen_rtx_REG ((INTEGRAL_TYPE_P (VALTYPE)                              \
                 && TYPE_PRECISION (VALTYPE) < BITS_PER_WORD)           \
	            ? word_mode                                         \
	            : TYPE_MODE (VALTYPE),				\
	            PROP_R0)

#define LIBCALL_VALUE(MODE) gen_rtx_REG (MODE, PROP_R0)

#define FUNCTION_VALUE_REGNO_P(N) ((N) == PROP_R0)

#define DEFAULT_PCC_STRUCT_RETURN 0

/*
 * the overal assembler file framework
 */
#undef  ASM_SPEC
#define ASM_COMMENT_START "\'"
#define ASM_APP_ON ""
#define ASM_APP_OFF ""

#undef  ASM_GENERATE_INTERNAL_LABEL
#define ASM_GENERATE_INTERNAL_LABEL(LABEL, PREFIX, NUM)		\
  do								\
    {								\
      sprintf (LABEL, ":%s%u", \
	       PREFIX, (unsigned) (NUM));			\
    }								\
  while (0)

#define FILE_ASM_OP     ""

/* Switch to the text or data segment.  */
#define TEXT_SECTION_ASM_OP  "\'\t.text"
#define DATA_SECTION_ASM_OP  "\'\t.data"
#define BSS_SECTION_ASM_OP   "\'\t.bss"

/* Assembler Commands for Alignment */

#define ASM_OUTPUT_ALIGN(STREAM,POWER) \
	fprintf (STREAM, "'\t.align\t%u\n", (1U<<POWER));

/* This says how to output an assembler line
   to define a global common symbol.  */

#define ASM_OUTPUT_COMMON(FILE, NAME, SIZE, ROUNDED)	\
  ( fputs (".comm ", (FILE)),				\
    assemble_name ((FILE), (NAME)),			\
    fprintf ((FILE), ",%u\n", (int)(ROUNDED)))

/* This says how to output an assembler line
   to define a local common symbol....  */
#undef  ASM_OUTPUT_LOCAL
#define ASM_OUTPUT_LOCAL(FILE, NAME, SIZE, ROUNDED)	\
  (fputs ("\t.lcomm\t", FILE),				\
  assemble_name (FILE, NAME),				\
  fprintf (FILE, ",%d\n", (int)SIZE))


/* ... and how to define a local common symbol whose alignment
   we wish to specify.  ALIGN comes in as bits, we have to turn
   it into bytes.  */
#undef  ASM_OUTPUT_ALIGNED_LOCAL
#define ASM_OUTPUT_ALIGNED_LOCAL(FILE, NAME, SIZE, ALIGN)		\
  do									\
    {									\
      fputs ("\t.bss\t", (FILE));					\
      assemble_name ((FILE), (NAME));					\
      fprintf ((FILE), ",%d,%d\n", (int)(SIZE), (ALIGN) / BITS_PER_UNIT);\
    }									\
  while (0)

/* This is how to output an assembler line
   that says to advance the location counter by SIZE bytes.  */
#undef  ASM_OUTPUT_SKIP
#define ASM_OUTPUT_SKIP(FILE,SIZE)  \
  fprintf (FILE, "\tbyte 0[%d]\n", (int)(SIZE))


/* Output and Generation of Labels */

#define GLOBAL_ASM_OP "\'\t.global\t"


#endif /* GCC_PROPELLER_H */
