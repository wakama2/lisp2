#include "lisp.h"

#ifdef USING_THCODE
# define SWITCHBEGIN goto *(pc->ptr)
# define SWITCHEND 
# define CASE(a)     L_##a:
# define NEXT()      goto *(pc->ptr)
# define DEFAULT		 L_ERROR:
#else
# define SWITCHBEGIN L_HEAD: switch(pc->i) {
# define SWITCHEND   }
# define CASE(a)     case INS_##a:
# define NEXT()      goto L_HEAD
# define DEFAULT		 default:
#endif

void vmrun(Context *ctx, WorkerThread *wth, Task *task) {
#ifdef USING_THCODE
	if(wth == NULL) {
#define I(a) ctx->jmptable[INS_##a] = &&L_##a;
#include "inst"
#undef I
		return;
	}
#endif
	register Code *pc  = task->pc;
	register Value *sp = task->sp;
	Scheduler *sche = wth->sche;

	SWITCHBEGIN;

	CASE(ICONST) {
		sp[pc[1].i].i = pc[2].i;
		pc += 3;
	} NEXT();

	CASE(MOV) {
		sp[pc[1].i] = sp[pc[2].i];
		pc += 3;
	} NEXT();

#define CASE_IOP(ins, op) \
	CASE(ins) { \
		sp[pc[1].i].i op sp[pc[2].i].i;\
		pc += 3; \
	} NEXT();
		
	CASE_IOP(IADD, +=);
	CASE_IOP(ISUB, -=);
	CASE_IOP(IMUL, *=);
	CASE_IOP(IDIV, /=);
	CASE_IOP(IMOD, %=);

#define CASE_IOPC(ins, op) \
	CASE(ins) { \
		sp[pc[1].i].i op pc[2].i;\
		pc += 3; \
	} NEXT();
		
	CASE_IOPC(IADDC, +=);
	CASE_IOPC(ISUBC, -=);
	CASE_IOPC(IMULC, *=);
	CASE_IOPC(IDIVC, /=);
	CASE_IOPC(IMODC, %=);
	
	CASE(INEG) {
		sp[pc[1].i].i = -sp[pc[1].i].i;
		pc += 2;
	} NEXT();

#define CASE_IJMPOP(ins, op) \
	CASE(ins) { \
		pc += (sp[pc[2].i].i op sp[pc[3].i].i) ? pc[1].i : 4; \
	} NEXT();
		
	CASE_IJMPOP(IJMPLT, <);
	CASE_IJMPOP(IJMPLE, <=);
	CASE_IJMPOP(IJMPGT, >);
	CASE_IJMPOP(IJMPGE, >=);
	CASE_IJMPOP(IJMPEQ, ==);
	CASE_IJMPOP(IJMPNE, !=);

#define CASE_IJMPOPC(ins, op) \
	CASE(ins) { \
		pc += (sp[pc[2].i].i op pc[3].i) ? pc[1].i : 4; \
	} NEXT();
		
	CASE_IJMPOPC(IJMPLTC, <);
	CASE_IJMPOPC(IJMPLEC, <=);
	CASE_IJMPOPC(IJMPGTC, >);
	CASE_IJMPOPC(IJMPGEC, >=);
	CASE_IJMPOPC(IJMPEQC, ==);
	CASE_IJMPOPC(IJMPNEC, !=);

	CASE(JMP) {
		pc += pc[1].i;
	} NEXT();

	CASE(LOAD_GLOBAL) {
		sp[pc[1].i] = pc[2].var->value;
		pc += 3;
	} NEXT();

	CASE(STORE_GLOBAL) {
		pc[2].var->value = sp[pc[1].i];
		pc += 3;
	} NEXT();

	CASE(CALL) {
		Value *sp2 = sp;
		sp += pc[2].i;
		sp[-2].sp = sp2;
		sp[-1].pc = pc + 3;
#ifdef USING_THCODE
		pc = pc[1].func->thcode;
#else
		pc = pc[1].func->code;
#endif
	} NEXT();

	CASE(SPAWN) {
		if(unlikely(!sche->isTaskEmpty())) {
			Task *t = sche->newTask(pc[1].func, sp + pc[2].i);
			if(unlikely(t != NULL)) {
				// spawn
				sp[pc[2].i - 3].task = t;
				sche->enqueue(t);
				pc += 3;
				NEXT();
			}
		}
		sp[pc[2].i - 3].task = NULL;
		// CALL
		Value *sp2 = sp;
		sp += pc[2].i;
		sp[-2].sp = sp2;
		sp[-1].pc = pc + 3;
#ifdef USING_THCODE
		pc = pc[1].func->thcode;
#else
		pc = pc[1].func->code;
#endif
	} NEXT();

	CASE(JOIN) {
		int res = pc[1].i;
		Task *t = sp[res].task;
		if(t != NULL) {
			if(t->stat == TASK_RUN) {
				task->pc = pc;
				task->sp = sp;
				sche->enqueue(task);
				return;
			} else {
				sp[res] = t->stack[0];
				sche->deleteTask(t);
			}
		} else {
			sp[res] = sp[res + 1];
		}
		pc += 2;
	} NEXT();

	CASE(RET) {
		Value *sp2 = sp[-2].sp;
		sp[-2] = sp[pc[1].i];
		pc = sp[-1].pc;
		sp = sp2;
	} NEXT();

	CASE(RETC) {
		Value *sp2 = sp[-2].sp;
		sp[-2].i = pc[1].i;
		pc = sp[-1].pc;
		sp = sp2;
	} NEXT();

	CASE(IPRINT) {
		fprintf(stdout, "%ld\n", (long int)sp[pc[1].i].i);
		pc += 2;
	} NEXT();

	CASE(FPRINT) {
		fprintf(stdout, "%lf\n", sp[pc[1].i].f);
		pc += 2;
	} NEXT();
	
	CASE(BPRINT) {
		fprintf(stdout, "%s\n", sp[pc[1].i].i ? "T" : "NIL");
		pc += 2;
	} NEXT();

	CASE(DEFUN) {
		defun(ctx, pc[1].cons);
		pc += 2;
	} NEXT();

	CASE(END) {
		task->stat = TASK_END;
		return;
	}

#ifndef USING_THCODE
	DEFAULT {
		fprintf(stderr, "Error instruction!\n");
		exit(1);
	};
#endif

	SWITCHEND;
}

