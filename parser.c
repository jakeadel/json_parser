#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "parser.h"

#define MAX_FILE_SIZE 1000

void error(char *message, int code) {
	printf("ERROR: %s, CODE: %i\n", message, code);
	exit(code);
}

Token readString(Lexer *l) {
	l->index++; // Get to the next value (past ")
	
	Token t;
	int start = l->index;
	// check for : ", escape characters (TODO), EOF
	for (;;) {
		char c = l->buf[l->index];
		if (c == '"') {
			t.type = T_STRING;
			int size = l->index - start;
			t.val = malloc(size + 1);
			memcpy(t.val, &l->buf[start], size);
			t.val[size] = '\0';
			t.size = size + 1;
			l->index++;
			return t;
		}
		else if (c == '\0') {
			t.type = T_ERR;
			return t;
		}
		else l->index++;
	}
}

Token readNumber(Lexer *l) {
	Token t;
	int start = l->index;
	bool decimal = false;

	for (;;) {
		char c = l->buf[l->index];
		if (isdigit(c)) {
			l->index++;
		}
		else if (c == '.') {
			if (decimal) {
				t.type = T_ERR;
				t.val = NULL;
				return t;
			}
			decimal = true;
			l->index++;
		}
		else {
			t.type = T_NUMBER;
			int size = l->index - start;
			t.val = malloc(size + 1);
			memcpy(t.val, &l->buf[start], size);
			t.val[size] = '\0';
			t.size = size + 1;
			return t;
		}
	}
}

Token readLiteral(Lexer *l) {
	Token t;
	t.val = NULL;
	if (strncmp(&l->buf[l->index], "true", 4) == 0) {
		l->index += 4;
		t.type = T_TRUE;
	}
	else if (strncmp(&l->buf[l->index], "false", 5) == 0) {
		l->index += 5;
		t.type = T_FALSE;
	}
	else if (strncmp(&l->buf[l->index], "null", 4) == 0) {
		l->index += 4;
		t.type = T_NULL;
	}
	else {
		t.type = T_ERR;
	}
	return t;
}

void skipWhitespace(Lexer *l) {
	while (l->index < l->buf_size) {
		char c = l->buf[l->index];
		if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
			l->index++;
		}
		else {
			break;
		}
	}
}

/*
	Crafting a json tokenizer in C
	Feels so easy to be incorrect
	protobuf more potential to be correct and know it
*/
Token createToken(TokenType type, char *val, size_t len) {
	Token t;
	t.type = type;
	if (val && len > 0) {
		t.val = malloc(len + 1);
		memcpy(t.val, val, len);
		t.val[len] = '\0';
	}
	else {
		t.val = NULL;
	}
	return t;
}

Token readToken(Lexer *l) {
	skipWhitespace(l);
	if (l->index >= l->buf_size) return createToken(T_EOF, NULL, 0);

	char c = l->buf[l->index];
	switch (c) {
		case '{': 
			l->index++;	
			return createToken(T_OPEN_OBJ, NULL, 0);
		case '}': 
			l->index++;	
			return createToken(T_CLOSE_OBJ, NULL, 0);
		case '[': 
			l->index++;	
			return createToken(T_OPEN_ARR, NULL, 0);
		case ']':
			l->index++;	
			return createToken(T_CLOSE_ARR, NULL, 0);
		case ':':
			l->index++;	
			return createToken(T_COLON, NULL, 0);
		case ',':
			l->index++;	
			return createToken(T_COMMA, NULL, 0);
		case '"': return readString(l);
		case '\0': return createToken(T_EOF, NULL, 0);
		default:
			if (isalpha(c)) {
				return readLiteral(l);
			}
			else if (isdigit(c) || c == '-') {
				return readNumber(l);
			}
			else {
				return createToken(T_ERR, NULL, 0);
			}
	};
}

