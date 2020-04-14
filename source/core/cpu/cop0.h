#pragma once


union Cop0STAT {
	uint raw;
	struct {
		uint IEc : 1;		/* Interrupt Enable (current) */
		uint KUc : 1;		/* Kernel-User Mode (current) */
		uint IEp : 1;		/* Interrupt Enable (previous) */
		uint KUp : 1;		/* Kernel-User Mode (previous) */
		uint IEo : 1;		/* Interrupt Enable (old) */
		uint KUo : 1;		/* Kernel-User Mode (old) */
		uint reserved1 : 2;
		uint Sw : 2;		/* Software Interrupt Mask */
		uint Intr : 6;	/* Hardware Interrupt Mask */
		uint IsC : 1;		/* Isolate Cache */
		uint reserved2 : 1;
		uint PZ : 1;		/* Parity Zero */
		uint reserved3 : 1;
		uint PE : 1;		/* Parity Error */
		uint TS : 1;		/* TLB Shutdown */
		uint BEV : 1;		/* Bootstrap Exception Vectors */
		uint reserved4 : 5;
		uint Cu : 4;		/* Coprocessor Usability */
	};
};

union Cop0CAUSE {
	uint raw;
	
	struct {
		uint : 2;
		uint exc_code : 5;	/* Exception Code */
		uint : 1;
		uint Sw : 2;		/* Software Interrupts */
		uint IP : 6;		/* Interrupt Pending */
		uint : 12;
		uint CE : 2;		/* Coprocessor Error */
		uint BT : 1;		/* Branch Taken */
		uint BD : 1;		/* Branch Delay */
	};
};

union Cop0 {
	uint regs[64];

	struct {
		uint r0;
		uint r1;
		uint r2;
		uint Bpc;		/* Breakpoint Program Counter */
		uint r4;
		uint BDA;		/* Breakpoint Data Address */
		uint TAR;		/* Target Address */
		uint DCIC;		/* Debug and Cache Invalidate Control */
		uint BadA;		/* Bad Address */
		uint BDAM;		/* Breakpoint Data Address Mask */
		uint r10;
		uint BpcM;		/* Breakpoint Program Counter Mask */
		Cop0STAT sr;	    /* Status */
		Cop0CAUSE cause;	/* Cause */
		uint epc;		/* Exception Program Counter */
		uint PRId;		/* Processor Revision Identifier */
		uint reserved[32];
	};
};