#include "lisp.h"

CodeBuilder::CodeBuilder(Context *ctx, Func *func) {
	this->ctx = ctx;
	this->func = func;
	ci = 0;
}

void CodeBuilder::addInst(int inst) {
	//printf("%03d: %s\n", ci, ctx->getInstName(inst));
#ifdef USING_THCODE
	code[ci++].ptr = ctx->getDTLabel(inst);
#else
	code[ci++].i = inst;
#endif
}

void CodeBuilder::addInt(int n) {
	//printf("%d\n", n);
	code[ci++].i = n;
}

void CodeBuilder::addFunc(Func *func) {
	code[ci++].func = func;
}

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

void CodeBuilder::setLabel(int n) {
	code[n + 1].i = ci - n;
}

void CodeBuilder::accept(Func *func) {
	Code *c = new Code[ci];
	memcpy(c, code, sizeof(Code) * ci);
	func->code = c;
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

