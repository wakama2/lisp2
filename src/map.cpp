#include "lisp.h"

HashMap::HashMap() {
	for(int i=0; i<MAP_MAX; i++) {
		keys[i] = NULL;
		values[i] = NULL;
	}
}

// TODO
void HashMap::put(const char *key, void *value) {
	for(int i=0; i<MAP_MAX; i++) {
		if(keys[i] != NULL && strcmp(keys[i], key) == 0) {
			values[i] = value;
			return;
		}
		if(keys[i] == NULL) {
			keys[i] = key;
			values[i] = value;
			return;
		}
	}
	fprintf(stderr, "hashmap put error");
	exit(1);
}

// TODO
void *HashMap::get(const char *key) {
	for(int i=0; i<MAP_MAX; i++) {
		if(keys[i] != NULL && strcmp(keys[i], key) == 0) {
			return values[i];
		}
	}
	return NULL;
}

