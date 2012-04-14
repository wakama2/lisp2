#include "lisp.h"

//------------------------------------------------------
Context::Context() {
	funclist = NULL;
	varlist = NULL;
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
void Context::putVar(Variable *var) {
	var->next = varlist;
	varlist = var;
}

Variable *Context::getVar(const char *name) {
	for(Variable *v = varlist; v != NULL; v = v->next) {
		if(strcmp(v->name, name) == 0) return v;
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

