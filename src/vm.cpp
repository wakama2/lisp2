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

static void do_nothing(Task *task, WorkerThread *wth) {}

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
		sp[pc[2].i] = pc[1].var->value;
		pc += 3;
	} NEXT();

	CASE(STORE_GLOBAL) {
		pc[1].var->value = sp[pc[2].i];
		pc += 3;
	} NEXT();

	CASE(CALL) {
		register Value *sp2 = sp;
		sp += pc[2].i;
		sp[-2].sp = sp2;
		sp[-1].pc = pc + 3;
		pc = pc[1].func->code;
	} NEXT();

	CASE(SPAWN) {
		Scheduler *sche = wth->sche;
		Task *t = sche->newTask(pc[1].func, sp + pc[2].i, do_nothing);
		sp[pc[2].i - 3].task = t;
		if(t == NULL) {
			// CALL
			register Value *sp2 = sp;
			sp += pc[2].i;
			sp[-2].sp = sp2;
			sp[-1].pc = pc + 3;
			pc = pc[1].func->code;
		} else {
			// spawn
			sche->enqueue(t);
			pc += 3;
		}
	} NEXT();

	CASE(JOIN) {
		Scheduler *sche = wth->sche;
		register int res = pc[1].i;
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
		if(sp != task->stack) {
			register Value *sp2 = sp[-2].sp;
			sp[-2] = sp[pc[1].i];
			pc = sp[-1].pc;
			sp = sp2;
		} else {
			sp[0] = sp[pc[1].i];
			task->stat = TASK_END;
			task->dest(task, wth);
			return;
		}
	} NEXT();

	CASE(RETC) {
		if(sp != task->stack) {
			register Value *sp2 = sp[-2].sp;
			sp[-2].i = pc[1].i;
			pc = sp[-1].pc;
			sp = sp2;
		} else {
			sp[0].i = pc[1].i;
			task->stat = TASK_END;
			task->dest(task, wth);
			return;
		}
	} NEXT();

	CASE(IPRINT) {
		fprintf(stdout, "%ld\n", sp[pc[1].i].i);
		pc += 2;
	} NEXT();

	CASE(BPRINT) {
		fprintf(stdout, "%s\n", sp[pc[1].i].i ? "T" : "Nil");
		pc += 2;
	} NEXT();

	CASE(END) {

	}

#ifndef USING_THCODE
	DEFAULT {
		fprintf(stderr, "Error instruction!\n");
		exit(1);
	};
#endif

	SWITCHEND;
}

