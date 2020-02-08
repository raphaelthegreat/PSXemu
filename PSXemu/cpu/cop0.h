#pragma once
#include <cstdint>

union Cop0STAT {
	uint32_t raw;
	struct {
		uint32_t IEc : 1;		/* Interrupt Enable (current) */
		uint32_t KUc : 1;		/* Kernel-User Mode (current) */
		uint32_t IEp : 1;		/* Interrupt Enable (previous) */
		uint32_t KUp : 1;		/* Kernel-User Mode (previous) */
		uint32_t IEo : 1;		/* Interrupt Enable (old) */
		uint32_t KUo : 1;		/* Kernel-User Mode (old) */
		uint32_t reserved1 : 2;
		uint32_t Sw : 2;		/* Software Interrupt Mask */
		uint32_t Intr : 6;	/* Hardware Interrupt Mask */
		uint32_t IsC : 1;		/* Isolate Cache */
		uint32_t reserved2 : 1;
		uint32_t PZ : 1;		/* Parity Zero */
		uint32_t reserved3 : 1;
		uint32_t PE : 1;		/* Parity Error */
		uint32_t TS : 1;		/* TLB Shutdown */
		uint32_t BEV : 1;		/* Bootstrap Exception Vectors */
		uint32_t reserved4 : 5;
		uint32_t Cu : 4;		/* Coprocessor Usability */
	};
};

union Cop0CAUSE {
	uint32_t raw;
	
	struct {
		uint32_t reserved1 : 2;
		uint32_t exc_code : 5;	/* Exception Code */
		uint32_t reserved2 : 1;
		uint32_t Sw : 2;		/* Software Interrupts */
		uint32_t IP : 6;		/* Interrupt Pending */
		uint32_t reserved3 : 12;
		uint32_t CE : 2;		/* Coprocessor Error */
		uint32_t BT : 1;		/* Branch Taken */
		uint32_t BD : 1;		/* Branch Delay */
	};
};

union Cop0 {
	uint32_t regs[64];

	struct {
		uint32_t r0;
		uint32_t r1;
		uint32_t r2;
		uint32_t BPC;		/* Breakpoint Program Counter */
		uint32_t r4;
		uint32_t BDA;		/* Breakpoint Data Address */
		uint32_t TAR;		/* Target Address */
		uint32_t DCIC;		/* Debug and Cache Invalidate Control */
		uint32_t BadA;		/* Bad Address */
		uint32_t BDAM;		/* Breakpoint Data Address Mask */
		uint32_t r10;
		uint32_t BPCM;		/* Breakpoint Program Counter Mask */
		Cop0STAT sr;	/* Status */
		Cop0CAUSE cause;	/* Cause */
		uint32_t epc;		/* Exception Program Counter */
		uint32_t PRId;		/* Processor Revision Identifier */
		uint32_t reserved[32];
	};
};