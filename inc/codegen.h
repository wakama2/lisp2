#ifndef CODEGEN_H
#define CODEGEN_H

class CodeBuilder {
private:
	Context *ctx;
	Func *func;
	ArrayBuilder<Code> codebuf;
	bool genthc;
	bool showir;

public:
	CodeBuilder(Context *_ctx, Func *_func, bool genthc, bool showir);
	
	void createIns(int ins);
	void createIntIns(int ins, int n);
	void createRegIntIns(int ins, int reg, int ival);
	void createRegIns(int ins, int reg);
	void createReg2Ins(int ins, int reg, int reg2);
	void createVarIns(int ins, int reg, Variable *var);
	void createFuncIns(int ins, Func *func, int sftsfp);
	
	void createIConst(int r, int v) { createRegIntIns(INS_ICONST, r, v); }
	void createMov(int r, int r2) { createReg2Ins(INS_MOV, r, r2); }
	void createIAdd(int r, int r2) { createReg2Ins(INS_IADD, r, r2); }
	void createISub(int r, int r2) { createReg2Ins(INS_ISUB, r, r2); }
	void createIMul(int r, int r2) { createReg2Ins(INS_IMUL, r, r2); }
	void createIDiv(int r, int r2) { createReg2Ins(INS_IDIV, r, r2); }
	void createIMod(int r, int r2) { createReg2Ins(INS_IMOD, r, r2); }
	void createIAddC(int r, int v) { createRegIntIns(INS_IADDC, r, v); }
	void createISubC(int r, int v) { createRegIntIns(INS_ISUBC, r, v); }
	void createINeg(int r) { createRegIns(INS_INEG, r); }
	void createJoin(int r) { createRegIns(INS_JOIN, r); }
	void createRet(int r) { createRegIns(INS_RET, r); }
	void createRetC(int n) { createIntIns(INS_RETC, n); }
	void createEnd() { createIns(INS_END); }
	void createLoadGlobal(int reg, Variable *var) { createVarIns(INS_LOAD_GLOBAL, reg, var); }
	void createStoreGlobal(int reg, Variable *var) { createVarIns(INS_STORE_GLOBAL, reg, var); }
	void createPrintInt(int r) { createRegIns(INS_IPRINT, r); }
	void createPrintBoolean(int r) { createRegIns(INS_BPRINT, r); }
	void createCall(Func *func, int ss) { createFuncIns(INS_CALL, func, ss); }
	void createSpawn(Func *func, int ss) { createFuncIns(INS_SPAWN, func, ss); }
	int  createCondOp(int inst, int a, int b, int offset = 0);
	int  createCondOpC(int inst, int a, int b, int offset = 0);
	int  createJmp(int offset = 0);
	void setLabel(int n);
	Code *getCode();
	int   getCodeLength() { return codebuf.getSize(); }
	Context *getCtx() { return ctx; }
	Func *getFunc()   { return func; }
};

ValueType codegen(Cons *cons, CodeBuilder *cb, int sp, bool spawn = false);
void codeopt(Context *ctx, Func *func);

#endif

