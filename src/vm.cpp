#include "lisp.h"

void vmrun(WorkerThread *wth) {
	if(wth == NULL) {
#define I(a) jmptable[a] = &&L_##a;
#include "inst"
#undef I
		return;
	}

	register Code *pc = wth->pc;
	register int *sfp = wth->stack;
	register Frame *fp = wth->frame;
	goto *(pc->ptr);

L_INS_ICONST:
	sfp[pc[1].i] = pc[2].i;
	goto *((pc += 3)->ptr);
	
L_INS_IMOV:
	sfp[pc[1].i] = sfp[pc[2].i];
	goto *((pc += 3)->ptr);

L_INS_IADD:
	sfp[pc[1].i] += sfp[pc[2].i];
	goto *((pc += 3)->ptr);

L_INS_ISUB:
	sfp[pc[1].i] -= sfp[pc[2].i];
	goto *((pc += 3)->ptr);

L_INS_IMUL:
	sfp[pc[1].i] *= sfp[pc[2].i];
	goto *((pc += 3)->ptr);

L_INS_IDIV:
	sfp[pc[1].i] /= sfp[pc[2].i];
	goto *((pc += 3)->ptr);

L_INS_IMOD:
	sfp[pc[1].i] %= sfp[pc[2].i];
	goto *((pc += 3)->ptr);

L_INS_INEG:
	sfp[pc[1].i] = -sfp[pc[1].i];
	goto *((pc += 2)->ptr);

L_INS_IJMPLT:
	pc += (sfp[pc[2].i] < sfp[pc[3].i]) ? pc[1].i : 4;
	goto *(pc->ptr);

L_INS_IJMPLE:
	pc += (sfp[pc[2].i] <= sfp[pc[3].i]) ? pc[1].i : 4;
	goto *(pc->ptr);

L_INS_IJMPGT:
	pc += (sfp[pc[2].i] > sfp[pc[3].i]) ? pc[1].i : 4;
	goto *(pc->ptr);

L_INS_IJMPGE:
	pc += (sfp[pc[2].i] >= sfp[pc[3].i]) ? pc[1].i : 4;
	goto *(pc->ptr);

L_INS_IJMPEQ:
	pc += (sfp[pc[2].i] == sfp[pc[3].i]) ? pc[1].i : 4;
	goto *(pc->ptr);

L_INS_IJMPNE:
	pc += (sfp[pc[2].i] != sfp[pc[3].i]) ? pc[1].i : 4;
	goto *(pc->ptr);

L_INS_JMP:
	pc += pc[1].i;
	goto *(pc->ptr);

L_INS_CALL:
	fp->sfp = sfp;
	fp->pc = pc + 4;
	fp->rix = pc[3].i;
	fp++;
	sfp += pc[2].i;
	pc = pc[1].func->code;
	goto *(pc->ptr);

L_INS_RET: {
		fp--;
		int rval = sfp[0];
		pc = fp->pc;
		sfp = fp->sfp;
		sfp[fp->rix] = rval;
	}
	goto *(pc->ptr);

L_INS_IPRINT:
	printf("%d\n", sfp[pc[1].i]);
	goto *((pc += 2)->ptr);

L_INS_TNILPRINT:
	printf("%s\n", sfp[pc[1].i] ? "T" : "Nil");
	goto *((pc += 2)->ptr);

L_INS_EXIT:
	return;
}

