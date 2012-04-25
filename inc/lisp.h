#ifndef LISP_H
#define LISP_H

//------------------------------------------------------
// configuration

#define WORKER_MAX 8
#define TASK_MAX   (WORKER_MAX * 3 / 2)
#define TASKQUEUE_MAX   32 /* must be 2^n */
#define TASK_STACKSIZE 1024

#define USING_THCODE

//------------------------------------------------------
// includes and structs

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <pthread.h>

struct Code;
struct Value;
struct Func;
struct Variable;
struct Cons;
struct WorkerThread;
struct Task;
class Scheduler;
class Context;
class CodeBuilder;

//------------------------------------------------------
// atomic function

#define ATOMIC_ADD(p, v) __sync_fetch_and_add(&(p), v)
#define ATOMIC_SUB(p, v) __sync_fetch_and_sub(&(p), v)
#define CAS(a, ov, nv) __sync_bool_compare_and_swap(&(a), ov, nv)

//------------------------------------------------------
// instruction, code, value

enum Instructions {
#define I(a) a,
#include "inst"
#undef I
	INS_COUNT,
};

struct Code {
	union {
		int64_t i;
		void *ptr;
		Func *func;
		Variable *var;
	};
};

struct Value {
	union {
		int64_t i;
		double d;
		const char *str;
		Task *task;
		Code *pc;
		Value *sp;
	};
};

//------------------------------------------------------
// function, variable

typedef void (*CodeGenFunc)(Func *, Cons *, CodeBuilder *, int sp);

struct Func {
	Code *code;
	const char *name;
	int argc;
	const char **args;
	CodeGenFunc codegen;
	Func *next;
};

struct Variable {
	Value value;
	const char *name;
	Variable *next;
};

//------------------------------------------------------
// context

class Context {
private:
	Func *funclist;
	Variable *varlist;

public:
	Scheduler *sche;
	bool flagShowIR;

	Context();
	~Context();
	void putFunc(Func *func);
	Func *getFunc(const char *name);
	void putVar(Variable *var);
	Variable *getVar(const char *name);
	const char *getInstName(int ins);
#ifdef USING_THCODE
	void *jmptable[INS_COUNT];
	void *getDTLabel(int ins); /* direct threaded code label */
#endif
};

void addDefaultFuncs(Context *ctx);

//------------------------------------------------------
// task

typedef void (*TaskMethod)(Task *task, WorkerThread *wth);

enum TaskStat {
	TASK_RUN,
	TASK_END,
};

struct Task {
	volatile TaskStat stat;
	Task *next;
	Code  *pc;
	Value *sp;
	TaskMethod dest;
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
	Context *ctx;
	Task **taskq;
	int taskEnqIndex;
	int taskDeqIndex;
	pthread_mutex_t tl_lock;
	pthread_cond_t  tl_cond;
	Task *taskpool;
	Task *freelist;
	volatile bool dead_flag;
	WorkerThread *wthpool;

public:
	Scheduler(Context *ctx);
	~Scheduler();
	void enqueue(Task *task);
	Task *dequeue();
	Task *newTask(Func *func, Value *args, TaskMethod dest);
	void deleteTask(Task *task);
	Context *getCtx() { return ctx; }
};

//------------------------------------------------------
// cons cell

enum ConsType {
	CONS_INT,
	CONS_STR,
	CONS_CAR,
};

struct Cons {
	ConsType type;
	union {
		int i;
		const char *str;
		Cons *car;
	};
	Cons *cdr;

	Cons(ConsType type) { this->type = type; car = NULL; cdr = NULL; }
};

void cons_free(Cons *cons);
void cons_print(Cons *cons, FILE *fp = stdout);
void cons_println(Cons *cons, FILE *fp = stdout);

//------------------------------------------------------
// parser

class Reader {
public:
	virtual ~Reader() {}
	virtual int read() = 0;
};

class Tokenizer {
private:
	Reader *reader;
	int nextch;
	int ch;
	int linenum;
	int cnum;
	char linebuf[256];

	int seek();

public:
	Tokenizer(Reader *r);
	bool isIntToken(int *n);
	bool isSymbol(char ch);
	bool isStrToken(const char **);
	bool isEof();
	void printErrorMsg(const char *msg);
};

bool parseCons(Tokenizer *tk, Cons **res);

//------------------------------------------------------
// code generator

class CodeBuilder {
private:
	Context *ctx;
	Func *func;
	Code code[256];
	int ci;
	void addInst(int inst);

public:
	CodeBuilder(Context *_ctx, Func *_func);
	void createIConst(int r, int i);
	void createMov(int r, int n);
	void createOp(int inst, int r, int a);
	void createIAdd(int r, int a) { createOp(INS_IADD, r, a); }
	void createISub(int r, int a) { createOp(INS_ISUB, r, a); }
	void createIMul(int r, int a) { createOp(INS_IMUL, r, a); }
	void createIDiv(int r, int a) { createOp(INS_IDIV, r, a); }
	void createIMod(int r, int a) { createOp(INS_IMOD, r, a); }
	void createIAddC(int r, int a) { createOp(INS_IADDC, r, a); }
	void createISubC(int r, int a) { createOp(INS_ISUBC, r, a); }
	void createINeg(int r);
	void createLoadGlobal(Variable *var, int r);
	void createStoreGlobal(Variable *var, int r);
	void createCall(Func *func, int shiftsfp, int rix);
	void createSpawn(Func *func, int shiftsfp, int rix);
	void createJoin(int n);
	void createRet();
	int createCondOp(int inst, int a, int b); // return label
	int createJmp();
	void setLabel(int n);

	Code *getCode();
	void codegen(Cons *cons, int sp);
	Context *getCtx() { return ctx; }
	Func *getFunc()   { return func; }
};

#endif

