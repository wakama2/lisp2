#include "lisp.h"

void vmrun(WorkerThread *wth) {
	if(wth == NULL) {
		jmptable[INS_IADD] = &&L_IADD;
		jmptable[INS_ISUB] = &&L_ISUB;
		jmptable[INS_IMUL] = &&L_IMUL;
		jmptable[INS_IDIV] = &&L_IDIV;
		jmptable[INS_IMOD] = &&L_IMOD;
		jmptable[INS_IJMPLT] = &&L_IJMPLT;
		jmptable[INS_IJMPLE] = &&L_IJMPLE;
		jmptable[INS_IJMPGT] = &&L_IJMPGT;
		jmptable[INS_IJMPGE] = &&L_IJMPGE;
		jmptable[INS_IJMPEQ] = &&L_IJMPEQ;
		jmptable[INS_IJMPNE] = &&L_IJMPNE;
		jmptable[INS_CALL] = &&L_CALL;
		jmptable[INS_RET] = &&L_RET;
		jmptable[INS_IPRINT] = &&L_IPRING;
		jmptable[INS_TNILPRINT] = &&L_TNILPRINT;
		jmptable[INS_EXIT] = &&L_EXIT;
		return;
	}

	register Code *pc;
	register int *sfp;

	goto *(pc->ptr);

L_IADD:
L_ISUB:
L_IMUL:
L_IDIV:
L_IMOD:
L_IJMPLT:
L_IJMPLE:
L_IJMPGT:
L_IJMPGE:
L_IJMPEQ:
L_IJMPNE:
L_CALL:
L_RET:
L_IPRING:
L_TNILPRINT:

L_EXIT:
	return;
}