ssize_t readFile(const char *filepath, char *buf) {
	int fd;
	ssize_t num_bytes;
	if ((fd = open(filepath, 0)) < 0)
		error("Unable to open file", fd);
	if ((num_bytes = read(fd, buf, MAX_FILE_SIZE)) <= 0)
		error("Unable to read file", num_bytes);
	printf("Bytes read: %li\n", num_bytes);
	return num_bytes;
}

void printToken(Token t) {
	const char *mappings[] = {
		"STRING", "NUMBER", "TRUE", "FALSE",
		"NULL", "COLON", "COMMA", "OPEN_OBJ",
		"CLOSE_OBJ", "OPEN_ARR", "CLOSE_ARR",
		"EOF", "ERR"
	};
	printf("Token: %s, val: %s\n", mappings[t.type], t.val);
}

Value parseArr(Token tokens[], int *i, int num_tokens) {
	Value v;
	v.type = ARRAY;

	Array *a = malloc(sizeof(Array));
	a->size = 0;
	a->capacity = 4;
	a->values = malloc(sizeof(Value) * 4);

	v.u.aval = a;

	(*i)++;

	if (tokens[*i].type == T_CLOSE_ARR) {
		(*i)++;
		return v;
	}

	for (;;) {
		if (*i > num_tokens - 1) {
			error("Unexpected end of input", -1);	
		}

		TokenType type = tokens[*i].type;

		Value elem;

		if (type == T_STRING || type == T_NUMBER || type == T_TRUE ||
			type == T_FALSE || type == T_NULL || type == T_OPEN_OBJ ||
			type == T_OPEN_ARR) {

			elem = parseValue(tokens, i, num_tokens);
		}
		else {
			error("Got wrong type when parsing list", *i);
		}

		if (a->size == a->capacity) {
			a->capacity *= 2;
			a->values = realloc(a->values, sizeof(Value) * a->capacity);
		}
		a->values[a->size++] = elem;

		if (tokens[*i].type == T_CLOSE_ARR) {
			(*i)++;
			break;
		}
		else if (tokens[*i].type == T_COMMA) {
			(*i)++;
			continue;
		}
		else {
			printf("Got: ");
			printToken(tokens[*i]);
			error("Expecting a comma or ]", *i);
		}
	}
	return v;
}

Value parseObj(Token tokens[], int *i, int num_tokens) {
	Value v;
	v.type = OBJECT;

	Object *o = malloc(sizeof(Object));
	o->size = 0;
	o->capacity = 4;
	o->pairs = malloc(sizeof(KeyValue) * 4);

	v.u.oval = o;

	(*i)++;

	if (tokens[*i].type == T_CLOSE_OBJ) {
		(*i)++;
		return v;
	}

	for (;;) {
		if (*i > num_tokens - 1) {
			error("Unexpected end of input", -1);	
		}

		if (tokens[*i].type != T_STRING) {
			error("Expecting string as key in object", -1);
		}

		KeyValue kv;
		kv.key = strdup(tokens[*i].val);
		(*i)++;

		if (tokens[*i].type != T_COLON) {
			error("Expecting colon between kv pair in object", -1);
		}
		(*i)++;

		// Parse value will be the root parsing function
		// it will take in the first token of an expected value and 
		// either handle or delegate it
		kv.value = parseValue(tokens, i, num_tokens);

		if (o->size == o->capacity) {
			o->capacity *= 2;
			o->pairs = realloc(o->pairs, o->capacity * sizeof(KeyValue));
		}
		o->pairs[o->size++] = kv;

		if (tokens[*i].type == T_CLOSE_OBJ) {
			(*i)++;
			break;
		}
		else if (tokens[*i].type == T_COMMA) {
			(*i)++;
			continue;
		}
		else {
			error("Expecting a comma or } after a kv pair", -1);
		}
	}
	return v;
}

Value createString(Token tokens[], int *i) {
	Value v;
	v.type = STRING;
	v.u.sval = strdup(tokens[*i].val);
	free(tokens[*i].val);

	(*i)++;
	return v;
}

