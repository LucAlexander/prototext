#include <inttypes.h>
#include <stdlib.h>
#include <stddef.h>

#include "pool.h"
#include "hashmap.h"

#define DEBUG

#define ERROR_BUFFER 128

#define ARENA_SIZE 0x10000
#define TOKEN_MAX 64

typedef enum TOKEN {
	EXEC_TOKEN='.',
	WORD_TOKEN=':',
	SUBWORD_TOKEN=',',
	IDENTIFIER_TOKEN=1000,
	ADD_TOKEN,
	SUB_TOKEN,
	MUL_TOKEN,
	DIV_TOKEN,
	MOD_TOKEN,
	DUP_TOKEN,
	POP_TOKEN,
	OVR_TOKEN,
	SWP_TOKEN,
	NIP_TOKEN,
	ROT_TOKEN,
	CUT_TOKEN,
	RET_TOKEN,
	BRK_TOKEN,
	AND_TOKEN,
	OR_TOKEN,
	XOR_TOKEN,
	NOT_TOKEN,
	COM_TOKEN,
	U8_TOKEN,
	I8_TOKEN,
	U16_TOKEN,
	I16_TOKEN,
	U32_TOKEN,
	I32_TOKEN,
	U64_TOKEN,
	I64_TOKEN,
	JMP_TOKEN,
	JEQ_TOKEN,
	JTR_TOKEN,
	JFA_TOKEN,
	JNE_TOKEN,
	JLT_TOKEN,
	JLE_TOKEN,
	JGT_TOKEN,
	JGE_TOKEN,
	STRING_TOKEN,
	CHAR_TOKEN,
	INTEGER_TOKEN,
} TOKEN;

#define COMMENT_OPEN '('
#define COMMENT_CLOSE ')'
#define STRING_OPEN '"'
#define CHAR_OPEN '\''

typedef struct string {
	char* text;
	uint64_t size;
	uint64_t i;
} string;

typedef struct token {
	string str;
	TOKEN tag;
	uint64_t val;
	uint8_t s;
} token;

MAP_DEF(TOKEN)

typedef struct assembler {
	string input;
	pool* mem;
	token* tokens;
	TOKEN_map opmap;
	char* err;
	uint64_t token_count;
} assembler;


