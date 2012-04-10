#include "lisp.h"
#include <readline/readline.h>
#include <readline/history.h>

//------------------------------------------------------
void Cons::print(FILE *fp) {
	bool b = false;
	Cons *self = this;
	while(self != NULL) {
		if(b) printf(" ");
		b = true;
		switch(self->type) {
		case CONS_INT: printf("%d", self->i); break;
		case CONS_STR: printf("%s", self->str); break;
		case CONS_CAR:
			fprintf(fp, "(");
			self->car->print(fp);
			fprintf(fp, ")");
			break;
		}
		self = self->cdr;
	}
}

void Cons::println(FILE *fp) {
	this->print(fp);
	fprintf(fp, "\n");
}

//------------------------------------------------------
static int eval2_getResult(Future *f) {
	WorkerThread *wth = f->wth;
	joinWorkerThread(wth);
	int res = wth->stack[0].i;
	deleteWorkerThread(wth);
	return res;
}

static void *WorkerThread_Start(void *ptr) {
	WorkerThread *wth = (WorkerThread *)ptr;
	vmrun(wth->ctx, wth);
	return NULL;
}

WorkerThread *newWorkerThread(Context *ctx, Code *pc, int argc, Value *argv) {
	ATOMIC_ADD(&ctx->threadCount, 1);
	WorkerThread *wth = new WorkerThread();
	wth->pc = pc;
	wth->fp = wth->frame;
	wth->sp = wth->stack;
	wth->ctx = ctx;
	if(argc != 0) {
		memcpy(wth->stack, argv, sizeof(argv[0]) * argc);
	}
	Future *future = &wth->future;
	future->wth = wth;
	future->getResult = eval2_getResult;
	pthread_create(&wth->pth, NULL, WorkerThread_Start, wth);
	return wth;
}

void joinWorkerThread(WorkerThread *wth) {
	pthread_join(wth->pth, NULL);
}

void deleteWorkerThread(WorkerThread *wth) {
	ATOMIC_SUB(&wth->ctx->threadCount, 1);
	delete wth;
}

//------------------------------------------------------
static void runCons(Context *ctx, Cons *cons) {
	if(cons->type == CONS_CAR) {
		for(; cons != NULL; cons = cons->cdr) {
			cons->println();
			runCons(ctx, cons->car);
		}
		return;
	}
	CodeBuilder *cb = new CodeBuilder(ctx, NULL);
	codegen(ctx, cons, cb);
	cb->createExit();
	//joinWorkerThread(wth);
	//printf("%d\n", wth->stack[0]);
	//deleteWorkerThread(wth);
	WorkerThread *wth = newWorkerThread(ctx, cb->code, 0, NULL);
	Future *f = &wth->future;
	printf("%d\n", f->getResult(f));
	delete cb;
}

//------------------------------------------------------
#define HISTFILE ".history"
static void runInteractive(Context *ctx) {
	read_history(HISTFILE);
	while(true) {
		char *in = readline(">>");
		if(in == NULL || strcmp(in, "exit") == 0 || strcmp(in, "quit") == 0) {
			break;
		}
		add_history(in);
		Cons *c = parseExpr(in).value;
		runCons(ctx, c);
		free(in);
		write_history(HISTFILE);
	}
}

static void runFromFile(Context *ctx, const char *filename) {
	FILE *fp = fopen(filename, "r");
	if(fp == NULL) return;
	char is[256];
	int size = fread(is, 1, 256, fp);
	fclose(fp);
	is[size] = '\0';

	const char *src = is;
	while(true) {
		ParseResult<Cons *> res = parseExpr(src);
		if(res.success) {
			src = res.src;
			runCons(ctx, res.value);
		} else {
			break;
		}
	}
}

//------------------------------------------------------
static Context *newContext() {
	Context *ctx = new Context();
	ctx->funcLen = 0;
	ctx->threadCount = 0;
	vmrun(ctx, NULL);
	return ctx;
}

static void deleteContext(Context *ctx) {
	delete ctx;
}

//------------------------------------------------------
int main(int argc, char **argv) {
	Context *ctx = newContext();
	if(argc >= 2) {
		runFromFile(ctx, argv[1]);
	} else {
		runInteractive(ctx);
	}
	deleteContext(ctx);
	return 0;
}

