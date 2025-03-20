#ifndef PROTOTEXT_RUNTIME_H
#define PROTOTEXT_RUNTIME_H

#include <inttypes.h>

#define PROTOTEXT_STACK_SIZE 0x1000000

#define PROTOTEXT_PUSH(type, u) *(type*)(&stack[sp-sizeof(type)]) = u; sp -= sizeof(type)
#define PROTOTEXT_POP(type, var) type var = *(type*)(&stack[sp]); sp += sizeof(type)

#define PROTOTEXT_WRITE() {\
	PROTOTEXT_POP(uint8_t, file);\
	PROTOTEXT_POP(uint64_t, len);\
	fnprintf(file, len, (char*)(&stack[sp-len]));\
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
#define PROTOTEXT_AND() { PROTOTEXT_ALU_OP(type, &) }
#define PROTOTEXT_OR() { PROTOTEXT_ALU_OP(type, |) }
#define PROTOTEXT_XOR() { PROTOTEXT_ALU_OP(type, ^) }

#define PROTOTEXT_DUP(type) {\
	PROTOTEXT_POP(type, a);\
	PROTOTEXT_PUSH(type, a);\
	PROTOTEXT_PUSH(type, a);\
}

#define PROTOTEXT_POP_(type) {\
	PROTOTEXT_POP(type, a);\
}

#define PROTOTEXT_OVR() {\
	PROTOTEXT_POP(type, a);\
	PROTOTEXT_POP(type, b);\
	PROTOTEXT_PUSH(type, b);\
	PROTOTEXT_PUSH(type, a);\
	PROTOTEXT_PUSH(type, b);\
}

#define PROTOTEXT_SWP() {\
	PROTOTEXT_POP(type, a);\
	PROTOTEXT_POP(type, b);\
	PROTOTEXT_PUSH(type, a);\
	PROTOTEXT_PUSH(type, b);\
}

#define PROTOTEXT_NIP() {\
	PROTOTEXT_POP(type, a);\
	PROTOTEXT_POP(type, b);\
	PROTOTEXT_PUSH(type, a);\
}

#define PROTOTEXT_ROT() {\
	PROTOTEXT_POP(type, a);\
	PROTOTEXT_POP(type, b);\
	PROTOTEXT_POP(type, c);\
	PROTOTEXT_PUSH(type, b);\
	PROTOTEXT_PUSH(type, a);\
	PROTOTEXT_PUSH(type, c);\
}

#define PROTOTEXT_CUT() {\
	PROTOTEXT_POP(type, a);\
	PROTOTEXT_POP(type, b);\
	PROTOTEXT_POP(type, c);\
	PROTOTEXT_PUSH(type, b);\
	PROTOTEXT_PUSH(type, a);\
}

#define PROTOTEXT_RET() {\
	return;\
}

#define PROTOTEXT_NOT() {\
	PROTOTEXT_POP(type, a);\
	a = !a;\
	PROTOTEXT_PUSH(type, a);\
}

#define PROTOTEXT_COM() {\
	PROTOTEXT_POP(type, a);\
	a = ~a;\
	PROTOTEXT_PUSH(type, a);\
}

#define PROTOTEXT_JMP(loc) {\
	goto loc;\
}

#define PROTOTEXT_JUMP_CONDITIONAL(condition)\
	PROTOTEXT_POP(type, a);\
	PROTOTEXT_PUSH(type, a);\
	if (condition){\
		goto loc;\
	}

#define PROTOTEXT_JUMP_CONDITIONAL2(condition)\
	PROTOTEXT_POP(type, a);\
	PROTOTEXT_POP(type, b);\
	PROTOTEXT_PUSH(type, b);\
	PROTOTEXT_PUSH(type, a);\
	if (condition){\
		goto loc;\
	}

#define PROTOTEXT_JTR(loc) { PROTOTEXT_JUMP_CONDITIONAL(a) }
#define PROTOTEXT_JFA(loc) { PROTOTEXT_JUMP_CONDITIONAL(!a) }

#define PROTOTEXT_JEQ(loc) { PROTOTEXT_JUMP_CONDITIONAL2(a==b) }
#define PROTOTEXT_JNE(loc) { PROTOTEXT_JUMP_CONDITIONAL2(a!=b) }
#define PROTOTEXT_JLT(loc) { PROTOTEXT_JUMP_CONDITIONAL2(a<b) }
#define PROTOTEXT_JLE(loc) { PROTOTEXT_JUMP_CONDITIONAL2(a<=b) }
#define PROTOTEXT_JGT(loc) { PROTOTEXT_JUMP_CONDITIONAL2(a>b) }
#define PROTOTEXT_JGE(loc) { PROTOTEXT_JUMP_CONDITIONAL2(a>=b) }

void PROTOTEXT_ENTRYPOINT();

//ensure compilation include #endif
