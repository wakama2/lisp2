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
# define CASE(a)     case a:
# define NEXT()      goto L_HEAD
# define DEFAULT		 default:
#endif

static void do_nothing(Task *task, WorkerThread *wth) {}

void vmrun(Context *ctx, WorkerThread *wth, Task *task) {
#ifdef USING_THCODE
	if(wth == NULL) {
#define I(a) ctx->jmptable[a] = &&L_##a;
#include "inst"
#undef I
		return;
	}
#endif
	register Code *pc  = task->pc;
	register Value *sp = task->sp;

	SWITCHBEGIN;

	CASE(INS_ICONST) {
		sp[pc[1].i].i = pc[2].i;
		pc += 3;
	} NEXT();

	CASE(INS_MOV) {
		sp[pc[1].i] = sp[pc[2].i];
		pc += 3;
	} NEXT();

#define CASE_IOP(ins, op) \
	CASE(ins) { \
		sp[pc[1].i].i op sp[pc[2].i].i;\
		pc += 3; \
	} NEXT();
		
	CASE_IOP(INS_IADD, +=);
	CASE_IOP(INS_ISUB, -=);
	CASE_IOP(INS_IMUL, *=);
	CASE_IOP(INS_IDIV, /=);
	CASE_IOP(INS_IMOD, %=);

#define CASE_IOPC(ins, op) \
	CASE(ins) { \
		sp[pc[1].i].i op pc[2].i;\
		pc += 3; \
	} NEXT();
		
	CASE_IOPC(INS_IADDC, +=);
	CASE_IOPC(INS_ISUBC, -=);
	
	CASE(INS_INEG) {
		sp[pc[1].i].i = -sp[pc[1].i].i;
		pc += 2;
	} NEXT();

#define CASE_IJMPOP(ins, op) \
	CASE(ins) { \
		pc += (sp[pc[2].i].i op sp[pc[3].i].i) ? pc[1].i : 4; \
	} NEXT();
		
	CASE_IJMPOP(INS_IJMPLT, <);
	CASE_IJMPOP(INS_IJMPLE, <=);
	CASE_IJMPOP(INS_IJMPGT, >);
	CASE_IJMPOP(INS_IJMPGE, >=);
	CASE_IJMPOP(INS_IJMPEQ, ==);
	CASE_IJMPOP(INS_IJMPNE, !=);

	CASE(INS_JMP) {
		pc += pc[1].i;
	} NEXT();

	CASE(INS_LOAD_GLOBAL) {
		sp[pc[2].i] = pc[1].var->value;
		pc += 3;
	} NEXT();

	CASE(INS_STORE_GLOBAL) {
		pc[1].var->value = sp[pc[2].i];
		pc += 3;
	} NEXT();

	CASE(INS_CALL) {
		Value *sp2 = sp;
		sp += pc[2].i;
		sp[-2].sp = sp2;
		sp[-1].pc = pc + 4;
		pc = pc[1].func->code;
	} NEXT();

	CASE(INS_SPAWN) {
		Scheduler *sche = wth->sche;
		Task *t = sche->newTask(pc[1].func, sp + pc[2].i, do_nothing);
		sp[pc[2].i - 3].task = t;
		if(t == NULL) {
			// INS_CALL
			Value *sp2 = sp;
			sp += pc[2].i;
			sp[-2].sp = sp2;
			sp[-1].pc = pc + 4;
			pc = pc[1].func->code;
		} else {
			// spawn
			sche->enqueue(t);
			pc += 4;
		}
	} NEXT();

	CASE(INS_JOIN) {
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

	CASE(INS_RET) {
		if(sp != task->stack) {
			register Value *sp2 = sp[-2].sp;
			sp[-2] = sp[0];
			pc = sp[-1].pc;
			sp = sp2;
		} else {
			task->stat = TASK_END;
			task->dest(task, wth);
			return;
		}
	} NEXT();

	CASE(INS_IPRINT) {
		fprintf(stdout, "%ld\n", sp[pc[1].i].i);
		pc += 2;
	} NEXT();

	CASE(INS_TNILPRINT) {
		fprintf(stdout, "%s\n", sp[pc[1].i].i ? "T" : "Nil");
		pc += 2;
	} NEXT();

	CASE(INS_END) {

	}

#ifndef USING_THCODE
	DEFAULT {
		fprintf(stderr, "Error instruction!\n");
		exit(1);
	};
#endif

	SWITCHEND;
}

