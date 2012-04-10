#ifndef LISP_H
#define LISP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

struct Code;
struct Func;
struct Context;
struct Value;
struct Future;
struct WorkerThread;

//------------------------------------------------------
// context

struct Frame {
	Value *sp;
	Code *pc;
	int rix;
};

struct Value {
	union {
		int i;
		double d;
		const char *str;
		Future *future;
	};
};

struct Future {
	WorkerThread *wth;
	int (*getResult)(Future *);
};

struct WorkerThread {
	pthread_t pth;
	Code *pc;
	Value *sp;
	Frame *fp;
	Context *ctx;
	Future future;
	Frame frame[256];
	Value stack[1024];
};

struct Context {
	void *jmptable[256];
	Func *funcs[256];
	int funcLen;
};

void vmrun(Context *ctx, WorkerThread *wth);
WorkerThread *newWorkerThread(Context *ctx, Code *pc, int argc, Value *argv);
void joinWorkerThread(WorkerThread *wth);
void deleteWorkerThread(WorkerThread *wth);
	
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

//------------------------------------------------------
// code
struct Code {
	union {
		int i;
		void *ptr;
		Func *func;
	};
};

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
	CodeBuilder(Context *_ctx, Func *_func) {
		ctx = _ctx;
		func = _func;
		sp = 0;
		ci = 0;
	}
	Context *ctx;
	Func *func;
	Code code[256];
	int ci;
	int sp;
	void addInst(int inst) {
		//printf("%03d: %s\n", ci, getInstName(inst));
		code[ci++].ptr = ctx->jmptable[inst];
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
		Code *c = new Code[ci];
		memcpy(c, code, sizeof(Code) * ci);
		func->code = c;
	}
};

void codegen(Context *ctx, Cons *cons, CodeBuilder *cb);

#endif

