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
Tokenizer::Tokenizer(Reader r, void *rp) {
	this->reader = r;
	this->reader_p = rp;
	this->linenum = 0;
	this->cnum = 0;
	this->ch = r(rp);
	this->nextch = r(rp);
	if(ch != EOF) {
		linebuf[cnum++] = ch;
	}
}

int Tokenizer::seek() {
	ch = nextch;
	if(ch == '\n') {
		linenum++;
		cnum = 0;
	} else if(ch != EOF) {
		linebuf[cnum] = ch;
		cnum++;
	}
	nextch = reader(reader_p);
	return ch;
}

bool Tokenizer::isEof() {
	while(isSpace(ch)) seek();
	return ch == EOF;
}

bool Tokenizer::isIntToken(int *n) {
	while(isSpace(ch)) seek();
	if(ch == '-' && isNumber(nextch)) {
		seek();
		isIntToken(n);
		*n = -(*n);
		return true;
	} else if(ch == '+' && isNumber(nextch)) {
		seek();
		return isIntToken(n);
	} else if(isNumber(ch)) {
		int num = 0;
		do {
			num = num * 10 + (ch - '0');
		} while(isNumber(seek()));
		*n = num;
		return true;
	} else {
		return false;
	}
}

bool Tokenizer::isSymbol(char c) {
	while(isSpace(ch)) seek();
	if(ch == c) {
		seek();
		return true;
	} else {
		return false;
	}
}

bool Tokenizer::isStrToken(const char **strp) {
	while(isSpace(ch)) seek();
	char str[256];
	int len = 0;
	while(!(isSpace(ch) || ch == ')' || ch == '(' || ch == EOF)) {
		str[len++] = toLower(ch);
		seek();
	}
	if(len == 0) return false;
	str[len] = '\0';
	char *str2 = new char[len + 1];
	strcpy(str2, str);
	*strp = str2;
	return true;
}

void Tokenizer::printErrorMsg(const char *msg) {
	fprintf(stderr, "parse error(line=%d): %s\n", linenum, msg);
	linebuf[cnum] = '\0';
	fprintf(stderr, "n = %d\n", cnum);
	fprintf(stderr, "%s\n", linebuf);
	for(int i=0; i<cnum; i++) {
		fprintf(stderr, " ");
	}
	fprintf(stderr, "^\n");
}

static bool parseError(Tokenizer *tk, const char *msg) {
	tk->printErrorMsg(msg);
	return false;
}

bool parseCons(Tokenizer *tk, Cons **res) {
	if(tk->isSymbol('(')) {
		Cons *c = new Cons(CONS_CAR);
		if(parseCons(tk, &c->car)) {
			Cons *cc = c->car;
			while(!tk->isSymbol(')')) {
				if(parseCons(tk, &cc->cdr)) {
					cc = cc->cdr;
				} else {
					cons_free(c);
					return false;
				}
			}
		} else {
			cons_free(c);
			return false;
		}
		*res = c;
		return true;
	}
	int n;
	if(tk->isIntToken(&n)) {
		Cons *c = new Cons(CONS_INT);
		c->i = n;
		*res = c;
		return true;
	}
	const char *str;
	if(tk->isStrToken(&str)) {
		Cons *c = new Cons(CONS_STR);
		c->str = str;
		*res = c;
		return true;
	}
	return parseError(tk, "require \")\"");
}

