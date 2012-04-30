#include "lisp.h"

static ValueType genSpawn(Func *func, Cons *cons, CodeBuilder *cb, int sp);

static int getArgIndex(Func *func, const char *name) {
	for(int i=0; i<(int)func->argc; i++) {
		if(strcmp(name, func->args[i]) == 0) {
			return i;
		}
	}
	return -1;
}

ValueType codegen(Cons *cons, CodeBuilder *cb, int sp, bool spawn) {
	if(cons->type == CONS_INT) {
		cb->createIConst(sp, cons->i);
		return VT_INT;
	} else if(cons->type == CONS_STR) {
		const char *name = cons->str;
		if(strcmp(name, "t") == 0) {
			cb->createIConst(sp, 1);
			return VT_BOOLEAN;
		}
		if(strcmp(name, "nil") == 0) {
			cb->createIConst(sp, 0);
			return VT_BOOLEAN;
		}
		if(cb->getFunc() != NULL) {
			int n = getArgIndex(cb->getFunc(), name);
			if(n != -1) {
				cb->createMov(sp, n);
				return VT_INT;
			}
		}
		Variable *var = cb->getCtx()->getVar(name);
		if(var != NULL) {
			cb->createLoadGlobal(var, sp);
			return var->type;
		}
		fprintf(stderr, "symbol not found: %s\n", cons->str);
		throw "";
	} else if(cons->type == CONS_CAR) {
		if(cons->car == NULL || cons->car->type != CONS_STR) {
			fprintf(stderr, "not function\n");
			throw "";
		}
		const char *name = cons->car->str;
		Func *func = cb->getCtx()->getFunc(name);
		if(func == NULL) {
			fprintf(stderr, "not function\n");
			throw "";
		}
		Cons *args = cons->car->cdr;
		if(spawn && func->args != NULL) {
			return genSpawn(func, cons->car->cdr, cb, sp);
		} else {
			return func->codegen(func, args, cb, sp);
		}
	} else {
		fprintf(stderr, "integer require\n");
		throw "";
	}
}

static ValueType genAdd(Func *, Cons *cons, CodeBuilder *cb, int sp) {
	if(cons == NULL) {
		cb->createIConst(sp, 0);
		return VT_INT;
	}
	ValueType vt = codegen(cons, cb, sp, cons->cdr != NULL);
	cons = cons->cdr;
	bool tf = vt == VT_FUTURE;
	for(; cons != NULL; cons = cons->cdr) {
		int sft = tf ? 2 : 1;
		if(codegen(cons, cb, sp + sft) != VT_INT) {
			fprintf(stderr, "not integer\n");
			throw "";
		}
		if(tf) {
			cb->createJoin(sp);
			tf = false;
		}
		cb->createIAdd(sp, sp + sft);
	}
	return VT_INT;
}

static ValueType genSub(Func *, Cons *cons, CodeBuilder *cb, int sp) {
	if(cons == NULL) return VT_INT;
	codegen(cons, cb, sp);
	cons = cons->cdr;
	if(cons == NULL) {
		cb->createINeg(sp);
		return VT_INT;
	}
	for(; cons != NULL; cons = cons->cdr) {
		if(codegen(cons, cb, sp + 1) != VT_INT) {
			fprintf(stderr, "not integer\n");
			throw "";
		}
		cb->createISub(sp, sp + 1);
	}
	return VT_INT;
}

static ValueType genMul(Func *, Cons *cons, CodeBuilder *cb, int sp) {
	codegen(cons, cb, sp);
	cons = cons->cdr;
	for(; cons != NULL; cons = cons->cdr) {
		codegen(cons, cb, sp + 1);
		cb->createIMul(sp, sp + 1);
	}
	return VT_INT;
}

static ValueType genDiv(Func *, Cons *cons, CodeBuilder *cb, int sp) {
	codegen(cons, cb, sp);
	cons = cons->cdr;
	for(; cons != NULL; cons = cons->cdr) {
		codegen(cons, cb, sp + 1);
		cb->createIDiv(sp, sp + 1);
	}
	return VT_INT;
}

