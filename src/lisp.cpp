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
		case CONS_FLOAT: printf("%lf", cons->f); break;
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
static void runCons(Context *ctx, Cons *cons) {
	Func *func = new Func();
	func->name = "__script";
	func->argc = 0;
	try {
		CodeBuilder cb(ctx, func, true);
		ValueType ty = codegen(cons, &cb, 0);
		if(ty == VT_INT) {
			cb.createPrintInt(0);
		} else if(ty == VT_BOOLEAN) {
			cb.createPrintBoolean(0);
		}
		cb.createRet(0);
		func->code = cb.getCode();
#ifdef USING_THCODE
		func->thcode = func->code;
#endif
		Scheduler *sche = ctx->sche;
		Task *task = sche->newTask(func, NULL);
		assert(task != NULL);
		sche->enqueueWaitFor(task);
		sche->deleteTask(task);
		delete [] func->code;
	} catch(char *str) {
	}
	delete func;
}

//------------------------------------------------------
static void compileAndRun(Context *ctx, Reader *r) {
	Tokenizer tk(r);
	Cons *res;
	while(parseCons(&tk, &res)) {
		if(res != NULL) {
			res->cdr = NULL;
			//cons_println(res);
			runCons(ctx, res);
			cons_free(res);
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
			if(in != NULL) free(in);
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
	const char *fname = NULL;
	for(int i=1; i<argc; i++) {
		if(strcmp(argv[i], "-i") == 0) {
			ctx->flagShowIR = true;
		} else if(strcmp(argv[i], "-inline") == 0) {
			i++;
			ctx->inlinecount = atoi(argv[i]);
		} else if(strcmp(argv[i], "-worker") == 0) {
			i++;
			int n = atoi(argv[i]);
			if(n >= 1 && n < 20) {
				ctx->workers = n;
			} else {
				fprintf(stderr, "error\n");
				exit(1);
			}
		} else {
			fname = argv[i];
		}
	}
	ctx->sche->initWorkers();
	if(fname != NULL) {
		runFromFile(ctx, fname);
	} else {
		runInteractive(ctx);
	}
	delete ctx;
	return 0;
}

