#ifndef PARSE_H
#define PARSE_H

//------------------------------------------------------
// reader

class Reader {
public:
	virtual ~Reader() {}
	virtual int read() = 0;
};

//------------------------------------------------------
// tokenizer

enum TokenType {
	TT_INT,
	TT_FLOAT,
	TT_STR,
	TT_OPEN,
	TT_CLOSE,
	TT_EOF,
};

class Tokenizer {
private:
	Reader *reader;
	int linenum;
	int nextch;
	ArrayBuilder<char> linebuf;
public:
	ArrayBuilder<char> tokenbuf;
	union {
		int ival;
		double fval;
	};
	const char *sval() { return tokenbuf.toArray(); }

	Tokenizer(Reader *r);
	void nextChar();
	TokenType nextToken();
	void printErrorMsg(const char *msg);
};

#endif

