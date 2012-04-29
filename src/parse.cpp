#include "lisp.h"

//------------------------------------------------------
static bool isSpace(char ch) {
	return ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t';
}

static bool isNumber(char ch) {
	return ch >= '0' && ch <= '9';
}

static int toLower(char ch) {
	if(ch >= 'A' && ch <= 'Z') return ch - 'A' + 'a';
	return ch;
}

//------------------------------------------------------
Tokenizer::Tokenizer(Reader *r) {
	this->reader = r;
	this->linenum = 0;
	this->nextch = ' ';
}

void Tokenizer::nextChar() {
	this->nextch = reader->read();
	if(nextch == '\n') {
		linenum++;
		linebuf.clear();
	} else if(nextch != EOF) {
		linebuf.add(nextch);
	}
}

TokenType Tokenizer::nextToken() {
	while(isSpace(nextch)) nextChar();
	tokenbuf.clear();
	if(nextch == '(' || nextch == ')') {
		tokenbuf.add(nextch);
		TokenType tt = nextch == '(' ? TT_OPEN : TT_CLOSE;
		nextChar();
		return tt;
	} else if(nextch == EOF) {
		return TT_EOF;
	} else {
		while(!(isSpace(nextch) || nextch == ')' || nextch == '(' || nextch == EOF)) {
			tokenbuf.add(toLower(nextch));
			nextChar();
		}
		tokenbuf.add('\0');
		// is integer ?
		int i = 0, num = 0;
		bool isint = false;
		if(tokenbuf.getSize() > 1 && (tokenbuf[0] == '+' || tokenbuf[0] == '-')) i++;
		for(; i<tokenbuf.getSize()-1; i++) {
			if(isNumber(tokenbuf[i])) {
				num = num * 10 + (tokenbuf[i] - '0');
				isint = true;
			} else {
				isint = false;
				break;
			}
		}
		if(isint) {
			if(tokenbuf[0] == '-') num = -num;
			this->ival = num;
			return TT_INT;
		}
		return TT_STR;
	}
}

void Tokenizer::printErrorMsg(const char *msg) {
	fprintf(stderr, "parse error(line=%d): %s\n", linenum, msg);
	linebuf.add('\0');
	fprintf(stderr, "%s\n", linebuf.getPtr());
	for(int i=0, j=linebuf.getSize(); i<j; i++) {
		fprintf(stderr, " ");
	}
	fprintf(stderr, "^\n");
}

//------------------------------------------------------

bool parseCons(Tokenizer *tk, Cons **res) {
	TokenType tt = tk->nextToken();
	if(tt == TT_OPEN) {
		Cons *cons = new Cons(CONS_CAR);
		Cons **c = &cons->car;
		while(parseCons(tk, c)) {
			if(*c == NULL) {
				*res = cons;
				return true;
			}
			c = &((*c)->cdr);
		}
		cons_free(cons);
		return false;
	} else if(tt == TT_CLOSE) {
		*res = NULL;
	} else if(tt == TT_INT) {
		Cons *c = new Cons(CONS_INT);
		c->i = tk->ival;
		*res = c;
	} else if(tt == TT_FLOAT) {
		Cons *c = new Cons(CONS_FLOAT);
		c->f = tk->fval;
		*res = c;
	} else if(tt == TT_STR) {
		Cons *c = new Cons(CONS_STR);
		c->str = tk->sval();
		*res = c;
	} else if(tt == TT_EOF) {
		return false;
	}
	return true;
}