// TODO: actually need to distinguish ints and floats in Token
Value createNumber(Token tokens[], int *i) {
	Value v;
	v.type = DOUBLE;
	char *end;
	double d = strtod(tokens[*i].val, &end);

	if (*end != '\0') {
		error("Unable to parse double", -1);
	}

	v.u.dval = d;

	(*i)++;
	return v;
}

Value createNull(int *i) {
	Value v;
	v.type = NUL;

	(*i)++;
	return v;
}

Value createTrue(int *i) {
	Value v;
	v.type = BOOL;
	v.u.bval = true;

	(*i)++;
	return v;
}

Value createFalse(int *i) {
	Value v;
	v.type = BOOL;
	v.u.bval = false;

	(*i)++;
	return v;
}

/*
	To do:
	1. handle int vs float
	2. handle arrays
	3. handle eof
*/
Value parseValue(Token tokens[], int *i, int num_tokens) {
	if (*i > num_tokens - 1) {
		error("Unexpected end of input", -1);	
	}

	TokenType type = tokens[*i].type;

	switch (type) {
		case T_STRING:
			return createString(tokens, i);
		case T_NUMBER:
			return createNumber(tokens, i);
		case T_TRUE:
			return createTrue(i);
		case T_FALSE:
			return createFalse(i);
		case T_NULL:
			return createNull(i);
		case T_OPEN_OBJ:
			return parseObj(tokens, i, num_tokens);
		case T_OPEN_ARR:
			return parseArr(tokens, i, num_tokens);
		case T_EOF:
			return;
		default:
			error("We should not have gotten here in parseValue", -1);
	};
}

void printTabs(int indent) {
	for (int i = 0; i < indent; i++) printf("\t");
}

void printValue(Value v, int indent) {
	switch (v.type) {
		case STRING:
			printf("\"%s\"", v.u.sval);
			break;
		case DOUBLE:
			printf("%f", v.u.dval);
			break;
		case BOOL:
			printf(v.u.bval ? "true" : "false");
			break;
		case NUL:
			printf("null");
			break;
		case OBJECT:
			printf("{\n");
			for (int i = 0; i < v.u.oval->size; i++) {
				// Then printing pairs
				printTabs(indent+1);
				printf("\"%s\": " , v.u.oval->pairs[i].key);
				printValue(v.u.oval->pairs[i].value, indent+1);
				if (i < v.u.oval->size - 1) printf(",");
				printf("\n");
			}
			printTabs(indent);
			printf("}");
			break;
		case ARRAY:
			printf("[\n");
			for (int i = 0; i < v.u.aval->size; i++) {
				printTabs(indent+1);
				printValue(v.u.aval->values[i], indent+1);
				if (i < v.u.aval->size - 1) printf(",");
				printf("\n");
			}
			printTabs(indent);
			printf("]");
			break;
		case EOF:
			return;
		default:
			printf("<BAD VALUE>\n");
	};
}

int main(int argc, char **argv) {
	/**
		1. Read file into buffer	
			- get fd from path		
			- read bytes from fd
		2. Parse buffer into tokens
			- skip whitespace
			- read key, then look for value
		3. Do something with the tokens
		4. End goal is a navigable struct of all of the kv pairs
	*/

	const char *filepath;
	char buf[MAX_FILE_SIZE];

	if (argc < 2) 
		error("Please provide a filepath argument", -1);

	ssize_t num_bytes = readFile(argv[1], buf);
	
	Lexer l;
	l.buf = buf;
	l.buf_size = num_bytes;
	l.index = 0;

	Token tokens[1000];
	int stackPointer = 0;

	for (;;) {
		Token t = readToken(&l);
		// printToken(t);
		if (t.type == T_EOF) break;

		tokens[stackPointer++] = t;
	}

	int i = 0;
	Value v = parseValue(tokens, &i, stackPointer);
	printValue(v, 0);

	// Now from tokens create a recursive structure of Values
	//Value root = createTree(tokenStack);
	// Need to free all Token values

	// The first one is the root and then parse individual ones from there

	return 0;
}

