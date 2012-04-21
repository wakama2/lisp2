#include "lisp.h"

CircularArray::CircularArray(int capa) {
	this->capa = capa;
	this->tasks = new Task *[1 << capa];
}

CircularArray::~CircularArray() {
	delete [] tasks;
}

CircularArray *CircularArray::resize(int bottom, int top) {
	CircularArray *newTask = new CircularArray(capa + 1);
	for(int i=top; i<bottom; i++) {
		newTask->put(i, get(i));
	}
	return newTask;
}

#define LOG_CAPACITY 4

UnboundedDEQueue::UnboundedDEQueue() {
	this->tasks = new CircularArray(LOG_CAPACITY);
	this->top = 0;
	this->bottom = 0;
}

UnboundedDEQueue::~UnboundedDEQueue() {
	delete tasks;
}

bool UnboundedDEQueue::isEmpty() {
	int lt = top;
	int lb = bottom;
	return lb <= lt;
}

void UnboundedDEQueue::pushBottom(Task *task) {
	int oldbottom = bottom;
	int oldtop = top;
	CircularArray *cur = tasks;
	int size = oldbottom - oldtop;
	if(size >= cur->capacity() - 1) {
		cur = cur->resize(oldbottom, oldtop);
		tasks = cur;
		printf("resized\n");
	}
	cur->put(oldbottom, task);
	bottom = oldbottom + 1;
}

Task *UnboundedDEQueue::popTop() {
	int oldtop = top;
	int newtop = oldtop + 1;
	int oldbottom = bottom;
	CircularArray *cur = tasks;
	int size = oldbottom - oldtop;
	if(size <= 0) {
		return NULL;
	}
	Task *task = cur->get(oldtop);
	if(CAS(top, oldtop, newtop)) {
		return task;
	}
	return NULL;
}

Task *UnboundedDEQueue::popBottom() {
	CircularArray *cur = tasks;
	bottom--;
	int oldtop = top;
	int newtop = oldtop + 1;
	int size = bottom - oldtop;
	if(size < 0) {
		bottom = oldtop;
		return NULL;
	}
	Task *task = cur->get(bottom);
	if(size > 0) {
		return task;
	}
	if(!CAS(top, oldtop, newtop)) {
		task = NULL;
	}
	bottom = oldtop + 1;
	return task;
}

