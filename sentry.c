#include <cheri/cheric.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "include/common.h"
#include "include/instructions.h"
#include "include/regs.h"

struct data {
	int a;
	int b;
	char c[256];
};

void* setup_sentry(void*, void (*f)(uint32_t*, void*));
void oop_sentry(struct data*, int);
void simple_sentry(int);

void oop_sentry(struct data* this, int arg) {
	printf("OOP: %p %d \n", this, arg);
}

void simple_sentry(int arg) {
	printf("Simple: %d \n", arg);
}

void gen_oop(uint32_t* code, void* function) {
	intptr_t* code_data = (intptr_t*)code;
	code_data[255] = (intptr_t)function;
	code_data[254] = (intptr_t)malloc(sizeof(struct data));
	
	uint32_t idx = 0;
	code[idx++] = auipcc(cs2, 1);
	code[idx++] = clc_128(cs3, cs2, ((-16) + (2 << 20)));
	code[idx++] = addi(a1, a0, 0);
	code[idx++] = addi(a2, a1, 0);
	code[idx++] = addi(a3, a2, 0);
	code[idx++] = addi(a4, a3, 0);
	code[idx++] = addi(a5, a4, 0);
	code[idx++] = addi(a6, a5, 0);
	code[idx++] = addi(a7, a6, 0);
	code[idx++] = clc_128(ca0, cs2, ((-32) + (2 << 20)));
	code[idx++] = cjalr(cnull, cs3);


}

void gen_simple(uint32_t* code, void* function) {
	intptr_t* code_data = (intptr_t*)code;
	code_data[255] = (intptr_t)function;
	
	uint32_t idx = 0;
	code[idx++] = auipcc(cs2, 1);
	code[idx++] = clc_128(cs3, cs2, ((-16) + (2 << 20)));
	code[idx++] = cjalr(cnull, cs3);
}

void gen_override(uint32_t* code, void* function) {
	intptr_t* code_data = (intptr_t*)code;
	code_data[255] = (intptr_t)function;
	
	uint32_t idx = 0;
	code[idx++] = auipcc(cs2, 1);
	code[idx++] = clc_128(cs3, cs2, ((-16) + (2 << 20)));
	code[idx++] = addi(a0, cnull, 42);
	code[idx++] = cjalr(cnull, cs3);
}

void* setup_sentry(void* function, void (*generate_code)(uint32_t*, void*)) {
	uint32_t *code =
		mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0);
	
	generate_code(code, function);
	
	return cheri_sealentry(cheri_setflags(code, 1));
}

void* setup_normal(void* function, void (*generate_code)(uint32_t*, void*)) {
	uint32_t *code =
		mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0);
	
	generate_code(code, function);
	
	return cheri_setflags(code, 1);
}

int main() {
	puts("0: Simple sentry");
	puts("1: OOP sentry");
	puts("2: Override sentry");
	puts("3: Normal function");
	puts("4: Fail");
	printf("Mode: ");
	uint32_t mode = 0;
	if(scanf("%u", &mode) == 0) {
		error("Invalid input");
	}

	void (*fpointer)(int);
	switch(mode) {
		case 0:
			fpointer = setup_sentry(simple_sentry, gen_simple);
			break;
		case 1:
			fpointer = setup_sentry(oop_sentry, gen_oop);
			break;
		case 2:
			fpointer = setup_sentry(simple_sentry, gen_override);
			break;
		case 3:
			fpointer = setup_normal(simple_sentry, gen_simple);
			break;
		case 4: // This case should fail
			fpointer = setup_sentry(simple_sentry, gen_simple);
			fpointer = cheri_incoffset(fpointer, 16);
			break;
	}

	fpointer(99);
	printf("END\n");
}
