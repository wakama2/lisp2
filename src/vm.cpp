#include "lisp.h"

void vmrun(WorkerThread *wth) {
	if(wth == NULL) {
		jmptable[INS_ICONST] = &&L_ICONST;
		jmptable[INS_IMOV] = &&L_IMOV;
		jmptable[INS_IADD] = &&L_IADD;
		jmptable[INS_ISUB] = &&L_ISUB;
		jmptable[INS_IMUL] = &&L_IMUL;
		jmptable[INS_IDIV] = &&L_IDIV;
		jmptable[INS_IMOD] = &&L_IMOD;
		jmptable[INS_INEG] = &&L_INEG;
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

	register Code *pc = wth->pc;
	register int *sfp = wth->stack;
	register Frame *fp = wth->frame;

	goto *(pc->ptr);

L_ICONST:
	sfp[pc[1].i] = pc[2].i;
	goto *((pc += 3)->ptr);
	
L_IMOV:
	sfp[pc[1].i] = sfp[pc[2].i];
	goto *((pc += 3)->ptr);

L_IADD:
	sfp[pc[1].i] += sfp[pc[2].i];
	goto *((pc += 3)->ptr);

L_ISUB:
	sfp[pc[1].i] -= sfp[pc[2].i];
	goto *((pc += 3)->ptr);

L_IMUL:
	sfp[pc[1].i] *= sfp[pc[2].i];
	goto *((pc += 3)->ptr);

L_IDIV:
	sfp[pc[1].i] /= sfp[pc[2].i];
	goto *((pc += 3)->ptr);

L_IMOD:
	sfp[pc[1].i] %= sfp[pc[2].i];
	goto *((pc += 3)->ptr);

L_INEG:
	sfp[pc[1].i] = -sfp[pc[1].i];
	goto *((pc += 2)->ptr);

L_IJMPLT:
	pc += (sfp[pc[1].i] < sfp[pc[2].i]) ? pc[3].i : 4;
	goto *(pc->ptr);

L_IJMPLE:
	pc += (sfp[pc[1].i] <= sfp[pc[2].i]) ? pc[3].i : 4;
	goto *(pc->ptr);

L_IJMPGT:
	pc += (sfp[pc[1].i] > sfp[pc[2].i]) ? pc[3].i : 4;
	goto *(pc->ptr);

L_IJMPGE:
	pc += (sfp[pc[1].i] >= sfp[pc[2].i]) ? pc[3].i : 4;
	goto *(pc->ptr);

L_IJMPEQ:
	pc += (sfp[pc[1].i] == sfp[pc[2].i]) ? pc[3].i : 4;
	goto *(pc->ptr);

L_IJMPNE:
	pc += (sfp[pc[1].i] != sfp[pc[2].i]) ? pc[3].i : 4;
	goto *(pc->ptr);

L_CALL:
	fp->sfp = sfp;
	fp->pc = pc + 4;
	fp->rix = pc[3].i;
	fp++;
	sfp += pc[2].i;
	pc = pc[1].func->code;
	goto *(pc->ptr);

L_RET: {
		fp--;
		int rval = sfp[0];
		pc = fp->pc;
		sfp = fp->sfp;
		sfp[fp->rix] = rval;
	}
	goto *(pc->ptr);

L_IPRING:
	printf("%d\n", sfp[pc[1].i]);
	goto *((pc += 2)->ptr);

L_TNILPRINT:
	printf("%s\n", sfp[pc[1].i] ? "T" : "Nil");
	goto *((pc += 2)->ptr);

L_EXIT:
	return;
}