static ValueType genMod(Func *, Cons *cons, CodeBuilder *cb, int sp) {
	codegen(cons, cb, sp);
	cons = cons->cdr;
	for(; cons != NULL; cons = cons->cdr) {
		codegen(cons, cb, sp + 1);
		cb->createIMod(sp, sp + 1);
	}
	return VT_INT;
}

static int toOp(const char *s) {
	if(strcmp(s, "<") == 0)  return INS_IJMPGE;
	if(strcmp(s, "<=") == 0) return INS_IJMPGT;
	if(strcmp(s, ">") == 0)  return INS_IJMPLE;
	if(strcmp(s, ">=") == 0) return INS_IJMPLT;
	if(strcmp(s, "==") == 0) return INS_IJMPNE;
	if(strcmp(s, "!=") == 0) return INS_IJMPEQ;
	return -1;
}

#define genCondFunc(_fname, _op) \
static ValueType _fname(Func *, Cons *cons, CodeBuilder *cb, int sp) { \
	if(cons->cdr == NULL) { \
		cb->createIConst(sp, 1); \
		return VT_BOOLEAN; \
	} \
	codegen(cons, cb, sp); \
	codegen(cons->cdr, cb, sp + 1); \
	int l = cb->createCondOp(_op, sp, sp + 1); \
	cb->createIConst(sp, 1); \
	int m = cb->createJmp(); \
	cb->setLabel(l); \
	cb->createIConst(sp, 0); \
	cb->setLabel(m); \
	return VT_BOOLEAN; \
}

genCondFunc(genLT, INS_IJMPGE);
genCondFunc(genLE, INS_IJMPGT);
genCondFunc(genGT, INS_IJMPLE);
genCondFunc(genGE, INS_IJMPLT);
genCondFunc(genEQ, INS_IJMPNE);
genCondFunc(genNE, INS_IJMPEQ);

static ValueType genIf(Func *, Cons *cons, CodeBuilder *cb, int sp) {
	Cons *cond = cons;
	cons = cons->cdr;
	Cons *thenCons = cons;
	cons = cons->cdr;
	Cons *elseCons = cons;
	// cond
	int op, label;
	if(cond->type == CONS_CAR && (op = toOp(cond->car->str)) != -1) {
		Cons *lhs = cond->car->cdr;
		Cons *rhs = lhs->cdr;
		codegen(lhs, cb, sp);
		codegen(rhs, cb, sp+1);
		label = cb->createCondOp(op, sp, sp+1);
	} else {
		ValueType cty = codegen(cond, cb, sp);
		if(cty == VT_BOOLEAN) {
			cb->createIConst(sp + 1, 0); /* nil */
			op = INS_IJMPEQ;
			label = cb->createCondOp(op, sp, sp+1);
		} else {
			label = -1;
		}
	}
	// then expr
	codegen(thenCons, cb, sp);
	int merge = cb->createJmp();
	// else expr
	if(label != -1) cb->setLabel(label);
	if(elseCons != NULL) codegen(elseCons, cb, sp);
	cb->setLabel(merge);
	return VT_INT;
}

static const char *newStr(const char *ss) {
	int len = strlen(ss);
	char *s = new char[len + 1];
	strcpy(s, ss);
	return s;
}

static Func *newFunc(const char *name, Cons *args, CodeGenFunc gen) {
	int argc = 0;
	for(Cons *c=args; c!=NULL; c=c->cdr) {
		argc++;
	}
	Func *f = new Func();
	f->name = newStr(name);
	f->argc = argc;
	f->args = argc != 0 ? new const char *[argc] : NULL;
	int i = 0;
	for(Cons *c=args; c!=NULL; c=c->cdr) {
		f->args[i++] = newStr(c->str);
	}
	f->code = NULL;
	f->codegen = gen;
	return f;
}

#define RSFT 2
static ValueType genCall(Func *func, Cons *cons, CodeBuilder *cb, int sp) {
	ArrayBuilder<ValueType> vals;
	int n = 0;
	sp += RSFT;
	for(; cons != NULL; cons = cons->cdr) {
		ValueType v = codegen(cons, cb, sp + n, cons->cdr != NULL);
		vals.add(v);
		n += v == VT_FUTURE ? 2 : 1;
	}
	n = 0;
	for(int i=0, j=vals.getSize(); i<j; i++) {
		if(vals[i] == VT_FUTURE) {
			cb->createJoin(sp + n);
			if(n != i) cb->createMov(sp + i, sp + n);
			n += 2;
		} else {
			if(n != i) cb->createMov(sp + i, sp + n);
			n += 1;
		}
	}
	cb->createCall(func, sp);
	return VT_INT;
}

