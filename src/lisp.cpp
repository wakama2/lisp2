#include "lisp.h"
#include <readline/readline.h>
#include <readline/history.h>

//------------------------------------------------------
void cons_free(Cons *cons) {
	if(cons->type == CONS_CAR && cons->car != NULL) {
		cons_free(cons->car);
	} else if(cons->type == CONS_STR) {
		delete [] cons->str;
	}
	if(cons->cdr != NULL) {
		cons_free(cons->cdr);
	}
	delete cons;
}

void cons_print(Cons *cons, FILE *fp) {
	bool b = false;
	for(; cons != NULL; cons = cons->cdr) {
		if(b) printf(" ");
		b = true;
		switch(cons->type) {
		case CONS_INT: printf("%d", cons->i); break;
		case CONS_STR: printf("%s", cons->str); break;
		case CONS_CAR:
			fprintf(fp, "(");
			cons_print(cons->car, fp);
			fprintf(fp, ")");
			break;
		}
	}
}

void cons_println(Cons *cons, FILE *fp) {
	cons_print(cons, fp);
	fprintf(fp, "\n");
}

//------------------------------------------------------
static pthread_mutex_t g_lock;
static pthread_cond_t  g_cond;

static void notify_runCons(Task *task, WorkerThread *wth) {
	pthread_mutex_lock(&g_lock);
	pthread_cond_signal(&g_cond);
	pthread_mutex_unlock(&g_lock);
}

//------------------------------------------------------
static void runCons(Context *ctx, Cons *cons) {
	Func *func = new Func();
	func->name = "__script";
	func->argc = 0;
	CodeBuilder *cb = new CodeBuilder(ctx, func);
	cb->codegen(cons, 0);
	cb->createRet(0);
	func->code = cb->getCode();

	Scheduler *sche = ctx->sche;
	Task *task = sche->newTask(func, NULL, notify_runCons);
	assert(task != NULL);
	pthread_mutex_lock(&g_lock);
	sche->enqueue(task);
	pthread_cond_wait(&g_cond, &g_lock);
	pthread_mutex_unlock(&g_lock);

	printf("%ld\n", task->stack[0].i);
	sche->deleteTask(task);
	delete [] func->code;
	delete func;
	delete cb;
}

//------------------------------------------------------
static void compileAndRun(Context *ctx, Reader *r) {
	Tokenizer tk(r);
	Cons *res;
	while(!tk.isEof()) {
		if(parseCons(&tk, &res)) {
			res->cdr = NULL;
			//cons_println(res);
			runCons(ctx, res);
			cons_free(res);
		} else {
			break;
		}
	}
}

//------------------------------------------------------
#define HISTFILE "history"

class CharReader : public Reader {
private:
	const char *in;
public:
	CharReader(const char *in) { this->in = in; }
	int read() { return *in != '\0' ? *in++ : EOF; }
};

static void runInteractive(Context *ctx) {
	// init readline
	read_history(HISTFILE);
	// start
	while(true) {
		char *in = readline(">>");
		if(in == NULL || strcmp(in, "exit") == 0 || strcmp(in, "quit") == 0) {
			break;
		}
		CharReader r(in);
		compileAndRun(ctx, &r);
		if(strlen(in) > 0) {
			add_history(in);
			write_history(HISTFILE);
		}
		free(in);
	}
}

//------------------------------------------------------
class FileReader : public Reader {
private:
	FILE *fp;
public:
	FileReader(FILE *fp) { this->fp = fp; }
	int read() { return fgetc(fp); }
};

static void runFromFile(Context *ctx, const char *filename) {
	FILE *fp = fopen(filename, "r");
	if(fp == NULL) {
		fprintf(stderr, "file open error: %s\n", filename);
		return;
	}
	FileReader fr(fp);
	compileAndRun(ctx, &fr);
	fclose(fp);
}

//------------------------------------------------------
int main(int argc, char **argv) {
	Context *ctx = new Context();
	pthread_mutex_init(&g_lock, NULL);
	pthread_cond_init(&g_cond, NULL);
	const char *fname = NULL;
	for(int i=1; i<argc; i++) {
		if(strcmp(argv[i], "-i") == 0) {
			ctx->flagShowIR = true;
		} else {
			fname = argv[i];
		}
	}
	if(fname != NULL) {
		runFromFile(ctx, fname);
	} else {
		runInteractive(ctx);
	}
	delete ctx;
	return 0;
}

