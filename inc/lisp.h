#ifndef LISP_H
#define LISP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

struct Code;
struct Func;
struct Value;
struct Future;
struct Cons;
class WorkerThread;
class Context;
class CodeBuilder;

#define TH_MAX 9
#define ATOMIC_ADD(p, v) __sync_fetch_and_add(p, v)
#define ATOMIC_SUB(p, v) __sync_fetch_and_sub(p, v)

//------------------------------------------------------
// context

struct Frame {
	Value *sp;
	Code *pc;
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
	union {
		WorkerThread *wth;
		Value v;
	};
	int (*getResult)(Future *);
};

class WorkerThread {
public:
	Context *ctx;
	pthread_t pth;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	// exec
	Code *pc;
	Value *sp;
	Frame *fp;
	WorkerThread *parent;
	WorkerThread *next;
	// private
	Future future;
	Frame frame[64];
	Value stack[256];

	void lock();
	void unlock();
};

class Context {
private:
	pthread_mutex_t mutex;

public:
	void *jmptable[256];
	Func *funcs[256];
	int funcLen;
	WorkerThread wth[TH_MAX];
	WorkerThread *freeThread;
	int threadCount;

	// methods
	Context();
	~Context();
	void lock();
	void unlock();
};

void vmrun(Context *ctx, WorkerThread *wth);
WorkerThread *newWorkerThread(Context *ctx, WorkerThread *wth, Code *pc, int argc, Value *argv);
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
	void codegen(CodeBuilder *cb);
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

enum {
	TYPE_INT,
	TYPE_TNIL,
	TYPE_STR,
	TYPE_FUTURE,
};

//------------------------------------------------------
// code generator
class CodeBuilder {
public:
	CodeBuilder(Context *_ctx, Func *_func) {
		ctx = _ctx;
		func = _func;
		sp = func != NULL ? func->argc : 0;
		ci = 0;
		for(int i=0; i<256; i++) stype[i] = -1;
		for(int i=0; i<sp; i++) stype[i] = TYPE_INT;
	}
	Context *ctx;
	Code code[256];
	int ci;
	int stype[256];
	void addInst(int inst) {
		printf("%03d: %s\n", ci, getInstName(inst));
		code[ci++].ptr = ctx->jmptable[inst];
	}
	void addInt(int n) {
		code[ci++].i = n;
	}
	void addFunc(Func *func) {
		code[ci++].func = func;
	}
	Func *func;
	int sp;
	void createIConst(int r, int i) {
		addInst(INS_ICONST);
		addInt(r);
		addInt(i);
		stype[r] = TYPE_INT;
	}
	void createMov(int r, int n) {
		addInst(INS_MOV);
		addInt(r);
		addInt(n);
		stype[r] = stype[n];
	}
	void createOp(int inst, int r, int a) {
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
	void createIAdd(int r, int a) { createOp(INS_IADD, r, a); }
	void createISub(int r, int a) { createOp(INS_ISUB, r, a); }
	void createIMul(int r, int a) { createOp(INS_IMUL, r, a); }
	void createIDiv(int r, int a) { createOp(INS_IDIV, r, a); }
	void createIMod(int r, int a) { createOp(INS_IMOD, r, a); }
	void createINeg(int r) {
		addInst(INS_INEG);
		addInt(r);
		assert(stype[r] == TYPE_INT);
	}
	void createCall(Func *func, int shift, int rix) {
		addInst(INS_CALL);
		addFunc(func);
		addInt(shift);
		addInt(rix);
		stype[rix] = TYPE_INT;
	}
	void createSpawn(Func *func, int shift, int rix) {
		addInst(INS_SPAWN);
		addFunc(func);
		addInt(shift);
		addInt(rix);
		stype[rix] = TYPE_FUTURE;
	}
	void createJoin(int n) {
		addInst(INS_JOIN);
		addInt(n);
		assert(stype[n] == TYPE_FUTURE);
		stype[n] = TYPE_INT;
	}
	void createRet() { 
		if(func != NULL && func->argc != 0) { createMov(0, func->argc); }
		addInst(INS_RET);
	}
	void createExit() {
		if(func != NULL && func->argc != 0) { createMov(0, func->argc); }
		addInst(INS_EXIT);
	}
	void setLabel(int n) {
		code[n + 1].i = ci - n;
	}
	void accept(Func *func) {
		Code *c = new Code[ci];
		memcpy(c, code, sizeof(Code) * ci);
		func->code = c;
	}
};

#endif