#define SRSFT 3
static ValueType genSpawn(Func *func, Cons *cons, CodeBuilder *cb, int sp) {
	int n = 0;
	for(; cons != NULL; cons = cons->cdr) {
		codegen(cons, cb, sp + SRSFT + n);
		n++;
	}
	cb->createSpawn(func, sp + SRSFT);
	return VT_FUTURE;
}

static ValueType genSetq(Func *, Cons *cons, CodeBuilder *cb, int sp) {
	assert(cons->type == CONS_STR);
	const char *name = cons->str;
	cons = cons->cdr;
	Cons *expr = cons;

	Variable *v = new Variable();
	v->name = newStr(name);
	v->value.i = 0;
	cb->getCtx()->putVar(v);

	v->type = codegen(expr, cb, sp);
	cb->createStoreGlobal(v, sp);
	return v->type;
}

struct Frame {
	Code *pc;
	int sp;
};

struct Label {
	int lb;
	Code *pc;
	int layer;
};

#define PUSH_LABEL(_l, _pc, _la) { \
		Label l; l.lb=(_l); l.pc=(_pc); l.layer=(_la); \
		la.add(l); \
	}

static bool isjmplable(ArrayBuilder<Label> *ab, Code *pc, int layer) {
	for(int i=0, j=ab->getSize(); i<j; i++) {
		Label l = (*ab)[i];
		if(pc == l.pc && layer == l.layer) return true;
	}
	return false;
}

static bool isReg2Op(int i) {
	switch(i) {
	case INS_IADD: 
	case INS_ISUB: 
	//case INS_IMUL: 
	//case INS_IDIV: 
	//case INS_IMOD: 
		return true;
	}
	return false;
}

static bool isReg2COp(int i) {
	switch(i) {
	case INS_IADDC: 
	case INS_ISUBC: 
		return true;
	}
	return false;
}

static int applyOpC(int i, int x, int y) {
	switch(i) {
	case INS_IADDC: return x + y;
	case INS_ISUBC: return x - y;
	}
	exit(1);
}
	
static int toConstOp(int i) {
	switch(i) {
	case INS_IADD: return INS_IADDC;
	case INS_ISUB: return INS_ISUBC;
	case INS_IJMPLT: return INS_IJMPLTC;
	case INS_IJMPLE: return INS_IJMPLEC;
	case INS_IJMPGT: return INS_IJMPGTC;
	case INS_IJMPGE: return INS_IJMPGEC;
	case INS_IJMPEQ: return INS_IJMPEQC;
	case INS_IJMPNE: return INS_IJMPNEC;
	}
	exit(1);
}

static bool isCondJmpOp(int i) {
	switch(i) {
	case INS_IJMPLT: 
	case INS_IJMPLE: 
	case INS_IJMPGT: 
	case INS_IJMPGE: 
	case INS_IJMPEQ: 
	case INS_IJMPNE:
		return true;
	}
	return false;
}

static bool isCondJmpCOp(int i) {
	switch(i) {
	case INS_IJMPLTC: 
	case INS_IJMPLEC: 
	case INS_IJMPGTC: 
	case INS_IJMPGEC: 
	case INS_IJMPEQC: 
	case INS_IJMPNEC:
		return true;
	}
	return false;
}

