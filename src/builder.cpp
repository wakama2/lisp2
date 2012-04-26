#include "lisp.h"

CodeBuilder::CodeBuilder(Context *ctx, Func *func, bool genthc) {
	this->ctx = ctx;
	this->func = func;
	this->genthc = genthc;
	ci = 0;
}

#define ADD(f, v) code[ci++].f = (v)

#ifdef USING_THCODE
# define ADDINS(n) { \
		if(genthc) { \
			ADD(ptr, ctx->getDTLabel(n)); \
		} else { \
			ADD(i, n); \
		} \
	}
#else
# define ADDINS(n) ADD(i, n)
#endif

void CodeBuilder::createIns(int ins) {
	if(ctx->flagShowIR) {
		printf("%03d: %s\n", ci, ctx->getInstName(ins));
	}
	ADDINS(ins);
}

void CodeBuilder::createIntIns(int ins, int reg, int ival) {
	if(ctx->flagShowIR) {
		printf("%03d: %s\t[%d] %d\n", ci, ctx->getInstName(ins), reg, ival);
	}
	ADDINS(ins);
	ADD(i, reg);
	ADD(i, ival);
}

void CodeBuilder::createRegIns(int ins, int reg) {
	if(ctx->flagShowIR) {
		printf("%03d: %s\t[%d]\n", ci, ctx->getInstName(ins), reg);
	}
	ADDINS(ins);
	ADD(i, reg);
}

void CodeBuilder::createReg2Ins(int ins, int reg, int reg2) {
	if(ctx->flagShowIR) {
		printf("%03d: %s\t[%d] [%d]\n", ci, ctx->getInstName(ins), reg, reg2);
	}
	ADDINS(ins);
	ADD(i, reg);
	ADD(i, reg2);
}

void CodeBuilder::createVarIns(int ins, int reg, Variable *var) {
	if(ctx->flagShowIR) {
		printf("%03d: %s\t[%d] %s\n", ci, ctx->getInstName(ins), reg, var->name);
	}
	ADDINS(ins);
	ADD(var, var);
	ADD(i, reg);
}

void CodeBuilder::createFuncIns(int ins, Func *func, int sftsfp) {
	if(ctx->flagShowIR) {
		printf("%03d: %s\t%s %d\n", ci, ctx->getInstName(ins), func->name, sftsfp);
	}
	ADDINS(ins);
	ADD(func, func);
	ADD(i, sftsfp);
}

int CodeBuilder::createCondOp(int inst, int a, int b) {
	int lb = ci;
	if(ctx->flagShowIR) {
		printf("%03d: %s\t[%d] [%d] L%d\n", ci, ctx->getInstName(inst), a, b, lb);
	}
	ADDINS(inst);
	ADD(i, 0);
	ADD(i, a);
	ADD(i, b);
	return lb;
}

int CodeBuilder::createJmp() {
	int lb = ci;
	if(ctx->flagShowIR) {
		printf("%03d: %s\tL%d\n", ci, ctx->getInstName(INS_JMP), lb);
	}
	ADDINS(INS_JMP);
	ADD(i, 0);
	return lb;
}

void CodeBuilder::setLabel(int n) {
	if(ctx->flagShowIR) {
		printf("L%d:\n", n);
	}
	code[n + 1].i = ci - n;
}

Code *CodeBuilder::getCode() {
	Code *c = new Code[ci];
	memcpy(c, code, sizeof(Code) * ci);
	return c;
}

void CodeBuilder::codegen(Cons *cons, int sp) {
	if(cons->type == CONS_CAR) {
		codegen(cons->car, sp);
	} else {
		assert(cons->type == CONS_STR);
		const char *name = cons->str;
		Cons *args = cons->cdr;
		Func *func = ctx->getFunc(name);
		assert(func != NULL);
		func->codegen(func, args, this, sp);
	}
}

