#ifndef LISP_H
#define LISP_H

//------------------------------------------------------
// configuration

#define WORKER_MAX 9
#define TASK_MAX   20 //(WORKER_MAX * 3)
#define TASK_STACKSIZE 1024

#define USING_THCODE

//------------------------------------------------------
// includes and structs

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
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

#define ATOMIC_ADD(p, v) __sync_fetch_and_add(p, v)
#define ATOMIC_SUB(p, v) __sync_fetch_and_sub(p, v)
#define CAS(a, ov, nv) __sync_bool_compare_and_swap(&a, ov, nv)

//------------------------------------------------------
// instruction, code, value

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
		Variable *var;
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

//------------------------------------------------------
// function

typedef void (*CodeGenFunc)(Func *, Cons *, CodeBuilder *, int sp);

struct Func {
	const char *name;
	int argc;
	const char *args[64];
	Code *code;
	CodeGenFunc codegen;
	Func *next;
};

struct Variable {
	const char *name;
	Value value;
	Variable *next;
};

//------------------------------------------------------
// context

class Context {
private:
	Func *funclist;
	Variable *varlist;

public:
#ifdef USING_THCODE
	void *jmptable[256];
#endif
	Scheduler *sche;

	Context();
	~Context();
	void putFunc(Func *func);
	Func *getFunc(const char *name);
	void putVar(Variable *var);
	Variable *getVar(const char *name);
	void *getDTLabel(int ins); /* direct threaded code label */
	const char *getInstName(int ins);
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
	// task info
	volatile TaskStat stat;
	Task *next;
	// exec info
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
	Task *newTask(Func *func, Value *args, TaskMethod dest);
	void deleteTask(Task *task);
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

	Cons(ConsType type) { this->type = type; cdr = NULL; }
};

void cons_free(Cons *cons);
void cons_print(Cons *cons, FILE *fp = stdout);
void cons_println(Cons *cons, FILE *fp = stdout);

//------------------------------------------------------
// parser

typedef int (*Reader)(void *p);

class Tokenizer {
private:
	Reader reader;
	void *reader_p;
	int nextch;
	int ch;
	int linenum;
	int cnum;
	char linebuf[256];

	int seek();

public:
	Tokenizer(Reader r, void *rp);

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
	Code code[256];
	int ci;
	void addInst(int inst);
	void addInt(int n);
	void addFunc(Func *func);
	void addVar(Variable *var);

public:
	Context *ctx;
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
	void createLoadGlobal(Variable *var, int r);
	void createStoreGlobal(Variable *var, int r);
	void createCall(Func *func, int shiftsfp, int rix);
	void createSpawn(Func *func, int shiftsfp, int rix);
	void createJoin(int n);
	void createRet();
	int createCondOp(int inst, int a, int b); // return label
	int createJmp();
	void setLabel(int n);

	void accept(Func *func);
	void codegen(Cons *cons, int sp);
};

#endif