static void opt_inline(Context *ctx, Func *func, int inlinecnt) {
	CodeBuilder cb(ctx, func, false);
	Code *pc = func->code;
	Frame *frame = new Frame[inlinecnt + 1];
	Frame *fp = frame;
	ArrayBuilder<Label> la;
	int sp = 0;
	int layer = 0;
	L_BEGIN:
	for(int i=0, j=la.getSize(); i<j; i++) {
		Label l = la[i];
		if(pc == l.pc && layer == l.layer) {
			cb.setLabel(l.lb);
			la[i].pc = NULL;
		}
	}
	switch(pc->i) {
	case INS_ICONST: {
		if(!isjmplable(&la, pc+3, layer)) {
			if(pc[3].i == INS_MOV && (pc[1].i == pc[5].i)) {
				cb.createIConst(pc[4].i + sp, pc[2].i);
				pc += 3 + 3;
				break;
			}
			if(isReg2Op(pc[3].i) && (pc[1].i == pc[5].i)) {
				// const a x && op b a -> opC b x
				cb.createIntIns(toConstOp(pc[3].i), pc[4].i+sp, pc[2].i);
				pc += 3 + 3;
				break;
			}
			if(isReg2COp(pc[3].i) && (pc[1].i == pc[4].i)) {
				// const a x && opC a y -> const a (x op y)
				cb.createIConst(pc[1].i+sp, applyOpC(pc[3].i, pc[2].i, pc[5].i));
				pc += 3 + 3;
				break;
			}
			if(isCondJmpOp(pc[3].i) && pc[1].i == pc[6].i) {
				// const a x && jmpxx b a -> jmpxxC b x
				int n = cb.createCondOpC(toConstOp(pc[3].i), pc[5].i+sp, pc[2].i);
				PUSH_LABEL(n, pc + 3 + pc[4].i, layer);
				pc += 3 + 4;
				break;
			}
			if(pc[3].i == INS_RET && (pc[1].i == pc[4].i) && layer == 0) {
				cb.createRetC(pc[2].i);
				pc += 3 + 2;
				break;
			}
		}
		cb.createIConst(pc[1].i + sp, pc[2].i);
		pc += 3;
		break;
	}
	case INS_MOV: {
		if(!isjmplable(&la, pc+3, layer)) {
			if(pc[3].i == INS_MOV && pc[1].i == pc[5].i) {
				// mov b a && mov c b -> mov c a
				cb.createMov(pc[4].i + sp, pc[2].i + sp);
				pc += 3 + 3;
				break;
			}
			if(pc[3].i == INS_RET && pc[1].i == pc[4].i && layer == 0) {
				// mov b a && ret b -> ret a
				cb.createRet(pc[2].i + sp);
				pc += 3 + 2;
				break;
			}
			if(isCondJmpOp(pc[3].i) && pc[1].i == pc[6].i) {
				// mov b a && jmpxx c b -> jmpxx c a
				int n = cb.createCondOp(pc[3].i, pc[5].i+sp, pc[2].i+sp);
				PUSH_LABEL(n, pc + 3 + pc[4].i, layer);
				pc += 3 + 4;
				break;
			}
			if(isCondJmpOp(pc[3].i) && pc[1].i == pc[5].i) {
				// mov b a && jmpxx b c -> jmpxx a c
				int n = cb.createCondOp(pc[3].i, pc[2].i+sp, pc[6].i+sp);
				PUSH_LABEL(n, pc + 3 + pc[4].i, layer);
				pc += 3 + 4;
				break;
			}
			if(isCondJmpCOp(pc[3].i) && pc[1].i == pc[5].i) {
				// mov b a && jmpxxC b n -> jmpxxC a n
				int n = cb.createCondOpC(pc[3].i, pc[2].i+sp, pc[6].i);
				PUSH_LABEL(n, pc + 3 + pc[4].i, layer);
				pc += 3 + 4;
				break;
			}
		}
		cb.createMov(pc[1].i + sp, pc[2].i + sp);
		pc += 3;
		break;
	}
	case INS_IADD: 
	case INS_ISUB: 
	case INS_IMUL: 
	case INS_IDIV: 
	case INS_IMOD: {
		cb.createReg2Ins(pc[0].i, pc[1].i + sp, pc[2].i + sp); pc += 3;
		break;
	}
	case INS_IADDC: cb.createIAddC(pc[1].i + sp, pc[2].i); pc += 3; break;
	case INS_ISUBC: cb.createISubC(pc[1].i + sp, pc[2].i); pc += 3; break;
	case INS_INEG: cb.createINeg(pc[1].i + sp); pc += 2; break;
	case INS_IJMPLT: 
	case INS_IJMPLE: 
	case INS_IJMPGT: 
	case INS_IJMPGE: 
	case INS_IJMPEQ: 
	case INS_IJMPNE: {
		int n = cb.createCondOp(pc[0].i, pc[2].i + sp, pc[3].i + sp);
		PUSH_LABEL(n, pc + pc[1].i, layer);
		pc += 4;
		break;
	}
	case INS_IJMPLTC: 
	case INS_IJMPLEC: 
	case INS_IJMPGTC: 
	case INS_IJMPGEC: 
	case INS_IJMPEQC: 
	case INS_IJMPNEC: {
		int n = cb.createCondOpC(pc[0].i, pc[2].i + sp, pc[3].i);
		PUSH_LABEL(n, pc + pc[1].i, layer);
		pc += 4;
		break;
	}
	
	case INS_JMP: {
		Code *pc2 = pc + pc[1].i;
		if(pc2->i == INS_RET) {
			// jump & return
			if(layer > 0) {
				cb.createMov(sp-2, sp + pc2[1].i);
				// goto end
				if(pc[2].i != INS_END) {
					int n = cb.createJmp();
					PUSH_LABEL(n, fp[-1].pc, layer-1);
				}
			} else {
				cb.createRet(pc2[1].i + sp);
			}
		} else {
			int n = cb.createJmp();
			PUSH_LABEL(n, pc2, layer);
		}
		pc += 2;
		break;
	}
	case INS_LOAD_GLOBAL:  cb.createLoadGlobal(pc[1].var, pc[2].i + sp); pc += 3; break;
	case INS_STORE_GLOBAL: cb.createStoreGlobal(pc[1].var, pc[2].i + sp); pc += 3; break;
	case INS_CALL:  {
		if(layer < inlinecnt) {
			// inline
			fp->pc = pc + 3;
			fp->sp = sp;
			sp += pc[2].i;
			pc = pc[1].func->code;
			fp++;
			layer++;
		} else {
			// not inline
			cb.createCall(pc[1].func, pc[2].i + sp);
			pc += 3;
		}
		break;
	}
	case INS_SPAWN: cb.createSpawn(pc[1].func, pc[2].i + sp); pc += 3; break;
	case INS_RET: {
		if(layer > 0) {
			cb.createMov(sp-2, sp + pc[1].i);
			// goto end
			if(pc[2].i != INS_END) {
				int n = cb.createJmp();
				PUSH_LABEL(n, fp[-1].pc, layer-1);
			}
		} else {
			cb.createRet(pc[1].i);
		}
		pc += 2;
		break;
	}
	case INS_RETC: {
		if(layer > 0) {
			cb.createIConst(sp-2, pc[1].i);
			pc += 3;
			// goto end
			if(pc->i != INS_END) {
				int n = cb.createJmp();
				PUSH_LABEL(n, fp[-1].pc, layer-1);
			}
		} else {
			cb.createRetC(pc[1].i);
			pc += 3;
		}
		break;
	}
	case INS_JOIN: cb.createJoin(pc[1].i + sp); pc += 2; break;
	case INS_IPRINT: cb.createPrintInt(pc[1].i + sp); pc += 2; break;
	case INS_BPRINT: cb.createPrintBoolean(pc[1].i + sp); pc += 2; break;
	case INS_END: {
		if(layer == 0) {
			cb.createEnd();
			goto L_FINAL;
		}
		layer--;
		fp--;
		pc = fp->pc;
		sp = fp->sp;
		break;
	}
	default:
		abort();
	}
	goto L_BEGIN;

	L_FINAL:;
	delete [] func->code;
	delete [] frame;
	func->code = cb.getCode();
	func->codeLength = cb.getCodeLength();
}

