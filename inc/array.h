#ifndef ARRAY_H
#define ARRAY_H

//------------------------------------------------------
// array

template<class T> class ArrayBuilder {
private:
	size_t size;
	size_t capacity;
	T *data;
public:
	ArrayBuilder(int capa = 8) {
		size = 0;
		capacity = capa;
		data = new T[capacity];
	}
	~ArrayBuilder() {
		delete [] data;
	}
	void add(T v) {
		if(size == capacity) {
			capacity *= 2;
			T *newdata = new T[capacity];
			memcpy(newdata, data, sizeof(T) * size);
			delete [] data;
			data = newdata;
		}
		data[size++] = v;
	}
	T &operator [] (size_t n) {
		assert(n < size);
		return data[n];
	}
	T get(size_t n) {
		assert(n < size);
		return data[n];
	}
	T getOrElse(size_t n, T def) {
		return n < size ? data[n] : def;
	}
	int  getSize() {
		return size;
	}
	void clear() {
		size = 0;
	}
	T *toArray() {
		T *newdata = new T[size];
		memcpy(newdata, data, sizeof(T) * size);
		return newdata;
	}
	T *getPtr() {
		return data;
	}
};

#endif

