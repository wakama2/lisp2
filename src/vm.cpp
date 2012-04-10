#include "lisp.h"

void vmrun(WorkerThread *wth) {
	if(wth == NULL) {
#define I(a) jmptable[a] = &&L_##a;
#include "inst"
#undef I
		return;
	}

	register Code *pc = wth->pc;
	register int *sp = wth->sp;
	register Frame *fp = wth->fp;
	goto *(pc->ptr);

L_INS_ICONST:
	sp[pc[1].i] = pc[2].i;
	goto *((pc += 3)->ptr);
	
L_INS_IMOV:
	sp[pc[1].i] = sp[pc[2].i];
	goto *((pc += 3)->ptr);

L_INS_IADD:
	sp[pc[1].i] += sp[pc[2].i];
	goto *((pc += 3)->ptr);

L_INS_ISUB:
	sp[pc[1].i] -= sp[pc[2].i];
	goto *((pc += 3)->ptr);

L_INS_IMUL:
	sp[pc[1].i] *= sp[pc[2].i];
	goto *((pc += 3)->ptr);

L_INS_IDIV:
	sp[pc[1].i] /= sp[pc[2].i];
	goto *((pc += 3)->ptr);

L_INS_IMOD:
	sp[pc[1].i] %= sp[pc[2].i];
	goto *((pc += 3)->ptr);

L_INS_INEG:
	sp[pc[1].i] = -sp[pc[1].i];
	goto *((pc += 2)->ptr);

L_INS_IJMPLT:
	pc += (sp[pc[2].i] < sp[pc[3].i]) ? pc[1].i : 4;
	goto *(pc->ptr);

L_INS_IJMPLE:
	pc += (sp[pc[2].i] <= sp[pc[3].i]) ? pc[1].i : 4;
	goto *(pc->ptr);

L_INS_IJMPGT:
	pc += (sp[pc[2].i] > sp[pc[3].i]) ? pc[1].i : 4;
	goto *(pc->ptr);

L_INS_IJMPGE:
	pc += (sp[pc[2].i] >= sp[pc[3].i]) ? pc[1].i : 4;
	goto *(pc->ptr);

L_INS_IJMPEQ:
	pc += (sp[pc[2].i] == sp[pc[3].i]) ? pc[1].i : 4;
	goto *(pc->ptr);

L_INS_IJMPNE:
	pc += (sp[pc[2].i] != sp[pc[3].i]) ? pc[1].i : 4;
	goto *(pc->ptr);

L_INS_JMP:
	pc += pc[1].i;
	goto *(pc->ptr);

L_INS_CALL:
	fp->sp = sp;
	fp->pc = pc + 4;
	fp->rix = pc[3].i;
	fp++;
	sp += pc[2].i;
	pc = pc[1].func->code;
	goto *(pc->ptr);

L_INS_RET: {
		fp--;
		int rval = sp[0];
		pc = fp->pc;
		sp = fp->sp;
		sp[fp->rix] = rval;
	}
	goto *(pc->ptr);

L_INS_IPRINT:
	printf("%d\n", sp[pc[1].i]);
	goto *((pc += 2)->ptr);

L_INS_TNILPRINT:
	printf("%s\n", sp[pc[1].i] ? "T" : "Nil");
	goto *((pc += 2)->ptr);

L_INS_EXIT:
	wth->pc = pc;
	wth->sp = sp;
	wth->fp = fp;
	return;
}