#ifdef USING_THCODE
static void opt_thcode(Context *ctx, Func *func) {
	CodeBuilder cb(ctx, func, true);
	Code *pc = func->code;

	L_BEGIN:
	switch(pc->i) {
		// int ins
	case INS_ICONST:
	case INS_IADDC:
	case INS_ISUBC:
	case INS_RETC:
		cb.createIntIns(pc[0].i, pc[1].i, pc[2].i);
		pc += 3;
		break;

		// reg2ins
	case INS_MOV:
	case INS_IADD:
	case INS_ISUB:
	case INS_IMUL:
	case INS_IDIV:
	case INS_IMOD:
		cb.createReg2Ins(pc[0].i, pc[1].i, pc[2].i);
		pc += 3;
		break;

		// regins
	case INS_INEG:
	case INS_RET:
	case INS_JOIN:
	case INS_IPRINT:
	case INS_BPRINT:
		cb.createRegIns(pc[0].i, pc[1].i);
		pc += 2;
		break;

// jmp pc+[r1] if [r1] < [r2]
	case INS_IJMPLT:
	case INS_IJMPLE:
	case INS_IJMPGT:
	case INS_IJMPGE:
	case INS_IJMPEQ:
	case INS_IJMPNE:
		cb.createCondOp(pc[0].i, pc[2].i, pc[3].i, pc[1].i);
		pc += 4;
		break;

// jmp pc+[r1] if [r1] < v2
	case INS_IJMPLTC:
	case INS_IJMPLEC:
	case INS_IJMPGTC:
	case INS_IJMPGEC:
	case INS_IJMPEQC:
	case INS_IJMPNEC:
		cb.createCondOpC(pc[0].i, pc[2].i, pc[3].i, pc[1].i);
		pc += 4;
		break;

// jmp pc+[r1]
	case INS_JMP:
		cb.createJmp(pc[1].i);
		pc += 2;
		break;

// global variable [var] [r1]
	case INS_LOAD_GLOBAL:
	case INS_STORE_GLOBAL:
		cb.createVarIns(pc[0].i, pc[1].i, pc[2].var);
		pc += 3;
		break;
// call [func], shift, rix
	case INS_CALL:
	case INS_SPAWN:
		cb.createFuncIns(pc[0].i, pc[1].func, pc[2].i);
		pc += 3;
		break;
	case INS_END:
		cb.createEnd();
		goto L_FINAL;
	}
	goto L_BEGIN;
	L_FINAL:
	func->thcode = cb.getCode();
	func->codeLength = cb.getCodeLength();
}
#endif

