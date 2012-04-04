#include "lisp.h"

static const char *sstack[64];
static int sstack_idx = 0;

static bool isSpace(char ch) {
	return ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t';
}

static bool isNumber(char ch) {
	return ch >= '0' && ch <= '9';
}

Cons *parseExpr(const char *src) {
	// skip space
	while(isSpace(*src)) src++;
	if(*src == '(') {
		// S-expression
		src++;
		Cons *car = parseExpr(src);
		src = sstack[--sstack_idx];
		return newConsCar(car, parseExpr(src));
	} else if(*src == ')') {
		src++;
		sstack[sstack_idx++] = src;
		return NULL;
	} else if(isNumber(*src) || (*src == '-' && isNumber(src[1]))) {
		bool isMinus = false;
		if(*src == '-') {
			src++;
			isMinus++;
		}
		int num = 0;
		while(isNumber(*src)) {
			num *= 10;
			num += *src - '0';
			src++;
		}
		if(isMinus) num = -num;
		return newConsI(num, parseExpr(src));
	} else if(*src == '\0') {
		return NULL;
	} else {
		char str[256];
		int len = 0;
		while(!(isSpace(*src) || *src == '\0')) {
			str[len++] = *src;
			src++;
		}
		str[len] = '\0';
		char *str2 = new char[len + 1];
		strcpy(str2, str);
		return newConsS(str2, parseExpr(src));
	}
}

