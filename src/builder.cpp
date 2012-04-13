#include "lisp.h"

CodeBuilder::CodeBuilder(Context *ctx, Func *func) {
	this->ctx = ctx;
	this->func = func;
	sp = func != NULL ? func->argc : 0;
	ci = 0;
	for(int i=0; i<256; i++) stype[i] = -1;
	for(int i=0; i<sp; i++) stype[i] = TYPE_INT;
}

void CodeBuilder::addInst(int inst) {
	printf("%03d: %s\n", ci, ctx->getInstName(inst));
	code[ci++].ptr = ctx->getDTLabel(inst);
}

void CodeBuilder::addInt(int n) {
	code[ci++].i = n;
}

void CodeBuilder::addFunc(Func *func) {
	code[ci++].func = func;
}

void CodeBuilder::createIConst(int r, int i) {
	addInst(INS_ICONST);
	addInt(r);
	addInt(i);
	stype[r] = TYPE_INT;
}

void CodeBuilder::createMov(int r, int n) {
	addInst(INS_MOV);
	addInt(r);
	addInt(n);
	stype[r] = stype[n];
}
void CodeBuilder::createOp(int inst, int r, int a) {
	addInst(inst);
	addInt(r);
	addInt(a);
	assert(stype[r] == TYPE_INT);
	assert(stype[a] == TYPE_INT);
	stype[r] = TYPE_INT;
}
// return label
int createCondOp(int inst, int a, int b) {
	int lb = ci;
	addInst(inst);
	addInt(0);
	addInt(a);
	addInt(b);
	assert(stype[a] == TYPE_INT);
	assert(stype[b] == TYPE_INT);
	return lb;
}
int createJmp() {
	int lb = ci;
	addInst(INS_JMP);
	addInt(0);
	return lb;
}
void CodeBuilder::createINeg(int r) {
	addInst(INS_INEG);
	addInt(r);
	assert(stype[r] == TYPE_INT);
}
void CodeBuilder::createCall(Func *func, int shift, int rix) {
	addInst(INS_CALL);
	addFunc(func);
	addInt(shift);
	addInt(rix);
	stype[rix] = TYPE_INT;
}
void CodeBuilder::createSpawn(Func *func, int shift, int rix) {
	addInst(INS_SPAWN);
	addFunc(func);
	addInt(shift);
	addInt(rix);
	stype[rix] = TYPE_FUTURE;
}
void CodeBuilder::createJoin(int n) {
	addInst(INS_JOIN);
	addInt(n);
	assert(stype[n] == TYPE_FUTURE);
	stype[n] = TYPE_INT;
}
void CodeBuilder::createRet() { 
	if(func != NULL && func->argc != 0) { createMov(0, func->argc); }
	addInst(INS_RET);
}
void CodeBuilder::createExit() {
	if(func != NULL && func->argc != 0) { createMov(0, func->argc); }
	addInst(INS_EXIT);
}
void CodeBuilder::setLabel(int n) {
	code[n + 1].i = ci - n;
}
void CodeBuilder::accept(Func *func) {
	Code *c = new Code[ci];
	memcpy(c, code, sizeof(Code) * ci);
	func->code = c;
}



