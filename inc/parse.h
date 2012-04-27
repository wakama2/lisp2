#ifndef PARSE_H
#define PARSE_H

//------------------------------------------------------
// parser

class Reader {
public:
	virtual ~Reader() {}
	virtual int read() = 0;
};

class Tokenizer {
private:
	Reader *reader;
	int nextch;
	int ch;
	int linenum;
	ArrayBuilder<char> linebuf;
	ArrayBuilder<char> tokenbuf;

	int seek();

public:
	Tokenizer(Reader *r);
	bool isIntToken(int *n);
	bool isSymbol(char ch);
	bool isStrToken(const char **);
	bool isEof();
	void printErrorMsg(const char *msg);
};

#endif