static ValueType genDefun(Func *, Cons *cons, CodeBuilder *cb, int sp) {
	const char *name = cons->str;
	cons = cons->cdr;
	Cons *args = cons->car;
	cons = cons->cdr;
	Func *func = newFunc(name, args, genCall);

	Context *ctx = cb->getCtx();
	ctx->putFunc(func);

	CodeBuilder newCb(ctx, func, false);
	codegen(cons, &newCb, func->argc);
	newCb.createRet(func->argc);
	newCb.createEnd();
	func->code = newCb.getCode();
	func->codeLength = newCb.getCodeLength();
#define CODESIZE_BORDER 400
	for(int i=0; i<ctx->inlinecount; i++) {
		opt_inline(ctx, func, 1);
		if(func->codeLength >= CODESIZE_BORDER) break;
	}
	for(int i=0; i<4; i++) {
		opt_inline(ctx, func, 0);
	}
#ifdef USING_THCODE
	opt_thcode(ctx, func);
#endif
	return VT_VOID;
}

void addDefaultFuncs(Context *ctx) {
	ctx->putFunc(newFunc("+" , NULL, genAdd));
	ctx->putFunc(newFunc("-" , NULL, genSub));
	ctx->putFunc(newFunc("*" , NULL, genMul));
	ctx->putFunc(newFunc("/" , NULL, genDiv));
	ctx->putFunc(newFunc("mod" , NULL, genMod));
	ctx->putFunc(newFunc("<" , NULL, genLT));
	ctx->putFunc(newFunc(">" , NULL, genGT));
	ctx->putFunc(newFunc("<=", NULL, genLE));
	ctx->putFunc(newFunc(">=", NULL, genGE));
	ctx->putFunc(newFunc("=", NULL, genEQ));
	ctx->putFunc(newFunc("eq", NULL, genEQ));
	ctx->putFunc(newFunc("equal", NULL, genEQ));
	ctx->putFunc(newFunc("!=", NULL, genNE));
	ctx->putFunc(newFunc("if", NULL, genIf));
	ctx->putFunc(newFunc("setq", NULL, genSetq));
	ctx->putFunc(newFunc("defun", NULL, genDefun));
}

