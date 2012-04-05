#include "lisp.h"

static bool isSpace(char ch) {
	return ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t';
}

static bool isNumber(char ch) {
	return ch >= '0' && ch <= '9';
}

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

static ParseResult<int> parseInt(const char *src) {
	int num = 0;
	while(isNumber(*src)) {
		int n = *src - '0';
		num = num * 10 + n;
		src++;
	}
	return ParseResult<int>::make(src, num);
}

static ParseResult<Cons *> parseExpr2(const char *src) {
	while(isSpace(*src)) src++;
	if(*src == '(') {
		src++;
		ParseResult<Cons *> res = parseExpr2(src);
		ParseResult<Cons *> cdr = parseExpr2(res.src);
		// return <cdr.src, new Cons(car, cdr)>
		return ParseResult<Cons *>::make(cdr.src, newConsCar(res.value, cdr.value));
	} else if(*src == ')') {
		src++;
		return ParseResult<Cons *>::make(src, NULL);
	} else if(*src == '-' && isNumber(src[1])) {
		ParseResult<int> res = parseInt(src + 1);
		ParseResult<Cons *> cdr = parseExpr2(res.src);
		return ParseResult<Cons *>::make(cdr.src, newConsI(-res.value, cdr.value));
	} else if(isNumber(*src)) {
		ParseResult<int> res = parseInt(src);
		ParseResult<Cons *> cdr = parseExpr2(res.src);
		return ParseResult<Cons *>::make(cdr.src, newConsI(res.value, cdr.value));
	} else if(*src == '\0') {
		return ParseResult<Cons *>::make(src, NULL);
	} else {
		char str[256];
		int len = 0;
		while(!(isSpace(*src) || *src == ')' ||  *src == '(' || *src == '\0')) {
			str[len++] = *src;
			src++;
		}
		str[len] = '\0';
		char *str2 = new char[len + 1];
		strcpy(str2, str);
		ParseResult<Cons *> cdr = parseExpr2(src);
		return ParseResult<Cons *>::make(cdr.src, newConsS(str2, cdr.value));
	}
}

Cons *parseExpr(const char *src) {
	return parseExpr2(src).value;
}

