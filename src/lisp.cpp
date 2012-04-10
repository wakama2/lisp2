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

static int getThreadId(WorkerThread *wth) {
	return (wth - wth->ctx->wth);
}

WorkerThread *newWorkerThread(Context *ctx, Code *pc, int argc, Value *argv) {
	WorkerThread *wth = NULL;

	pthread_mutex_lock(&ctx->lock);
	//ATOMIC_ADD(&ctx->threadCount, 1);
	ctx->threadCount++;

	wth = ctx->freeThread;
	if(wth != NULL) {
		ctx->freeThread = wth->next;
	}
	pthread_mutex_unlock(&ctx->lock);
	if(wth == NULL) {
		//abort();
		return NULL;
	}
	if(argc == 1) printf("[%d]: start %d\n", getThreadId(wth), argv[0].i);

	//WorkerThread *wth = new WorkerThread();
	wth->pc = pc;
	wth->fp = wth->frame;
	wth->sp = wth->stack;
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
	printf("[%d]: join\n", getThreadId(wth));
	pthread_join(wth->pth, NULL);
}

void deleteWorkerThread(WorkerThread *wth) {
	//delete wth;
	Context *ctx = wth->ctx;
	pthread_mutex_lock(&ctx->lock);
	ctx->threadCount--;
	wth->next = ctx->freeThread;
	ctx->freeThread = wth;
	printf("[%d]: exit\n", getThreadId(wth));
	pthread_mutex_unlock(&ctx->lock);
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
	ctx->freeThread = &ctx->wth[0];
	for(int i=0; i<TH_MAX; i++) {
		ctx->wth[i].ctx = ctx;
		ctx->wth[i].next = ctx->wth + i + 1;
		ctx->wth[i].parent = NULL;
	}
	ctx->wth[TH_MAX-1].next = NULL;
	pthread_mutex_init(&ctx->lock, NULL);
	vmrun(ctx, NULL); /* init jmptable */
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

