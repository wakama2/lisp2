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
struct WorkerThread;
struct Task;
class Scheduler;
class Context;
class CodeBuilder;

#define WORKER_MAX 8
#define TASK_MAX   (WORKER_MAX * 4)
#define ATOMIC_ADD(p, v) __sync_fetch_and_add(p, v)
#define ATOMIC_SUB(p, v) __sync_fetch_and_sub(p, v)
#define CAS(a, ov, nv) __sync_bool_compare_and_swap(&a, ov, nv)

//------------------------------------------------------
// context

enum {
#define I(a) a,
#include "inst"
#undef I
};

struct Code {
	union {
		int i;
		void *ptr;
		Func *func;
	};
};

struct Value {
	union {
		int i;
		double d;
		const char *str;
		Task *task;
		Code *pc;
		Value *sp;
	};
};

enum ValueType {
	VT_INT,
	VT_TNIL,
	VT_FUTURE,
};

typedef void (*CodeGenFunc)(Func *, Cons *, CodeBuilder *, int sp);

struct Func {
	const char *name;
	int argc;
	const char *args[64];
	Code *code;
	CodeGenFunc codegen;
	Func *next;
};

class Context {
private:
	Func *funclist;
public:
	void *jmptable[256];

	Context();
	~Context();
	void putFunc(Func *func);
	Func *getFunc(const char *name);
	void *getDTLabel(int ins); /* direct threaded code label */
	const char *getInstName(int ins);
};

void addDefaultFuncs(Context *ctx);

//------------------------------------------------------
// task

#define TASK_STACKSIZE 256

enum TaskStat {
	TASK_RUN,
	TASK_END,
};

struct Task {
	// task info
	volatile TaskStat stat;
	Task *next;
	// exec info
	Code  *pc;
	Value *sp;
	Value stack[TASK_STACKSIZE];
};

//------------------------------------------------------
// worker thread

struct WorkerThread {
	Context *ctx;
	Scheduler *sche;
	int id;
	pthread_t pth;
};

void vmrun(Context *ctx, WorkerThread *wth, Task *task);

//------------------------------------------------------
// scheduler

class Scheduler {
private:
	Task *tl_head;
	Task *tl_tail;
	pthread_mutex_t tl_lock;
	pthread_cond_t  tl_cond;
	Task *taskpool;
	Task *freelist;
	Task dummyTask;

	WorkerThread *wthpool;

public:
	Context *ctx;
	Scheduler(Context *ctx);
	void enqueue(Task *task);
	Task *dequeue();
	Task *newTask();
	void deleteTask(Task *task);
};

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
// code generator

class CodeBuilder {
private:
	Code code[256];
	int ci;
	ValueType vtype[256];
	void addInst(int inst);
	void addInt(int n);
	void addFunc(Func *func);

public:
	Context *ctx;
	int sp;
	Func *func;

	CodeBuilder(Context *_ctx, Func *_func);
	void createIConst(int r, int i);
	void createMov(int r, int n);
	void createOp(int inst, int r, int a);
	void createIAdd(int r, int a) { createOp(INS_IADD, r, a); }
	void createISub(int r, int a) { createOp(INS_ISUB, r, a); }
	void createIMul(int r, int a) { createOp(INS_IMUL, r, a); }
	void createIDiv(int r, int a) { createOp(INS_IDIV, r, a); }
	void createIMod(int r, int a) { createOp(INS_IMOD, r, a); }
	void createINeg(int r);
	void createCall(Func *func, int shiftsfp, int rix);
	void createSpawn(Func *func, int shiftsfp, int rix);
	void createJoin(int n);
	void createRet();
	int createCondOp(int inst, int a, int b); // return label
	int createJmp();
	void setLabel(int n);

	void accept(Func *func);
};

void codegen(CodeBuilder *cb, Cons *cons, int sp);

#endif

