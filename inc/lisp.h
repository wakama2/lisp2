#ifndef LISP_H
#define LISP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

struct Code;
struct Func;

//------------------------------------------------------
// context

struct Frame {
	int *sfp;
	Code *pc;
	int rix;
};

struct WorkerThread {
	pthread_t pth;
	Code *pc;
	int stack[256];
	Frame frame[256];
};

struct Context {
	Func *funcs[256];
	int funcLen;

	Context() {
		funcLen = 0;
	}
};

void vmrun(WorkerThread *wth);

//------------------------------------------------------
// cons

enum {
	CONS_INT,
	CONS_STR,
	CONS_CAR,
};

struct Cons {
	int type;
	union {
		int i;
		const char *str;
		Cons *car;
	};
	Cons *cdr;

	void print(FILE *fp = stdout);
	void println(FILE *fp = stdout);
};

struct Func {
	const char *name;
	int argc;
	const char *args[64];
	Code *code;
};

//------------------------------------------------------
// parser
template <class T>
struct ParseResult {
	const char *src;
	T value;
	bool success;

	static ParseResult<T> make(const char *src, T value, bool success = true) {
		ParseResult<T> r;
		r.src = src;
		r.value = value;
		r.success = success;
		return r;
	}
};

ParseResult<Cons *> parseExpr(const char *src);
Cons *newConsI(int i, Cons *cdr);
Cons *newConsS(const char *str, Cons *cdr);
Cons *newConsCar(Cons *car, Cons *cdr);

//------------------------------------------------------
// code
struct Code {
	union {
		int i;
		void *ptr;
		Func *func;
	};
};
extern void *jmptable[256];

enum {
#define I(a) a,
#include "inst"
#undef I
};

const char *getInstName(int inst);

//------------------------------------------------------
// code generator
class CodeBuilder {
public:
	CodeBuilder(Func *f = NULL) {
		func = f;
		code = new Code[256];
		sp = 0;
		ci = 0;
	}
	Func *func;
	Code *code;
	int ci;
	int sp;
	void addInst(int inst) {
		printf("%03d: %s\n", ci, getInstName(inst));
		code[ci++].ptr = jmptable[inst];
	}
	void addInt(int n) {
		code[ci++].i = n;
	}
	void addFunc(Func *func) {
		code[ci++].func = func;
	}
	void createIConst(int r, int i) {
		addInst(INS_ICONST);
		addInt(r);
		addInt(i);
	}
	void createMov(int r, int n) {
		addInst(INS_IMOV);
		addInt(r);
		addInt(n);
	}
	void createOp(int inst, int r, int a) {
		addInst(inst);
		addInt(r);
		addInt(a);
	}
	// return label
	int createCondOp(int inst, int a, int b) {
		int lb = ci;
		addInst(inst);
		addInt(0);
		addInt(a);
		addInt(b);
		return lb;
	}
	int createJmp() {
		int lb = ci;
		addInst(INS_JMP);
		addInt(0);
		return lb;
	}
	void createIAdd(int r, int a) { createOp(INS_IADD, r, a); }
	void createISub(int r, int a) { createOp(INS_ISUB, r, a); }
	void createIMul(int r, int a) { createOp(INS_IMUL, r, a); }
	void createIDiv(int r, int a) { createOp(INS_IDIV, r, a); }
	void createIMod(int r, int a) { createOp(INS_IMOD, r, a); }
	void createINeg(int r) {
		addInst(INS_INEG);
		addInt(r);
	}
	void createCall(Func *func, int shift, int rix) {
		addInst(INS_CALL);
		addFunc(func);
		addInt(shift);
		addInt(rix);
	}
	void createRet() { addInst(INS_RET); }
	void setLabel(int n) {
		code[n + 1].i = ci - n;
	}
	void accept(Func *func) {
		func->code = code;
	}
};

void codegen(Context *ctx, Cons *cons, CodeBuilder *cb);

#endif

