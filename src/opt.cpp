#include "lisp.h"

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

static bool isjmplabel(ArrayBuilder<Label> *ab, Code *pc, int layer) {
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
	case INS_IMUL: 
	case INS_IDIV: 
	case INS_IMOD: 
		return true;
	}
	return false;
}

static bool isReg2COp(int i) {
	switch(i) {
	case INS_IADDC: 
	case INS_ISUBC: 
	case INS_IMULC: 
	case INS_IDIVC: 
	case INS_IMODC: 
		return true;
	}
	return false;
}

static int applyOpC(int i, int x, int y) {
	switch(i) {
	case INS_IADDC: return x + y;
	case INS_ISUBC: return x - y;
	case INS_IMULC: return x * y;
	case INS_IDIVC: return x / y;
	case INS_IMODC: return x % y;
	case INS_IJMPLTC: return x < y;
	case INS_IJMPLEC: return x <= y;
	case INS_IJMPGTC: return x > y;
	case INS_IJMPGEC: return x >= y;
	case INS_IJMPEQC: return x == y;
	case INS_IJMPNEC: return x != y;
	}
	abort();
}
	
static int toConstOp(int i) {
	switch(i) {
	case INS_IADD: return INS_IADDC;
	case INS_ISUB: return INS_ISUBC;
	case INS_IMUL: return INS_IMULC;
	case INS_IDIV: return INS_IDIVC;
	case INS_IMOD: return INS_IMODC;
	case INS_IJMPLT: return INS_IJMPLTC;
	case INS_IJMPLE: return INS_IJMPLEC;
	case INS_IJMPGT: return INS_IJMPGTC;
	case INS_IJMPGE: return INS_IJMPGEC;
	case INS_IJMPEQ: return INS_IJMPEQC;
	case INS_IJMPNE: return INS_IJMPNEC;
	}
	abort();
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

static int toRevCondJmpOp(int i) {
	switch(i) {
	case INS_IJMPLT: return INS_IJMPGE;
	case INS_IJMPLE: return INS_IJMPGT;
	case INS_IJMPGT: return INS_IJMPLE;
	case INS_IJMPGE: return INS_IJMPLT;
	case INS_IJMPEQ: return INS_IJMPNE;
	case INS_IJMPNE: return INS_IJMPEQ;
	default: abort();
	}
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

static int getOpSize(int i) {
	switch(i) {
		// int ins
	case INS_RETC:
		return 2;
		// reg int ins
	case INS_ICONST:
	case INS_IADDC:
	case INS_ISUBC:
	case INS_IMULC:
	case INS_IDIVC:
	case INS_IMODC:
		// reg2ins
	case INS_MOV:
	case INS_IADD:
	case INS_ISUB:
	case INS_IMUL:
	case INS_IDIV:
	case INS_IMOD:
		return 3;

		// regins
	case INS_INEG:
	case INS_RET:
	case INS_JOIN:
	case INS_IPRINT:
	case INS_BPRINT:
		return 2;

		// jmp
	case INS_IJMPLT:
	case INS_IJMPLE:
	case INS_IJMPGT:
	case INS_IJMPGE:
	case INS_IJMPEQ:
	case INS_IJMPNE:
	case INS_IJMPLTC:
	case INS_IJMPLEC:
	case INS_IJMPGTC:
	case INS_IJMPGEC:
	case INS_IJMPEQC:
	case INS_IJMPNEC:
		return 4;

// jmp pc+[r1]
	case INS_JMP:
		return 2;

// global variable [var] [r1]
	case INS_LOAD_GLOBAL:
	case INS_STORE_GLOBAL:
		return 3;

// call [func], shift, rix
	case INS_CALL:
	case INS_SPAWN:
		return 3;
	case INS_DEFUN:
		return 2;
	case INS_END:
		return 1;
	default:
		abort();
	}
}

static void opt_inline(Context *ctx, Func *func, int inlinecnt, bool showir) {
	CodeBuilder cb(ctx, func, false, showir);
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
	// unused inst
	if((pc[0].i == INS_ICONST || pc[0].i == INS_MOV || isReg2Op(pc[0].i) || isReg2COp(pc[0].i)) &&
			!isjmplabel(&la, pc+3, layer) && (pc[3].i == INS_RET || pc[3].i == INS_RETC) &&
			pc[1].i != pc[4].i) {
		pc += 3;
		goto L_BEGIN;
	}
	switch(pc->i) {
	case INS_ICONST: {
		if(!isjmplabel(&la, pc+3, layer)) {
			if(pc[3].i == INS_MOV && (pc[1].i == pc[5].i)) {
				cb.createIConst(pc[4].i + sp, pc[2].i);
				pc += 3 + 3;
				break;
			}
			if(isReg2Op(pc[3].i) && (pc[1].i == pc[5].i)) {
				// const a x && op b a -> opC b x
				cb.createRegIntIns(toConstOp(pc[3].i), pc[4].i+sp, pc[2].i);
				pc += 3 + 3;
				break;
			}
			if((pc[3].i == INS_IADD || pc[3].i == INS_IMUL) && (pc[1].i == pc[4].i)) {
				// const a x && op a b -> mov a b && opC a x
				cb.createMov(pc[4].i + sp, pc[5].i + sp);
				cb.createRegIntIns(toConstOp(pc[3].i), pc[4].i+sp, pc[2].i);
				pc += 3 + 3;
				break;
			}
			if(isReg2COp(pc[3].i) && (pc[1].i == pc[4].i)) {
				// const a x && opC a y -> const a (x op y)
				cb.createIConst(pc[1].i+sp, applyOpC(pc[3].i, pc[2].i, pc[5].i));
				pc += 3 + 3;
				break;
			}
			if(pc[3].i == INS_INEG && (pc[1].i == pc[4].i)) {
				// const a x && neg a -> const a -x
				cb.createIConst(pc[1].i + sp, -pc[2].i);
				pc += 3 + 2;
				break;
			}
			if(isCondJmpOp(pc[3].i) && pc[1].i == pc[6].i) {
				// const a x && jmpxx b a -> jmpxxC b x
				int n = cb.createCondOpC(toConstOp(pc[3].i), pc[5].i+sp, pc[2].i);
				PUSH_LABEL(n, pc + 3 + pc[4].i, layer);
				pc += 3 + 4;
				break;
			}
			if(isCondJmpOp(pc[3].i) && pc[1].i == pc[5].i) {
				// const a x && jmpxx a b -> jmp[!xx]C b x
				int op = toConstOp(toRevCondJmpOp(pc[3].i));
				int n = cb.createCondOpC(op, pc[6].i+sp, pc[2].i);
				PUSH_LABEL(n, pc + 3 + pc[4].i, layer);
				pc += 3 + 4;
				break;
			}
			if(isCondJmpCOp(pc[3].i) && pc[1].i == pc[5].i) {
				// const a x && jmpxxC a y
				if(applyOpC(pc[3].i, pc[2].i, pc[6].i)) {
					int n = cb.createJmp();
					PUSH_LABEL(n, pc + 3 + pc[4].i, layer);
				}
				pc += 3 + 4;
				break;
			}
			if(pc[3].i == INS_RET && (pc[1].i == pc[4].i) && layer == 0) {
				// const a x && ret a -> retc x
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
		if(!isjmplabel(&la, pc+3, layer)) {
			if(pc[3].i == INS_MOV && pc[1].i == pc[5].i) {
				// mov b a && mov c b -> mov c a
				cb.createMov(pc[4].i + sp, pc[2].i + sp);
				pc += 3 + 3;
				break;
			}
			if(isReg2Op(pc[3].i) && (pc[1].i == pc[5].i)) {
				// mov b a && op c b -> op c a
				cb.createReg2Ins(pc[3].i, pc[4].i+sp, pc[2].i);
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
			if(isReg2COp(pc[3].i) && pc[6].i == INS_RET && pc[1].i == pc[4].i && pc[4].i == pc[7].i && !isjmplabel(&la, pc+6, layer)) {
				// mov b a && opC b x && ret b -> opC a x && ret a
				cb.createRegIntIns(pc[3].i, pc[2].i + sp, pc[5].i);
				cb.createRet(pc[2].i + sp);
				pc += 3 + 3 + 2;
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
	case INS_IMOD:
		cb.createReg2Ins(pc[0].i, pc[1].i + sp, pc[2].i + sp);
		pc += 3;
		break;
	case INS_IADDC:
	case INS_ISUBC:
	case INS_IMULC:
	case INS_IDIVC:
	case INS_IMODC:
		if(pc[0].i == INS_IADDC && pc[3].i == INS_IADDC && pc[1].i == pc[4].i) {
			// iaddc a x && iaddc a y -> iaddc a (x+y)
			cb.createRegIntIns(pc[0].i, pc[1].i + sp, pc[2].i + pc[5].i);
			pc += 3 + 3;
			break;
		}
		if((pc[0].i == INS_IMULC && pc[3].i == INS_IMULC && pc[1].i == pc[4].i) ||
				(pc[0].i == INS_IDIVC && pc[3].i == INS_IDIVC && pc[1].i == pc[4].i)) {
			// imulc a x && imulc a y -> imulc a (x*y)
			// idivc a x && idivc a y -> idivc a (x*y)
			cb.createRegIntIns(pc[0].i, pc[1].i + sp, pc[2].i * pc[5].i);
			pc += 3 + 3;
			break;
		}
		if(pc[0].i == INS_ISUBC) {
			cb.createRegIntIns(INS_IADDC, pc[1].i + sp, -pc[2].i);
			pc += 3;
			break;
		}
		if(pc[0].i == INS_IADDC && pc[2].i == 0) {
			// do nothing
		} else if((pc[0].i == INS_IMULC || pc[0].i == INS_IDIVC) && pc[2].i == 1) {
			// do nothing
		} else if(pc[0].i == INS_IMULC && pc[2].i == 0) {
			cb.createIConst(pc[1].i + sp, 0);
		} else {
			cb.createRegIntIns(pc[0].i, pc[1].i + sp, pc[2].i);
		}
		pc += 3;
		break;
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
		if(pc2->i == INS_RET && layer == 0) {
			cb.createRet(pc2[1].i + sp);
			pc += 2;
		} else if(pc2->i == INS_RETC && layer == 0) {
			cb.createRetC(pc2[1].i);
			pc += 2;
		} else if(isCondJmpOp(pc2->i) && layer == 0) {
			int n = cb.createCondOp(pc2[0].i, pc2[2].i + sp, pc2[3].i + sp);
			PUSH_LABEL(n, pc2 + pc2[1].i, layer);
			int m = cb.createJmp();
			PUSH_LABEL(m, pc2 + 4, layer);
			pc += 2;
		} else if(isCondJmpCOp(pc2->i) && layer == 0) {
			int n = cb.createCondOpC(pc2[0].i, pc2[2].i + sp, pc2[3].i);
			PUSH_LABEL(n, pc2 + pc2[1].i, layer);
			int m = cb.createJmp();
			PUSH_LABEL(m, pc2 + 4, layer);
			pc += 2;
		} else if(pc2 == pc + 2) {
			// do nothing
			pc += 2;
		} else {
			// skip dead code
			Code *pc3 = pc + 2;
			while(pc3 < pc2 && pc3->i != INS_END && !isjmplabel(&la, pc3, layer)) {
				pc3 += getOpSize(pc3->i);
			}
			int n = cb.createJmp();
			PUSH_LABEL(n, pc2, layer);
			pc = pc3;
		}
		break;
	}
	case INS_LOAD_GLOBAL:  cb.createLoadGlobal (pc[1].i + sp, pc[2].var); pc += 3; break;
	case INS_STORE_GLOBAL: cb.createStoreGlobal(pc[1].i + sp, pc[2].var); pc += 3; break;
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
		// skip dead code
		while(pc->i != INS_END && !isjmplabel(&la, pc, layer)) {
			pc += getOpSize(pc->i);
		}
		break;
	}
	case INS_RETC: {
		if(layer > 0) {
			cb.createIConst(sp-2, pc[1].i);
			pc += 2;
			// goto end
			if(pc->i != INS_END) {
				int n = cb.createJmp();
				PUSH_LABEL(n, fp[-1].pc, layer-1);
			}
		} else {
			cb.createRetC(pc[1].i);
			pc += 2;
		}
		// skip dead code
		while(pc->i != INS_END && !isjmplabel(&la, pc, layer)) {
			pc += getOpSize(pc->i);
		}
		break;
	}
	case INS_JOIN: cb.createJoin(pc[1].i + sp); pc += 2; break;
	case INS_IPRINT: cb.createPrintInt(pc[1].i + sp); pc += 2; break;
	case INS_BPRINT: cb.createPrintBoolean(pc[1].i + sp); pc += 2; break;
	case INS_DEFUN: cb.createConsIns(pc[0].i, pc[1].cons); pc += 2; break;
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
	CodeBuilder cb(ctx, func, true, false);
	Code *pc = func->code;

	L_BEGIN:
	switch(pc->i) {
		// int ins
	case INS_RETC:
		cb.createIntIns(pc[0].i, pc[1].i);
		pc += 2;
		break;

		// reg int ins
	case INS_ICONST:
	case INS_IADDC:
	case INS_ISUBC:
	case INS_IMULC:
	case INS_IDIVC:
	case INS_IMODC:
		cb.createRegIntIns(pc[0].i, pc[1].i, pc[2].i);
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
// defun [cons]
	case INS_DEFUN:
		cb.createConsIns(pc[0].i, pc[1].cons);
		pc += 2;
		break;
	case INS_END:
		cb.createEnd();
		goto L_FINAL;
	default:
		abort();
	}
	goto L_BEGIN;
	L_FINAL:
	func->thcode = cb.getCode();
	func->codeLength = cb.getCodeLength();
}
#endif

#define CODESIZE_BORDER 400

void codeopt(Context *ctx, Func *func) {
	for(int i=0; i<2; i++) {
		opt_inline(ctx, func, 0, false);
	}
	for(int i=0; i<ctx->inlinecount; i++) {
		opt_inline(ctx, func, 1, false);
		if(func->codeLength >= CODESIZE_BORDER) break;
	}
	for(int i=0; i<4; i++) {
		opt_inline(ctx, func, 0, false);
	}
	opt_inline(ctx, func, 0, true);
#ifdef USING_THCODE
	opt_thcode(ctx, func);
#endif
}

