#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "assembler.h"

MAP_IMPL(TOKEN)
MAP_IMPL(uint64_t)
MAP_IMPL(pointer_thunk)

#define assert_local(c, ...)\
	if (!(c)){\
		snprintf(assm->err, ERROR_BUFFER, "Failed local assertion " #c "\n" __VA_ARGS__);\
		return 0;\
	}

#define assert_prop()\
	if (*assm->err != 0){\
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
	uint64_t last = 0;
	if (c == '0'){
		assm->input.i += 1;
		t->str.size += 1;
		c = assm->input.text[assm->input.i];
		if (c == 'x'){
			assm->input.i += 1;
			t->str.size += 1;
			while (assm->input.i < assm->input.size){
				last = t->val;
				t->val <<= 4;
				c = assm->input.text[assm->input.i];
				if (c >= '0' && c <= '9'){
					t->val += (c - 48);
				}
				else if (c >= 'A' && c <= 'F'){
					t->val += (c - 55);
				}
				else if (c >= 'a' && c <= 'f'){
					t->val += (c - 87);
				}
				else {
					t->val = last;
					t->str.size -= 1;
					return 0;
				}
				assm->input.i += 1;
				t->str.size += 1;
			}
			return 0;
		}
		if (c == 'b'){
			assm->input.i += 1;
			t->str.size += 1;
			while (assm->input.i < assm->input.size){
				last = t->val;
				t->val <<= 1;
				c = assm->input.text[assm->input.i];
				if (c != '0' && c != '1'){
					t->val = last;
					t->str.size -= 1;
					return 0;
				}
				t->val += (c-48);
				assm->input.i += 1;
				t->str.size += 1;
			}
			return 0;
		}
		if (c == 'o'){
			assm->input.i += 1;
			t->str.size += 1;
			while (assm->input.i < assm->input.size){
				last = t->val;
				t->val <<= 3;
				c = assm->input.text[assm->input.i];
				if (c < '0' && c > '7'){
					t->val = last;
					t->str.size -= 1;
					return 0;
				}
				t->val += (c-48);
				assm->input.i += 1;
				t->str.size += 1;
			}
			return 0;
		}
	}
	while (assm->input.i < assm->input.size){
		last = t->val;
		t->val *= 10;
		c = assm->input.text[assm->input.i];
		if (isdigit(c) == 0){
			t->val = last;
			t->str.size -= 1;
			return 0;
		}
		t->val += (c-48);
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
		case REFERENCE_TOKEN:
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
			t->str.text = &assm->input.text[assm->input.i];
			while (assm->input.i < assm->input.size){
				c = assm->input.text[assm->input.i];
				if (c == '\n'){
					line += 1;
				}
				else if (c == STRING_OPEN){
					t->str.size -= 1;
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
			if (c == '\n'){
				line += 1;
			}
			if (character == '\\'){
				switch (c){
				case 'a': c = '\a'; break;
				case 'b': c = '\b'; break;
				case 'e': c = '\033'; break;
				case 'f': c = '\f'; break;
				case 'r': c = '\r'; break;
				case 't': c = '\t'; break;
				case 'v': c = '\v'; break;
				case '\\': c = '\\'; break;
				case '\'': c = '\''; break;
				case '"': c = '"'; break;
				case '?': c = '\?'; break;
				case 'n': c = '\n'; break;
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
	printf("TOKENS:\n");
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
		case REFERENCE_TOKEN:
			printf("REF | ");
			break;
		case IDENTIFIER_TOKEN:
			printf("IDEN | ");
			break;
		case WRITE_TOKEN:
			printf("WRITE | ");
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
		case SHR_TOKEN:
			printf("SHR | ");
			break;
		case SHL_TOKEN:
			printf("SHL | ");
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
			printf("NIP | ");
			break;
		case ROT_TOKEN:
			printf("ROT | ");
			break;
		case CUT_TOKEN:
			printf("CUT | ");
			break;
		case RET_TOKEN:
			printf("RET | ");
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
			printf("MODE U16 | ");
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
			printf("INT | (%lx) ", t->val);
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

void pointer_thunk_request(pointer_thunk_map* map, char* const name, uint64_t* pointer){
	pointer_thunk* res = pointer_thunk_map_access(map, name);
	if (res == NULL){
		res = pool_request(map->mem, sizeof(pointer_thunk));
		res->tag = THUNK_WAITING;
		res->next = NULL;
		res->data.waiting_pointer = pointer;
		uint64_t len = strlen(name);
		char* namecopy = pool_request(map->mem, len);
		strncpy(namecopy, name, len);
		pointer_thunk_map_insert(map, namecopy, res);
		return;
	}
	if (res->tag == THUNK_RESOLVED){
		*pointer = res->data.resolved_location;
		return;
	}
	pointer_thunk* temp = res->next;
	res->next = pool_request(map->mem, sizeof(pointer_thunk));
	res->next->next = temp;
	res->next->tag = THUNK_WAITING;
	res->next->data.waiting_pointer = pointer;
	return;
}

void pointer_thunk_fulfill(pointer_thunk_map* map, char* const name, uint64_t pointer){
	pointer_thunk* res = pointer_thunk_map_access(map, name);
	if (res == NULL){
		res = pool_request(map->mem, sizeof(pointer_thunk));
		res->tag = THUNK_RESOLVED;
		res->next = NULL;
		res->data.resolved_location = pointer;
		uint64_t len = strlen(name);
		char* namecopy = pool_request(map->mem, len);
		strncpy(namecopy, name, len);
		pointer_thunk_map_insert(map, namecopy, res);
		return;
	}
	if (res->tag == THUNK_RESOLVED){
		res->data.resolved_location = pointer;
		return;
	}
	while (res != NULL){
		*res->data.waiting_pointer = pointer;
		res->tag = THUNK_RESOLVED;
		res = res->next;
	}
}

uint8_t
check_label_bucket(assembler* const assm, pointer_thunk_map_bucket* bucket){
	if (bucket->tag == BUCKET_EMPTY){
		return 0;
	}
	assert_local(bucket->value->tag == THUNK_RESOLVED, "Unresolved label reference '%s'\n", bucket->key);
	if (bucket->left != NULL){
		check_label_bucket(assm, bucket->left);
	}
	if (bucket->right != NULL){
		check_label_bucket(assm, bucket->right);
	}
	return 0;
}

uint8_t
unresolved_labels(assembler* const assm){
	HASHMAP_ITERATE(i){
		pointer_thunk_map_bucket* bucket = &assm->parse.thunks.buckets[i];
		check_label_bucket(assm, bucket);
		assert_prop();
	}
	return 0;
}

uint8_t
parse_tokens(assembler* const assm){
	assm->parse.instruction_cap = 2*assm->token_count;
	assm->parse.instructions = pool_request(assm->mem, sizeof(instruction)*2*assm->parse.instruction_cap);
	assm->parse.label_order = pool_request(assm->mem, sizeof(label_assoc));
	assm->parse.label_order->next = NULL;
	assm->parse.label_order->label = NULL;
	assm->parse.label_order->instruction = 0;
	label_assoc* current_assoc = assm->parse.label_order;
	uint8_t mode = MODE_U8;
	uint64_t i = 0;
	uint8_t broke = 1;
	while (i < assm->token_count){
		if (assm->parse.instruction_count == assm->parse.instruction_cap){
			uint64_t new_cap = assm->parse.instruction_cap * 2;
			instruction* new = pool_request(assm->mem, sizeof(instruction)*new_cap);
			for (uint64_t k = 0;k<assm->parse.instruction_cap;++k){
				new[i] = assm->parse.instructions[k];
			}
			assm->parse.instruction_cap = new_cap;
			assm->parse.instructions = new;
		}
		instruction* inst = &assm->parse.instructions[assm->parse.instruction_count];
		token t = assm->tokens[i];
		i += 1;
		switch (t.tag){
		case EXEC_TOKEN:
			assert_local(0, "Unexpected .\n");
		case WORD_TOKEN:
			assert_local(0, "Unexpected :\n");
		case SUBWORD_TOKEN:
			assert_local(0, "Unexpected ,\n");
		case REFERENCE_TOKEN:
			token next_iden = assm->tokens[i];
			assert_local(next_iden.tag == IDENTIFIER_TOKEN);
			i += 1;
			uint64_t size = next_iden.str.size;
			char* subname = pool_request(assm->mem, assm->parse.super_label_size+size+1);
			strncpy(subname, assm->parse.super_label, assm->parse.super_label_size);
			strncat(subname, next_iden.str.text, size);
			subname[assm->parse.super_label_size+size] = '\0';
			inst->tag = PUSH_INST;
			inst->data.push.mode = MODE_U64;
			inst->data.push.ref = 1;
			inst->data.push.label = subname;
			inst->data.push.size = size;
			pointer_thunk_request(&assm->parse.thunks, subname, &inst->data.push.bytes);
			assm->parse.instruction_count += 1;
			continue;
		case IDENTIFIER_TOKEN:
			token next = assm->tokens[i];
			if (next.tag == EXEC_TOKEN){
				i += 1;	
				inst->tag = EXEC_INST;
				char* name = t.str.text;
				uint64_t size = t.str.size;
				char save = name[size];
				name[size] = '\0';
				inst->data.exec.label = pool_request(assm->mem, size*sizeof(char));
				strncpy(inst->data.exec.label, name, size);
				inst->data.exec.size = size;
				pointer_thunk_request(&assm->parse.thunks, name, &inst->data.exec.pointer);
				name[size] = save;
			}
			else if (next.tag == WORD_TOKEN){
				assert_local(broke == 1);
				broke = 0;
				i += 1;	
				char* name = t.str.text;
				uint64_t size = t.str.size;
				char save = name[size];
				name[size] = '\0';
				uint64_t* index = pool_request(assm->mem, sizeof(uint64_t));
				*index = assm->parse.instruction_count;
				uint8_t dup = uint64_t_map_insert(&assm->parse.labels, name, index);
				assert_local(dup == 0, " Duplicate label '%s'\n", name);
				pointer_thunk_fulfill(&assm->parse.thunks, name, *index);
				current_assoc->label = pool_request(assm->mem, (size+1) * sizeof(char));
				strncpy(current_assoc->label, name, size);
				current_assoc->size = size;
				current_assoc->instruction = *index;
				current_assoc->next = pool_request(assm->mem, sizeof(label_assoc));
				current_assoc->tag = WORD_LABEL;
				current_assoc = current_assoc->next;
				name[size] = save;
				assm->parse.super_label = name;
				assm->parse.super_label_size = size;
				continue;
			}
			else if (next.tag == SUBWORD_TOKEN){
				i += 1;
				uint64_t size = t.str.size;
				assert_local(assm->parse.super_label != NULL);
				char* subname = pool_request(assm->mem, assm->parse.super_label_size+size+1);
				strncpy(subname, assm->parse.super_label, assm->parse.super_label_size);
				strncat(subname, t.str.text, size);
				subname[assm->parse.super_label_size+size] = '\0';
				uint64_t* index = pool_request(assm->mem, sizeof(uint64_t));
				*index = assm->parse.instruction_count;
				uint8_t dup = uint64_t_map_insert(&assm->parse.labels, subname, index);
				assert_local(dup == 0, " Duplicate label '%s'\n", subname);
				pointer_thunk_fulfill(&assm->parse.thunks, subname, *index);
				current_assoc->label = subname;
				current_assoc->size = assm->parse.super_label_size+size;
				current_assoc->instruction = *index;
				current_assoc->next = pool_request(assm->mem, sizeof(label_assoc));
				current_assoc->tag = SUBWORD_LABEL;
				current_assoc = current_assoc->next;
				continue;
			}
			else{
				inst->tag = PUSH_INST;
				char* name = t.str.text;
				uint64_t size = t.str.size;
				char save = name[size];
				name[size] = '\0';
				pointer_thunk_request(&assm->parse.thunks, name, &inst->data.push.bytes);
				name[size] = save;
				inst->data.push.mode = mode;
				inst->data.push.ref = 0;
				inst->data.push.label = NULL;
			}
			assm->parse.instruction_count += 1;
			continue;
		case U8_TOKEN:
			mode = MODE_U8;
			continue;
		case I8_TOKEN:
			mode = MODE_I8;
			continue;
		case U16_TOKEN:
			mode = MODE_U16;
			continue;
		case I16_TOKEN:
			mode = MODE_I16;
			continue;
		case U32_TOKEN:
			mode = MODE_U32;
			continue;
		case I32_TOKEN:
			mode = MODE_I32;
			continue;
		case U64_TOKEN:
			mode = MODE_U64;
			continue;
		case I64_TOKEN:
			mode = MODE_I64;
			continue;
		case RET_TOKEN:
			broke = 1;
		case WRITE_TOKEN:
		case ADD_TOKEN: case SUB_TOKEN: case MUL_TOKEN: case DIV_TOKEN:
		case MOD_TOKEN: case SHR_TOKEN: case SHL_TOKEN:
		case DUP_TOKEN: case POP_TOKEN: case OVR_TOKEN: case SWP_TOKEN:
		case NIP_TOKEN: case ROT_TOKEN: case CUT_TOKEN:
		case AND_TOKEN: case OR_TOKEN: case XOR_TOKEN: case NOT_TOKEN:
		case COM_TOKEN:
		case JMP_TOKEN: case JEQ_TOKEN: case JTR_TOKEN: case JFA_TOKEN:
		case JNE_TOKEN: case JLT_TOKEN: case JLE_TOKEN: case JGT_TOKEN:
		case JGE_TOKEN:
			token next_dot = assm->tokens[i];
			if (next_dot.tag == EXEC_TOKEN){
				i += 1;
			}
			inst->tag = OPCODE_INST;
			inst->data.opcode = t.tag;
			assm->parse.instruction_count += 1;
			continue;
		case STRING_TOKEN:
			for (uint64_t c = t.str.size;c>0;--c){
				inst->tag = PUSH_INST;
				inst->data.push.bytes = t.str.text[c-1];
				inst->data.push.mode = MODE_I8;
				inst->data.push.ref = 0;
				inst->data.push.label = NULL;
				assm->parse.instruction_count += 1;
				inst = &assm->parse.instructions[assm->parse.instruction_count];
			}
			continue;
		case CHAR_TOKEN:
			inst->tag = PUSH_INST;
			inst->data.push.bytes = t.val;
			inst->data.push.mode = mode;
			inst->data.push.ref = 0;
			inst->data.push.label = NULL;
			assm->parse.instruction_count += 1;
			continue;
		case INTEGER_TOKEN:
			inst->tag = PUSH_INST;
			inst->data.push.bytes = t.val;
			if (t.val < 0xFF){
				inst->data.push.mode = mode;
			}
			else if (t.val < 0xFFFF){
				if (mode < MODE_U16){
					inst->data.push.mode = MODE_U16;
				}
			}
			else if (t.val < 0xFFFFFFFF){
				if (mode < MODE_U32){
					inst->data.push.mode = MODE_U32;
				}
			}
			else{
				inst->data.push.mode = MODE_U64;
			}
			if (t.s == 1){
				inst->data.push.mode += 1;
				inst->data.push.bytes *= -1; // TODO this probably doesnt work
			}
			inst->data.push.ref = 0;
			inst->data.push.label = NULL;
			assm->parse.instruction_count += 1;
			continue;
		}
	}
	assert_local(broke == 1);
	return 0;
}

void show_instructions(assembler* const assm){
	printf("INSTRUCTIONS:\n");
	for (uint64_t i = 0;i<assm->parse.instruction_count;++i){
		instruction inst = assm->parse.instructions[i];
		switch (inst.tag){
		case PUSH_INST:
			printf("PUSH %lx, mode %u ", inst.data.push.bytes, inst.data.push.mode);
			if (inst.data.push.ref == 1){
				printf("(%s) ", inst.data.push.label);
			}
			printf("| ");
			continue;
		case EXEC_INST:
			printf("EXEC %lu |\n", inst.data.exec.pointer);
			continue;
		case OPCODE_INST:
			printf("BUILTIN |\n");
			continue;
		default:
			printf("UNKNOWN INSTRUCTION TYPE\n");
		}
	}
	printf("\n");
}

uint8_t
generate_instructions(assembler* const assm, const char* output){
	assm->code.cap = INITIAL_TARGET_SIZE;
	assm->code.header_cap = INITIAL_TARGET_SIZE;
	assm->code.text = pool_request(assm->mem, sizeof(char)*assm->code.cap);
	assm->code.header = pool_request(assm->mem, sizeof(char)*assm->code.header_cap);
	label_assoc* next = assm->parse.label_order;
	char entrypoint[] = "void PROTOTEXT_ENTRYPOINT(){\n";
	uint64_t entry_len = sizeof(entrypoint)/sizeof(entrypoint[0]);
	strncpy(assm->code.text, entrypoint, entry_len);
	assm->code.size += entry_len;
	strncpy(assm->code.header, "", 8);
	assm->code.header_size = 0;
	char* last_label = NULL;
	uint64_t len = 0;
	MODE mode = MODE_U8;
	for (uint64_t i = 0;i<assm->parse.instruction_count;++i){
		char push[MAX_PUSH_SIZE+1] = "";
		push[0] = '\0';
		while (i == next->instruction){
			if (next->tag == SUBWORD_LABEL){
				strncat(assm->code.text, next->label, next->size);
				strncat(assm->code.text, ":", 2);
				assm->code.size += next->size+1;
			}
			else {
				char reset_stub[] = "}\nvoid\n";
				char func_stub[] = "(){";
				uint64_t reset_len = sizeof(reset_stub)/sizeof(reset_stub[0]);
				uint64_t func_len = sizeof(func_stub)/sizeof(func_stub[0]);
				strncat(assm->code.text, reset_stub, reset_len);
				strncat(assm->code.text, next->label, next->size);
				strncat(assm->code.text, func_stub, func_len);
				assm->code.size += reset_len + next->size + func_len;
				strncat(assm->code.header, "void ", 6);
				strncat(assm->code.header, next->label, next->size);
				strncat(assm->code.header, "();\n", 5);
				assm->code.header_size += next->size + 9;
			}
			next = next->next;
		}
		instruction* inst = &assm->parse.instructions[i];
		if (inst->tag == PUSH_INST){
			mode = inst->data.push.mode;
			last_label = NULL;
			if (inst->data.push.ref == 1){
				last_label = inst->data.push.label;
			}
			switch (inst->data.push.mode){
			case MODE_U8:
			case MODE_I8:
				snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_PUSH(uint8_t, %lu);\n", inst->data.push.bytes);
				break;
			case MODE_U16:
			case MODE_I16:
				snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_PUSH(uint16_t, %lu);\n", inst->data.push.bytes);
				break;
			case MODE_U32:
			case MODE_I32:
				snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_PUSH(uint32_t, %lu);\n", inst->data.push.bytes);
				break;
			case MODE_U64:
			case MODE_I64:
				snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_PUSH(uint64_t, %lu);\n", inst->data.push.bytes);
				break;
			}
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		}
		if (inst->tag == EXEC_INST){
			strncat(assm->code.text, inst->data.exec.label, inst->data.exec.size);
			strncat(assm->code.text, "();\n", 5);
			assm->code.size += inst->data.exec.size + 4;
			continue;
		}
		char argtype[] = "uint64_t";
		if (mode == MODE_I8){ strncpy(argtype, "int8_t", 9); }
		else if (mode == MODE_U8){ strncpy(argtype, "uint8_t", 9); }
		else if (mode == MODE_I16){ strncpy(argtype, "int16_t", 9); }
		else if (mode == MODE_U16){ strncpy(argtype, "uint16_t", 9); }
		else if (mode == MODE_I32){ strncpy(argtype, "int32_t", 9); }
		else if (mode == MODE_U32){ strncpy(argtype, "uint32_t", 9); }
		else if (mode == MODE_I64){ strncpy(argtype, "int64_t", 9); }
		switch (inst->data.opcode){
		case WRITE_TOKEN:
			char write_stub[] = "PROTOTEXT_WRITE();\n";
			uint64_t write_len = sizeof(write_stub)/sizeof(write_stub[0]);
			strncat(assm->code.text, write_stub, write_len);
			assm->code.size += 19;
			continue;
		case ADD_TOKEN:
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_ADD(%s);\n", argtype);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case SUB_TOKEN:
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_SUB(%s);\n", argtype);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case MUL_TOKEN:
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_MUL(%s);\n", argtype);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case DIV_TOKEN:
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_DIV(%s);\n", argtype);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case MOD_TOKEN:
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_MOD(%s);\n", argtype);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case SHR_TOKEN:
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_SHR(%s);\n", argtype);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case SHL_TOKEN:
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_SHL(%s);\n", argtype);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case DUP_TOKEN:
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_DUP(%s);\n", argtype);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case POP_TOKEN:
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_POP_(%s);\n", argtype);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case OVR_TOKEN:
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_OVR(%s);\n", argtype);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case SWP_TOKEN:
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_SWP(%s);\n", argtype);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case NIP_TOKEN:
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_NIP(%s);\n", argtype);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case ROT_TOKEN:
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_ROT(%s);\n", argtype);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case CUT_TOKEN:
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_CUT(%s);\n", argtype);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case RET_TOKEN:
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_RET();\n");
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case AND_TOKEN:
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_AND(%s);\n", argtype);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case OR_TOKEN:
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_OR(%s);\n", argtype);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case XOR_TOKEN:
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_XOR(%s);\n", argtype);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case NOT_TOKEN:
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_NOT(%s);\n", argtype);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case COM_TOKEN:
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_COM(%s);\n", argtype);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case JMP_TOKEN:
			assert_local(last_label != NULL);
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_JMP(%s);\n", last_label);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case JEQ_TOKEN:
			assert_local(last_label != NULL);
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_JEQ(%s, %s);\n", argtype, last_label);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case JTR_TOKEN:
			assert_local(last_label != NULL);
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_JTR(%s, %s);\n", argtype, last_label);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case JFA_TOKEN:
			assert_local(last_label != NULL);
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_JFA(%s, %s);\n", argtype, last_label);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case JNE_TOKEN:
			assert_local(last_label != NULL);
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_JNE(%s, %s);\n", argtype, last_label);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case JLT_TOKEN:
			assert_local(last_label != NULL);
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_JLT(%s, %s);\n", argtype, last_label);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case JLE_TOKEN:
			assert_local(last_label != NULL);
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_JLE(%s, %s);\n", argtype, last_label);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case JGT_TOKEN:
			assert_local(last_label != NULL);
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_JGT(%s, %s);\n", argtype, last_label);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		case JGE_TOKEN:
			assert_local(last_label != NULL);
			snprintf(push, MAX_PUSH_SIZE, "PROTOTEXT_JGE(%s, %s);\n", argtype, last_label);
			len = strnlen(push, MAX_PUSH_SIZE);
			strncat(assm->code.text, push, len);
			assm->code.size += len;
			continue;
		default:
			assert_local(0, "Encountered non opcode tag in opcode instruction\n");
		}
	}
	strncat(assm->code.text, "\n}\n", 4);
	assm->code.size += 4;
	char output_source[MAX_PUSH_SIZE] = "";
	snprintf(output_source, MAX_PUSH_SIZE, "%s.c", output);
	char output_header[MAX_PUSH_SIZE] = "";
	snprintf(output_header, MAX_PUSH_SIZE, "%s.h", output);
	{
		FILE* runtime = fopen("runtime.c", "r");
		assert_local(runtime != NULL, "Unable to access runtime");
		pool outmem = pool_alloc(ARENA_SIZE, POOL_STATIC);
		if (outmem.buffer == NULL){
			fclose(runtime);
			assert_local(0, "Unable to allocate output buffer");
		}
		uint64_t read_bytes = fread(outmem.buffer, sizeof(uint8_t), outmem.left, runtime);
		fclose(runtime);
		if (read_bytes == outmem.left){
			pool_dealloc(&outmem);
			assert_local(0, "Runtime file exceeded buffer maximum");
		}
		FILE* outfile = fopen(output_source, "w");
		assert_local(outfile != NULL, "Unable to create output file for writing");
		char* cast = outmem.buffer;
		cast[read_bytes] = '\0';
		assm->code.text[assm->code.size] = '\0';
		fprintf(outfile, "#include \"%s\"\n", output_header);
		fprintf(outfile, (char*)outmem.buffer);
		fprintf(outfile, assm->code.text);
		fclose(outfile);
		pool_dealloc(&outmem);
	}
	{
		FILE* runtime = fopen("runtime.h", "r");
		assert_local(runtime != NULL, "Unable to access runtime");
		pool outmem = pool_alloc(ARENA_SIZE, POOL_STATIC);
		if (outmem.buffer == NULL){
			fclose(runtime);
			assert_local(0, "Unable to allocate output buffer");
		}
		uint64_t read_bytes = fread(outmem.buffer, sizeof(uint8_t), outmem.left, runtime);
		fclose(runtime);
		if (read_bytes == outmem.left){
			pool_dealloc(&outmem);
			assert_local(0, "Runtime file exceeded buffer maximum");
		}
		FILE* outfile = fopen(output_header, "w");
		assert_local(outfile != NULL, "Unable to create output header for writing");
		char* cast = outmem.buffer;
		cast[read_bytes] = '\0';
		assm->code.header[assm->code.header_size] = '\0';
		fprintf(outfile, (char*)outmem.buffer);
		fprintf(outfile, assm->code.header);
		fprintf(outfile, "#endif\n");
		fclose(outfile);
		pool_dealloc(&outmem);
	}
	return 0;
}

uint8_t
assemble_cstr(assembler* const assm, const char* output){
	lex_cstr(assm);
	assert_prop();
#ifdef DEBUG
	show_tokens(assm);
#endif
	parse_tokens(assm);
	assert_prop();
#ifdef DEBUG
	show_instructions(assm);
#endif
	unresolved_labels(assm);
	assert_prop();
	generate_instructions(assm, output);
	return 0;
}

void
opmap_fill(TOKEN_map* opmap){
	uint64_t opcode_count = STRING_TOKEN-WRITE_TOKEN;
	TOKEN* tokens = pool_request(opmap->mem, sizeof(TOKEN)*opcode_count);
	for (uint8_t i = 0;i<opcode_count;++i){
		tokens[i] = i+WRITE_TOKEN;
	}
	TOKEN_map_insert(opmap, "write", tokens++);
	TOKEN_map_insert(opmap, "add", tokens++);
	TOKEN_map_insert(opmap, "sub", tokens++);
	TOKEN_map_insert(opmap, "mul", tokens++);
	TOKEN_map_insert(opmap, "div", tokens++);
	TOKEN_map_insert(opmap, "mod", tokens++);
	TOKEN_map_insert(opmap, "shr", tokens++);
	TOKEN_map_insert(opmap, "shl", tokens++);
	TOKEN_map_insert(opmap, "dup", tokens++);
	TOKEN_map_insert(opmap, "pop", tokens++);
	TOKEN_map_insert(opmap, "ovr", tokens++);
	TOKEN_map_insert(opmap, "swp", tokens++);
	TOKEN_map_insert(opmap, "nip", tokens++);
	TOKEN_map_insert(opmap, "rot", tokens++);
	TOKEN_map_insert(opmap, "cut", tokens++);
	TOKEN_map_insert(opmap, "ret", tokens++);
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
	uint64_t_map labels = uint64_t_map_init(&mem);
	pointer_thunk_map thunks = pointer_thunk_map_init(&mem);
	parser parse = {
		.instruction_count = 0,
		.instructions = NULL,
		.labels=labels,
		.label_order = NULL,
		.thunks=thunks,
		.super_label=NULL,
		.super_label_size = 0
	};
	target code = {
		.text = NULL,
		.size = 0,
		.header_size = 0,
		.cap = 0,
		.header_cap = 0
	};
	assembler assm = {
		.parse = parse,
		.code = code,
		.input = str,
		.mem = &mem,
		.tokens = NULL,
		.opmap=opmap,
		.err = pool_request(&mem, sizeof(char)*(ERROR_BUFFER+1))
	};
	*assm.err = '\0';
	assemble_cstr(&assm, output);
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
