#include <inttypes.h>
#include <stdlib.h>
#include <stddef.h>

#include "pool.h"
#include "hashmap.h"

#define DEBUG

#define ERROR_BUFFER 128

#define ARENA_SIZE 0x10000
#define TOKEN_MAX 64
#define MAX_PUSH_SIZE 128

#define INITIAL_TARGET_SIZE 0x1000

typedef enum TOKEN {
	EXEC_TOKEN='.',
	WORD_TOKEN=':',
	SUBWORD_TOKEN=',',
	REFERENCE_TOKEN=';',
	IDENTIFIER_TOKEN=1000,
	WRITE_TOKEN,
	ADD_TOKEN,
	SUB_TOKEN,
	MUL_TOKEN,
	DIV_TOKEN,
	MOD_TOKEN,
	SHR_TOKEN,
	SHL_TOKEN,
	DUP_TOKEN,
	POP_TOKEN,
	OVR_TOKEN,
	SWP_TOKEN,
	NIP_TOKEN,
	ROT_TOKEN,
	CUT_TOKEN,
	RET_TOKEN,
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
	uint64_t val;
	TOKEN tag;
	uint8_t s;
} token;

typedef enum MODE {
	MODE_U8,
	MODE_I8,
	MODE_U16,
	MODE_I16,
	MODE_U32,
	MODE_I32,
	MODE_U64,
	MODE_I64
} MODE;

typedef struct instruction {
	union {
		struct {
			uint64_t bytes;
			MODE mode;
			uint8_t ref;
			char* label;
			uint64_t size;
		} push;
		struct {
			char* label;
			uint64_t size;
			uint64_t pointer;
		} exec;
		TOKEN opcode;
	} data;
	enum {
		PUSH_INST,
		EXEC_INST,
		OPCODE_INST
	} tag;
} instruction;

typedef struct pointer_thunk pointer_thunk;
typedef struct pointer_thunk {
	pointer_thunk* next;
	union {
		uint64_t resolved_location;
		uint64_t* waiting_pointer;
	} data;
	enum {
		THUNK_RESOLVED,
		THUNK_WAITING
	} tag;
} pointer_thunk;

MAP_DEF(uint64_t)
MAP_DEF(pointer_thunk)

void pointer_thunk_request(pointer_thunk_map* map, char* const name, uint64_t* pointer);
void pointer_thunk_fulfill(pointer_thunk_map* map, char* const name, uint64_t pointer);

typedef struct label_assoc label_assoc;
typedef struct label_assoc {
	label_assoc* next;
	char* label;
	uint64_t size;
	uint64_t instruction;
	enum {
		WORD_LABEL,
		SUBWORD_LABEL
	} tag;
} label_assoc;

typedef struct parser {
	uint64_t_map labels;
	label_assoc* label_order;
	pointer_thunk_map thunks;
	char* super_label;
	uint64_t super_label_size;
	instruction* instructions;
	uint64_t instruction_count;
	uint64_t instruction_cap;
} parser;

typedef struct target {
	char* text;
	char* header;
	uint64_t size;
	uint64_t cap;
	uint64_t header_cap;
} target;

MAP_DEF(TOKEN)

typedef struct assembler {
	parser parse;
	target code;
	string input;
	pool* mem;
	token* tokens;
	uint64_t token_count;
	TOKEN_map opmap;
	char* err;
} assembler;


