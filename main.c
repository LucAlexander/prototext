#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "assembler.h"

MAP_IMPL(TOKEN)

#define assert_local(c, ...)\
	if (!(c)){\
		snprintf(assm->err, ERROR_BUFFER, "Failed local assertion " #c "\n" __VA_ARGS__);\
		return 0;\
	}

#define assert_prop()\
	if (assm->err != 0){\
		return 0;\
	}

uint8_t
lex_integer(assembler* const assm, token* const t){
	t->val = 0;
	t->tag = INTEGER_TOKEN;
	char c = assm->input.text[assm->input.i];
	if (c == '-'){
		t->str.size += 1;
		assm->input.i += 1;
		t->s = 1;
	}
	uint8_t digit = 1;
	if (c == '0'){
		assm->input.i += 1;
		t->str.size += 1;
		c = assm->input.text[assm->input.i];
		if (c == 'x'){
			assm->input.i += 1;
			t->str.size += 1;
			while (assm->input.i < assm->input.size){
				c = assm->input.text[assm->input.i];
				if (c >= '0' && c <= '9'){
					t->val += (c - 48) * digit;
				}
				else if (c >= 'A' && c <= 'F'){
					t->val += (c - 55) * digit;
				}
				else if (c >= 'a' && c <= 'f'){
					t->val += (c - 87) * digit;
				}
				else {
					t->str.size -= 1;
					return 0;
				}
				digit *= 16;
				assm->input.i += 1;
				t->str.size += 1;
			}
			return 0;
		}
		if (c == 'b'){
			assm->input.i += 1;
			t->str.size += 1;
			while (assm->input.i < assm->input.size){
				c = assm->input.text[assm->input.i];
				if (c != '0' && c != '1'){
					t->str.size -= 1;
					return 0;
				}
				t->val += (c-48) * digit;
				digit *= 2;
				assm->input.i += 1;
				t->str.size += 1;
			}
			return 0;
		}
		if (c == 'o'){
			assm->input.i += 1;
			t->str.size += 1;
			while (assm->input.i < assm->input.size){
				c = assm->input.text[assm->input.i];
				if (c < '0' && c > '7'){
					t->str.size -= 1;
					return 0;
				}
				t->val += (c-48) * digit;
				digit *= 8;
				assm->input.i += 1;
				t->str.size += 1;
			}
			return 0;
		}
	}
	while (assm->input.i < assm->input.size){
		c = assm->input.text[assm->input.i];
		if (isdigit(c) == 0){
			t->str.size -= 1;
			return 0;
		}
		t->val += (c-48)*digit;
		digit *= 10;
		assm->input.i += 1;
		t->str.size += 1;
	}
	return 0;
}

uint8_t
lex_identifier(assembler* const assm, token* const t){
	t->tag = IDENTIFIER_TOKEN;
	while (assm->input.i < assm->input.size){
		char c = assm->input.text[assm->input.i];
		if (!isalnum(c) && c != '_'){
			t->str.size -= 1;
			break;
		}
		t->str.size += 1;
		assm->input.i += 1;
	}
	char* name = t->str.text;
	uint64_t size = t->str.size;
	char save = name[size];
	name[size] = '\0';
	TOKEN* tok = TOKEN_map_access(&assm->opmap, name);
	if (tok != NULL){
		name[size] = save;
		t->tag = *tok;
		return 0;
	}
	name[size] = save;
	return 0;
}

uint8_t
lex_cstr(assembler* const assm){
	assm->token_count = 0;
	uint64_t line = 1;
	assm->tokens = pool_request(assm->mem, sizeof(token));
	while (assm->input.i < assm->input.size){
		token* t = &assm->tokens[assm->token_count];
		char c = assm->input.text[assm->input.i];
		while (assm->input.i < assm->input.size){
			c = assm->input.text[assm->input.i];
			switch (c){
			case '\n':
				line += 1;
			case ' ':
			case '\r':
			case '\t':
				assm->input.i += 1;
				continue;
			default:
			}
			break;
		}
		if (assm->input.i >= assm->input.size){
			break;
		}
		t->str.text = &assm->input.text[assm->input.i];
		t->str.size = 1;
		switch (c){
		case EXEC_TOKEN:
		case WORD_TOKEN:
		case SUBWORD_TOKEN:
			t->tag = c;
			assm->input.i += 1;
			pool_request(assm->mem, sizeof(token));
			assm->token_count += 1;
			continue;
		case COMMENT_OPEN:
			assm->input.i += 1;
			uint8_t nest = 0;
			while (assm->input.i < assm->input.size){
				c = assm->input.text[assm->input.i];
				if (c == '\n'){
					line += 1;
				}
				if (c == COMMENT_OPEN){
					nest += 1;
					assm->input.i += 1;
					continue;
				}
				else if (c == COMMENT_CLOSE){
					if (nest == 0){
						assm->input.i += 1;
						break;
					}
					assm->input.i += 1;
					nest -= 1;
					continue;
				}
				assm->input.i += 1;
			}
			continue;
		case STRING_OPEN:
			assm->input.i += 1;
			while (assm->input.i < assm->input.size){
				c = assm->input.text[assm->input.i];
				if (c == '\n'){
					line += 1;
				}
				else if (c == STRING_OPEN){
					t->str.size += 1;
					assm->input.i += 1;
					break;
				}
				t->str.size += 1;
				assm->input.i += 1;
			}
			pool_request(assm->mem, sizeof(token));
			assm->token_count += 1;
			t->tag = STRING_TOKEN;
			continue;
		case CHAR_OPEN:
			assm->input.i += 1;
			char character = assm->input.text[assm->input.i];
			assm->input.i += 1;
			c = assm->input.text[assm->input.i];
			if (character == '\\'){
				switch (c){
				case 'a': c = '\a'; break;
				case 'b': c = '\b'; break;
				case 'e': c = '\033'; break;
				case 'f': c = '\f'; break;
				case 'n': c = '\n'; break;
				case 'r': c = '\r'; break;
				case 't': c = '\t'; break;
				case 'v': c = '\v'; break;
				case '\\': c = '\\'; break;
				case '\'': c = '\''; break;
				case '"': c = '"'; break;
				case '?': c = '\?'; break;
				case '\n': line += 1; break;
				}
				assm->input.i += 1;
			}
			else{
				c = character;
			}
			t->val = c;
			t->s = 1;
			t->tag = CHAR_TOKEN;
			c = assm->input.text[assm->input.i];
			assm->input.i += 1;
			assert_local(c == CHAR_OPEN);
			pool_request(assm->mem, sizeof(token));
			assm->token_count += 1;
			continue;
		default:
		}
		if (isdigit(c) || c == '-'){
			lex_integer(assm, t);
		}
		else {
			assert_local(isalpha(c) || c == '_');
			lex_identifier(assm, t);
		}
		pool_request(assm->mem, sizeof(token));
		assm->token_count += 1;
	}
	return 0;
}

void
show_tokens(assembler* const assm){
	for (uint64_t i = 0;i<assm->token_count;++i){
		token* t = &assm->tokens[i];
		printf("[ ");
		switch (t->tag){
		case EXEC_TOKEN:
			printf("EXEC | ");
			break;
		case WORD_TOKEN:
			printf("WORD | ");
			break;
		case SUBWORD_TOKEN:
			printf("SUB | ");
			break;
		case IDENTIFIER_TOKEN:
			printf("IDEN | ");
			break;
		case ADD_TOKEN:
			printf("ADD | ");
			break;
		case SUB_TOKEN:
			printf("SUB | ");
			break;
		case MUL_TOKEN:
			printf("NUL | ");
			break;
		case DIV_TOKEN:
			printf("DIV | ");
			break;
		case MOD_TOKEN:
			printf("MOD | ");
			break;
		case DUP_TOKEN:
			printf("DUP | ");
			break;
		case POP_TOKEN:
			printf("POP | ");
			break;
		case OVR_TOKEN:
			printf("OVR | ");
			break;
		case SWP_TOKEN:
			printf("SWP | ");
			break;
		case NIP_TOKEN:
			printf("ROT | ");
			break;
		case CUT_TOKEN:
			printf("CUT | ");
			break;
		case RET_TOKEN:
			printf("RET | ");
			break;
		case BRK_TOKEN:
			printf("BRK | ");
			break;
		case AND_TOKEN:
			printf("AND | ");
			break;
		case OR_TOKEN:
			printf("OR | ");
			break;
		case XOR_TOKEN:
			printf("XOR | ");
			break;
		case NOT_TOKEN:
			printf("NOT | ");
			break;
		case COM_TOKEN:
			printf("COM | ");
			break;
		case U8_TOKEN:
			printf("MODE U8 | ");
			break;
		case I8_TOKEN:
			printf("MODE I8 | ");
			break;
		case U16_TOKEN:
			printf("MODE U16| ");
			break;
		case I16_TOKEN:
			printf("MODE I16 | ");
			break;
		case U32_TOKEN:
			printf("MODE U32 | ");
			break;
		case I32_TOKEN:
			printf("MODE I32 | ");
			break;
		case U64_TOKEN:
			printf("MODE U64 | ");
			break;
		case I64_TOKEN:
			printf("MODE I64 | ");
			break;
		case JMP_TOKEN:
			printf("JMP | ");
			break;
		case JEQ_TOKEN:
			printf("JEQ | ");
			break;
		case JTR_TOKEN:
			printf("JTR | ");
			break;
		case JFA_TOKEN:
			printf("JFA | ");
			break;
		case JNE_TOKEN:
			printf("JNE | ");
			break;
		case JLT_TOKEN:
			printf("JLT | ");
			break;
		case JLE_TOKEN:
			printf("JLE | ");
			break;
		case JGT_TOKEN:
			printf("JGT | ");
			break;
		case JGE_TOKEN:
			printf("JGE | ");
			break;
		case STRING_TOKEN:
			printf("STRING | ");
			break;
		case CHAR_TOKEN:
			printf("CHAR | (%c) ", (char)t->val);
			break;
		case INTEGER_TOKEN:
			printf("INT | ");
			break;
		default:
			printf("UNKNOWN | ");
			break;
		}
		char* name = t->str.text;
		uint64_t size = t->str.size;
		char save = name[size];
		name[size] = '\0';
		printf("'%s' ", name);
		name[size] = save;
		printf("]-");
	}
	printf("\n");
}

uint8_t
assemble_cstr(assembler* const assm){
	lex_cstr(assm);
	assert_prop();
#ifdef DEBUG
	show_tokens(assm);
#endif
	return 0;
}

void
opmap_fill(TOKEN_map* opmap){
	uint64_t opcode_count = STRING_TOKEN-ADD_TOKEN;
	TOKEN* tokens = pool_request(opmap->mem, sizeof(TOKEN)*opcode_count);
	for (uint8_t i = 0;i<opcode_count;++i){
		tokens[i] = i+ADD_TOKEN;
	}
	TOKEN_map_insert(opmap, "add", tokens++);
	TOKEN_map_insert(opmap, "sub", tokens++);
	TOKEN_map_insert(opmap, "mul", tokens++);
	TOKEN_map_insert(opmap, "div", tokens++);
	TOKEN_map_insert(opmap, "mod", tokens++);
	TOKEN_map_insert(opmap, "dup", tokens++);
	TOKEN_map_insert(opmap, "pop", tokens++);
	TOKEN_map_insert(opmap, "ovr", tokens++);
	TOKEN_map_insert(opmap, "swp", tokens++);
	TOKEN_map_insert(opmap, "nip", tokens++);
	TOKEN_map_insert(opmap, "rot", tokens++);
	TOKEN_map_insert(opmap, "cut", tokens++);
	TOKEN_map_insert(opmap, "ret", tokens++);
	TOKEN_map_insert(opmap, "brk", tokens++);
	TOKEN_map_insert(opmap, "and", tokens++);
	TOKEN_map_insert(opmap, "or", tokens++);
	TOKEN_map_insert(opmap, "xor", tokens++);
	TOKEN_map_insert(opmap, "not", tokens++);
	TOKEN_map_insert(opmap, "com", tokens++);
	TOKEN_map_insert(opmap, "u8", tokens++);
	TOKEN_map_insert(opmap, "i8", tokens++);
	TOKEN_map_insert(opmap, "u16", tokens++);
	TOKEN_map_insert(opmap, "i16", tokens++);
	TOKEN_map_insert(opmap, "u32", tokens++);
	TOKEN_map_insert(opmap, "i32", tokens++);
	TOKEN_map_insert(opmap, "u64", tokens++);
	TOKEN_map_insert(opmap, "i64", tokens++);
	TOKEN_map_insert(opmap, "jmp", tokens++);
	TOKEN_map_insert(opmap, "jeq", tokens++);
	TOKEN_map_insert(opmap, "jtr", tokens++);
	TOKEN_map_insert(opmap, "jfa", tokens++);
	TOKEN_map_insert(opmap, "jne", tokens++);
	TOKEN_map_insert(opmap, "jlt", tokens++);
	TOKEN_map_insert(opmap, "jle", tokens++);
	TOKEN_map_insert(opmap, "jgt", tokens++);
	TOKEN_map_insert(opmap, "jge", tokens++);
}

void
assemble_file(const char* input, const char* output){
	FILE* fd = fopen(input, "r");
	if (fd == NULL){
		fprintf(stderr, "File '%s' could not be opened\n", input);
		return;
	}
	pool mem = pool_alloc(ARENA_SIZE, POOL_STATIC);
	if (mem.buffer == NULL){
		fprintf(stderr, "Couldn't allocate arena\n");
		fclose(fd);
		return;
	}
	uint64_t read_bytes = fread(mem.buffer, sizeof(uint8_t), mem.left, fd);
	fclose(fd);
	if (read_bytes == mem.left){
		fprintf(stderr, "File too big\n");
		pool_dealloc(&mem);
		return;
	}
	string str = {.size=read_bytes, .i=0};
	str.text = pool_request(&mem, read_bytes);
	if (str.text == NULL){
		fprintf(stderr, "Unable to allocate buffer\n");
		pool_dealloc(&mem);
		return;
	}
	TOKEN_map opmap = TOKEN_map_init(&mem);
	opmap_fill(&opmap);
	assembler assm = {
		.input = str,
		.mem = &mem,
		.tokens = NULL,
		.opmap=opmap,
		.err = 0
	};
	assemble_cstr(&assm);
	if (assm.err != 0){
		fprintf(stderr, "%s\n", assm.err);
	}
	pool_dealloc(&mem);
}

int32_t main(int argc, char** argv){
	if (argc < 2){
		fprintf(stderr, "Use -h for help\n");
		return 0;
	}
	if (strncmp(argv[1], "-h", TOKEN_MAX) == 0){
		printf("-s [input file] -o [output file]\n");
		return 0;
	}
	if (strncmp(argv[1], "-s", TOKEN_MAX) == 0){
		if (argc < 5){
			fprintf(stderr, "Wrong number of arguments for assembler: -s input.src -o output");
			return 0;
		}
		if (strncmp(argv[3], "-o", TOKEN_MAX) != 0){
			fprintf(stderr, "Missing required flag 'o' to designate output\n");
			return 0;
		}
		assemble_file(argv[2], argv[4]);
	}
	return 0;
}
