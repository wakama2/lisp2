#include "lisp.h"

void vmrun(Context *ctx, WorkerThread *wth, Task *task) {
	if(wth == NULL) {
#define I(a) ctx->jmptable[a] = &&L_##a;
#include "inst"
#undef I
		return;
	}

	register Code *pc  = task->pc;
	register Value *sp = task->sp;
	goto *(pc->ptr);

L_INS_ICONST:
	sp[pc[1].i].i = pc[2].i;
	goto *((pc += 3)->ptr);
	
L_INS_MOV:
	sp[pc[1].i] = sp[pc[2].i];
	goto *((pc += 3)->ptr);

L_INS_IADD:
	sp[pc[1].i].i += sp[pc[2].i].i;
	goto *((pc += 3)->ptr);

L_INS_ISUB:
	sp[pc[1].i].i -= sp[pc[2].i].i;
	goto *((pc += 3)->ptr);

L_INS_IMUL:
	sp[pc[1].i].i *= sp[pc[2].i].i;
	goto *((pc += 3)->ptr);

L_INS_IDIV:
	sp[pc[1].i].i /= sp[pc[2].i].i;
	goto *((pc += 3)->ptr);

L_INS_IMOD:
	sp[pc[1].i].i %= sp[pc[2].i].i;
	goto *((pc += 3)->ptr);

L_INS_INEG:
	sp[pc[1].i].i = -sp[pc[1].i].i;
	goto *((pc += 2)->ptr);

L_INS_IJMPLT:
	pc += (sp[pc[2].i].i < sp[pc[3].i].i) ? pc[1].i : 4;
	goto *(pc->ptr);

L_INS_IJMPLE:
	pc += (sp[pc[2].i].i <= sp[pc[3].i].i) ? pc[1].i : 4;
	goto *(pc->ptr);

L_INS_IJMPGT:
	pc += (sp[pc[2].i].i > sp[pc[3].i].i) ? pc[1].i : 4;
	goto *(pc->ptr);

L_INS_IJMPGE:
	pc += (sp[pc[2].i].i >= sp[pc[3].i].i) ? pc[1].i : 4;
	goto *(pc->ptr);

L_INS_IJMPEQ:
	pc += (sp[pc[2].i].i == sp[pc[3].i].i) ? pc[1].i : 4;
	goto *(pc->ptr);

L_INS_IJMPNE:
	pc += (sp[pc[2].i].i != sp[pc[3].i].i) ? pc[1].i : 4;
	goto *(pc->ptr);

L_INS_JMP:
	pc += pc[1].i;
	goto *(pc->ptr);

L_INS_CALL:
	Value *sp2 = sp;
	sp += pc[2].i;
	sp[-2].sp = sp2;
	sp[-1].pc = pc + 4; // rix
	pc = pc[1].func->code;
	goto *(pc->ptr);

L_INS_SPAWN: {
	Task *task = sche->newTask();
	sp[pc[3].i].task = task;
	if(task == NULL) {
		// INS_CALL
		goto L_INS_CALL;
	} else {
		// spawn
		sche->enqueue(task);
		goto *((pc += 4)->ptr);
	}

L_INS_JOIN:
	Task *task = sp[pc[1].i].task;
	if(task != NULL) {
		if(!task->isEnd()) {
			return; // TODO
		} else {
			sp[pc[2].i].i = task->get();
			sche->deleteTask(sp[1].task);
		}
	}
	goto *((pc += 3)->ptr);

L_INS_RET:
	if(sp != t->stack) {
		pc = sp[-1].pc;
		sp = sp[-2].sp;
		goto *(pc->ptr);
	} else {
		t->stat = TASK_END;
		// call task destructor
		return;
	}

L_INS_IPRINT:
	printf("%d\n", sp[pc[1].i].i);
	goto *((pc += 2)->ptr);

L_INS_TNILPRINT:
	printf("%s\n", sp[pc[1].i].i ? "T" : "Nil");
	goto *((pc += 2)->ptr);

L_INS_EXIT:
	wth->pc = pc;
	wth->sp = sp;
	wth->fp = fp;
	return;
}

