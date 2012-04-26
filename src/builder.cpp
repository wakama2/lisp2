#include "lisp.h"

CodeBuilder::CodeBuilder(Context *ctx, Func *func) {
	this->ctx = ctx;
	this->func = func;
	ci = 0;
}

void CodeBuilder::addInst(int inst) {
	if(ctx->flagShowIR) {
		printf("%03d: %s\n", ci, ctx->getInstName(inst));
	}
#ifdef USING_THCODE
	code[ci++].ptr = ctx->getDTLabel(inst);
#else
	code[ci++].i = inst;
#endif
}

#define addInt(n)     code[ci++].i = n;
#define addFunc(func) code[ci++].func = func;
#define addVar(var)   code[ci++].var = var;

void CodeBuilder::createIConst(int r, int i) {
	addInst(INS_ICONST);
	addInt(r);
	addInt(i);
}

void CodeBuilder::createMov(int r, int n) {
	addInst(INS_MOV);
	addInt(r);
	addInt(n);
}

void CodeBuilder::createOp(int inst, int r, int a) {
	addInst(inst);
	addInt(r);
	addInt(a);
}

int CodeBuilder::createCondOp(int inst, int a, int b) {
	int lb = ci;
	addInst(inst);
	addInt(0);
	addInt(a);
	addInt(b);
	return lb;
}

int CodeBuilder::createJmp() {
	int lb = ci;
	addInst(INS_JMP);
	addInt(0);
	return lb;
}

void CodeBuilder::createINeg(int r) {
	addInst(INS_INEG);
	addInt(r);
}

void CodeBuilder::createLoadGlobal(Variable *var, int r) {
	addInst(INS_LOAD_GLOBAL);
	addVar(var);
	addInt(r);
}

void CodeBuilder::createStoreGlobal(Variable *var, int r) {
	addInst(INS_STORE_GLOBAL);
	addVar(var);
	addInt(r);
}

void CodeBuilder::createCall(Func *func, int shift, int rix) {
	addInst(INS_CALL);
	addFunc(func);
	addInt(shift);
	addInt(rix);
}

void CodeBuilder::createSpawn(Func *func, int shift, int rix) {
	addInst(INS_SPAWN);
	addFunc(func);
	addInt(shift);
	addInt(rix);
}

void CodeBuilder::createJoin(int n) {
	addInst(INS_JOIN);
	addInt(n);
}

void CodeBuilder::createRet() { 
	if(func != NULL && func->argc != 0) { createMov(0, func->argc); }
	addInst(INS_RET);
}

void CodeBuilder::createEnd() {
	addInst(INS_END);
}

void CodeBuilder::setLabel(int n) {
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

