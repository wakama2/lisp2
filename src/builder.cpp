#include "lisp.h"

CodeBuilder::CodeBuilder(Context *ctx, Func *func, bool genthc) {
	this->ctx = ctx;
	this->func = func;
	this->genthc = genthc;
	if(ctx->flagShowIR) {
		printf("//----------------------------//\n");
		if(func != NULL) {
			printf("* defun %s argc=%d\n", func->name, func->argc);
		}
	}
}

#define ADD(f, v) { \
		Code c; \
		c.f = (v); \
		codebuf.add(c); \
	}

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

#define ci codebuf.getSize()
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

int CodeBuilder::createCondOpC(int inst, int a, int b) {
	int lb = ci;
	if(ctx->flagShowIR) {
		printf("%03d: %s\t[%d] %d L%d\n", ci, ctx->getInstName(inst), a, b, lb);
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
	codebuf[n+1].i = ci - n;
}

Code *CodeBuilder::getCode() {
	return codebuf.toArray();
}



