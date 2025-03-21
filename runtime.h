#ifndef PROTOTEXT_RUNTIME_H
#define PROTOTEXT_RUNTIME_H

#include <inttypes.h>
#include <stdio.h>

#define PROTOTEXT_STACK_SIZE 0x1000000

#define PROTOTEXT_PUSH(type, u) *(type*)(&stack[sp-sizeof(type)]) = u; sp -= sizeof(type)
#define PROTOTEXT_POP(type, var) type var = *(type*)(&stack[sp]); sp += sizeof(type)

#define PROTOTEXT_WRITE() {\
	PROTOTEXT_POP(uint8_t, file);\
	PROTOTEXT_POP(uint64_t, len);\
	FILE* f = stdout;\
	switch (file) {\
	case 0:\
		f = stdin;\
	case 2:\
		f = stderr;\
	}\
	char* string = (char*)(&stack[sp]);\
	for (uint64_t i = 0;i<len;++i){\
		fputc(string[i], f);\
	}\
}

#define PROTOTEXT_ALU_OP(type, op)\
	PROTOTEXT_POP(type, a);\
	PROTOTEXT_POP(type, b);\
	type c = a op b;\
	PROTOTEXT_PUSH(type, c);

#define PROTOTEXT_ADD(type) { PROTOTEXT_ALU_OP(type, +) }
#define PROTOTEXT_SUB(type) { PROTOTEXT_ALU_OP(type, -) }
#define PROTOTEXT_MUL(type) { PROTOTEXT_ALU_OP(type, *) }
#define PROTOTEXT_DIV(type) { PROTOTEXT_ALU_OP(type, /) }
#define PROTOTEXT_MOD(type) { PROTOTEXT_ALU_OP(type, %) }
#define PROTOTEXT_SHR(type) { PROTOTEXT_ALU_OP(type, >>) }
#define PROTOTEXT_SHL(type) { PROTOTEXT_ALU_OP(type, <<) }
#define PROTOTEXT_AND(type) { PROTOTEXT_ALU_OP(type, &) }
#define PROTOTEXT_OR(type) { PROTOTEXT_ALU_OP(type, |) }
#define PROTOTEXT_XOR(type) { PROTOTEXT_ALU_OP(type, ^) }

#define PROTOTEXT_DUP(type) {\
	PROTOTEXT_POP(type, a);\
	PROTOTEXT_PUSH(type, a);\
	PROTOTEXT_PUSH(type, a);\
}

#define PROTOTEXT_POP_(type) {\
	PROTOTEXT_POP(type, a);\
}

#define PROTOTEXT_OVR(type) {\
	PROTOTEXT_POP(type, a);\
	PROTOTEXT_POP(type, b);\
	PROTOTEXT_PUSH(type, b);\
	PROTOTEXT_PUSH(type, a);\
	PROTOTEXT_PUSH(type, b);\
}

#define PROTOTEXT_SWP(type) {\
	PROTOTEXT_POP(type, a);\
	PROTOTEXT_POP(type, b);\
	PROTOTEXT_PUSH(type, a);\
	PROTOTEXT_PUSH(type, b);\
}

#define PROTOTEXT_NIP(type) {\
	PROTOTEXT_POP(type, a);\
	PROTOTEXT_POP(type, b);\
	PROTOTEXT_PUSH(type, a);\
}

#define PROTOTEXT_ROT(type) {\
	PROTOTEXT_POP(type, a);\
	PROTOTEXT_POP(type, b);\
	PROTOTEXT_POP(type, c);\
	PROTOTEXT_PUSH(type, b);\
	PROTOTEXT_PUSH(type, a);\
	PROTOTEXT_PUSH(type, c);\
}

#define PROTOTEXT_CUT(type) {\
	PROTOTEXT_POP(type, a);\
	PROTOTEXT_POP(type, b);\
	PROTOTEXT_POP(type, c);\
	PROTOTEXT_PUSH(type, b);\
	PROTOTEXT_PUSH(type, a);\
}

#define PROTOTEXT_RET() {\
	return;\
}

#define PROTOTEXT_NOT(type) {\
	PROTOTEXT_POP(type, a);\
	a = !a;\
	PROTOTEXT_PUSH(type, a);\
}

#define PROTOTEXT_COM(type) {\
	PROTOTEXT_POP(type, a);\
	a = ~a;\
	PROTOTEXT_PUSH(type, a);\
}

#define PROTOTEXT_JMP(loc) {\
	PROTOTEXT_POP(uint64_t, c);\
	goto loc;\
}

#define PROTOTEXT_JUMP_CONDITIONAL(type, condition)\
	PROTOTEXT_POP(uint64_t, c);\
	PROTOTEXT_POP(type, a);\
	PROTOTEXT_PUSH(type, a);\
	if (condition){\
		goto loc;\
	}

#define PROTOTEXT_JUMP_CONDITIONAL2(type, condition)\
	PROTOTEXT_POP(uint64_t, c);\
	PROTOTEXT_POP(type, a);\
	PROTOTEXT_POP(type, b);\
	PROTOTEXT_PUSH(type, b);\
	PROTOTEXT_PUSH(type, a);\
	if (condition){\
		goto loc;\
	}

#define PROTOTEXT_JTR(type, loc) { PROTOTEXT_JUMP_CONDITIONAL(type, a) }
#define PROTOTEXT_JFA(type, loc) { PROTOTEXT_JUMP_CONDITIONAL(type, !a) }

#define PROTOTEXT_JEQ(type, loc) { PROTOTEXT_JUMP_CONDITIONAL2(type, a==b) }
#define PROTOTEXT_JNE(type, loc) { PROTOTEXT_JUMP_CONDITIONAL2(type, a!=b) }
#define PROTOTEXT_JLT(type, loc) { PROTOTEXT_JUMP_CONDITIONAL2(type, a<b) }
#define PROTOTEXT_JLE(type, loc) { PROTOTEXT_JUMP_CONDITIONAL2(type, a<=b) }
#define PROTOTEXT_JGT(type, loc) { PROTOTEXT_JUMP_CONDITIONAL2(type, a>b) }
#define PROTOTEXT_JGE(type, loc) { PROTOTEXT_JUMP_CONDITIONAL2(type, a>=b) }

void PROTOTEXT_ENTRYPOINT();

