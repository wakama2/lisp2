#include "lisp.h"

//------------------------------------------------------
Context::Context() {
	funclist = NULL;
#ifdef USING_THCODE
	vmrun(this, NULL, NULL); // init jmptable
#endif
	addDefaultFuncs(this);   // init funcs
}

Context::~Context() {

}

//------------------------------------------------------
void Context::putFunc(Func *func) {
	func->next = funclist;
	funclist = func;
}

Func *Context::getFunc(const char *name) {
	for(Func *f = funclist; f != NULL; f = f->next) {
		if(strcmp(f->name, name) == 0) return f;
	}
	return NULL;
}

//------------------------------------------------------
#ifdef USING_THCODE
void *Context::getDTLabel(int ins) {
	return jmptable[ins];
}
#endif

const char *Context::getInstName(int inst) {
	switch(inst) {
#define I(a) case a: return #a;
#include "inst"
#undef I
		default: return "";
	}
}

