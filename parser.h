#include <stdbool.h>

#define MAX_STRING_SIZE 100
#define MAX_NESTING 64


typedef enum Type {
	INT,
	DOUBLE,
	STRING,
	BOOL,
	NUL,
	OBJECT,
	ARRAY
} Type;

typedef enum {
	T_STRING, T_NUMBER, T_TRUE, T_FALSE,
	T_NULL, T_COLON, T_COMMA, T_OPEN_OBJ,
	T_CLOSE_OBJ, T_OPEN_ARR, T_CLOSE_ARR,
	T_EOF, T_ERR
} TokenType;

typedef struct {
	TokenType type;
	char *val;
	size_t size;
} Token;

typedef struct {
	char *buf;
	size_t buf_size;
	int index;
} Lexer;

typedef struct Object Object;
typedef struct Array Array;

typedef struct {
	Type type;
	union {
		int ival;
		double dval;
		bool bval;
		char *sval;
		Object *oval;
		Array *aval;
	} u;
} Value;

typedef struct {
	char *key;
	Value value;
} KeyValue;

struct Object {
	KeyValue *pairs;
	size_t size;
	size_t capacity;
};

struct Array {
	Value *values;
	size_t size;
	size_t capacity;
};

Value parseValue(Token tokens[], int *i, int num_tokens);