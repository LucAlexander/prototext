#include <stdio.h>
#include "runtime.h"

static uint8_t stack[PROTOTEXT_STACK_SIZE];
static uint64_t sp = PROTOTEXT_STACK_SIZE;

int main(int argc, char** argv){
	PROTOTEXT_ENTRYPOINT();
	return 0;
}


