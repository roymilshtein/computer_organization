#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>
#include <math.h>

typedef unsigned int uint;

int hex_to_int(char c)
{
	switch (c)
	{
	case '0': return 0;
	case '1': return 1;
	case '2': return 2;
	case '3': return 3;
	case '4': return 4;
	case '5': return 5;
	case '6': return 6;
	case '7': return 7;
	case '8': return 8;
	case '9': return 9;

	case 'a': return 10;
	case 'b': return 11;
	case 'c': return 12;
	case 'd': return 13;
	case 'e': return 14;
	case 'f': return 15;

	case 'A': return 10;
	case 'B': return 11;
	case 'C': return 12;
	case 'D': return 13;
	case 'E': return 14;
	case 'F': return 15;

	default: return -1;  //shouldn't happen
	}
}

static int regs[16] = { 0 }; //array of integers. value in index 'i' is the value of register 'i'
static unsigned short pc = 0; //program counter
static unsigned short ic = 0; //instruction counter, will be used for counting clock cycles
static int memory[4096] = { 0 }; // initializing memory array in size of 4096
static int halt_flag = 0;

//define operations 
void op_add(uint rd, uint rs, uint rt) { // rd, rs, rt are between 0-15
	regs[rd] = regs[rs] + regs[rt];
}

void op_sub(uint rd, uint rs, uint rt) {
	regs[rd] = regs[rs] - regs[rt];
}

void op_mul(uint rd, uint rs, uint rt) {
	regs[rd] = regs[rs] * regs[rt];
}

void op_and(uint rd, uint rs, uint rt) {
	regs[rd] = regs[rs] & regs[rt];
}

void op_or(uint rd, uint rs, uint rt) {
	regs[rd] = regs[rs] | regs[rt];
}

void op_xor(uint rd, uint rs, uint rt) {
	regs[rd] = regs[rs] ^ regs[rt];
}

void op_sll(uint rd, uint rs, uint rt) {
	regs[rd] = regs[rs] << regs[rt];
}

void op_sra(uint rd, uint rs, uint rt) { // arithmetic shift with sign extension to the right
	regs[rd] = *(int32_t*)&regs[rs] >> regs[rt];
}

void op_srl(uint rd, uint rs, uint rt) { //adding zeroes at the beginning - logical shift right
	regs[rd] = regs[rs] >> regs[rt];
}

void op_beq(uint rd, uint rs, uint rt) {
	if (regs[rs] == regs[rt])
	{
		pc = (regs[rd] & 0xFFF) - 3;
	}
}

void op_bne(uint rd, uint rs, uint rt) {
	if (regs[rs] != regs[rt])
	{
		pc = (regs[rd] & 0xFFF) - 3;
	}
}

void op_blt(uint rd, uint rs, uint rt) {
	if (regs[rs] < regs[rt])
	{
		pc = (regs[rd] & 0xFFF) - 3;
	}
}

void op_bgt(uint rd, uint rs, uint rt) {
	if (regs[rs] > regs[rt])
	{
		pc = (regs[rd] & 0xFFF) - 3;
	}
}

void op_ble(uint rd, uint rs, uint rt) {
	if (regs[rs] <= regs[rt])
	{
		pc = (regs[rd] & 0xFFF) - 3;
	}
}

void op_bge(uint rd, uint rs, uint rt) {
	if (regs[rs] >= regs[rt])
	{
		pc = (regs[rd] & 0xFFF) - 3;
	}
}

void op_jal(uint rd, uint rs) {
	regs[rd] = pc + 4;
	pc = (regs[rs] & 0xFFF) - 3;
}

void op_lw(uint rd, uint rs, uint rt) {
	regs[rd] = memory[(regs[rs] + regs[rt]) % 4096]; //mod in case of address outside memory scope.
}

void op_sw(uint rd, uint rs, uint rt) {
	memory[(regs[rs] + regs[rt]) % 4096] = regs[rd]; //mod in case of address outside memory scope.
}

void op_halt(uint rd, uint rs, uint rt) {
	//printf("HALT!");
	halt_flag = 1;
}

int hex_string_to_int(const char* str) {
	int num = 0;
	for (int i = 0; str[i] != '\0' && str[i] != '\n'; i++)
	{
		num = num * 16 + hex_to_int(str[i]);
	}
	if (((num & (1 << 19)) >> 19) == 1) // negative
	{
		num = num - pow(2, 20);
	}
	return num;
}

void print_inst_to_trace(FILE* trace, unsigned short pc, int inst) {
	fprintf(trace, "%.3X %.5X %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X %.8X\n"
		, pc, inst, regs[0], regs[1], regs[2], regs[3], regs[4], regs[5]
		, regs[6], regs[7], regs[8], regs[9], regs[10],
		regs[11], regs[12], regs[13], regs[14], regs[15]);
}
void execute_op(uint op_code, uint rd, uint rs, uint rt) {
	switch (op_code)
	{
	case 0:
		op_add(rd, rs, rt);
		break;
	case 1:
		op_sub(rd, rs, rt);
		break;
	case 2:
		op_mul(rd, rs, rt);
		break;
	case 3:
		op_and(rd, rs, rt);
		break;
	case 4:
		op_or(rd, rs, rt);
		break;
	case 5:
		op_xor(rd, rs, rt);
		break;
	case 6:
		op_sll(rd, rs, rt);
		break;
	case 7:
		op_sra(rd, rs, rt);
		break;
	case 8:
		op_srl(rd, rs, rt);
		break;
	case 9:
		op_beq(rd, rs, rt);
		break;
	case 10:
		op_bne(rd, rs, rt);
		break;
	case 11:
		op_blt(rd, rs, rt);
		break;
	case 12:
		op_bgt(rd, rs, rt);
		break;
	case 13:
		op_ble(rd, rs, rt);
		break;
	case 14:
		op_bge(rd, rs, rt);
		break;
	case 15:
		op_jal(rd, rs); // removed rt. not needed here!
		break;
	case 16:
		op_lw(rd, rs, rt);
		ic++;
		break;
	case 17:
		op_sw(rd, rs, rt);
		ic++;
		break;
	case 18:
		op_halt(rd, rs, rt);
		break;
	default:
		printf("ERROR! op not found!\n"); // not meant to happen. checked already in assembler.
		exit(-1);
		break;
	}

}
void simulate(FILE* memin, FILE* trace) {
	char line[7]; //max line len (5 + \n + \0)
	char* ptr= NULL;
	int counter = 0;
	while ((fgets(line, sizeof(line), memin)) != NULL) { //fill memory[4096] with converted lines
		line[5] = '\0';
		int conv_line = hex_string_to_int(line);

		memory[counter] = conv_line; // just a line converted to int 
		counter++;
	}
	uint rd, rs, rt, op_code;
	int imm_bool = 0; // will be set to 1 when we have imm (used to jump pc by 2)
	do
	{
		int curr_inst = memory[pc];
		imm_bool = 0;
		op_code = (curr_inst & 0xFF000) >> 12;
		rd = (curr_inst & 0x00F00) >> 8;
		rs = (curr_inst & 0x000F0) >> 4;
		rt = (curr_inst & 0x0000F);
		if (rd == 1 || rs == 1 || rt == 1) // we have immediate!
		{
			//read next memory element as imm
			int imm_val = memory[pc + 1];
			regs[1] = imm_val;
			imm_bool = 1;
		}
		else {
			regs[1] = 0; //no imm in instruction
		}
		print_inst_to_trace(trace, pc, curr_inst);
		//execute op:
		execute_op(op_code, rd, rs, rt);
		//increase ic depending on instruction:		
		ic += 1 + imm_bool; // added extra 1 for sw/lw when executing them	
		pc += 1 + imm_bool;
	} while (halt_flag != 1);
}

int main(int argc, char* argv[]) {

	FILE* memin;
	memin = fopen(argv[1], "r"); // argv[1] = memin.txt (our input)
	if (!memin)
	{
		fprintf(stderr, "main(): fopen(memin.txt) failed - ");
		perror(NULL);
		exit(-1);
	}

	FILE* trace;
	trace = fopen(argv[4], "w"); // argv[4] = trace.txt
	if (!trace)
	{
		fprintf(stderr, "main(): fopen(trace.txt) failed - ");
		perror(NULL);
		exit(-1);
	}

	simulate(memin, trace);

	FILE* memout;
	memout = fopen(argv[2], "w"); // argv[2] = memout.txt (our main output)
	if (!memout)
	{
		fprintf(stderr, "main(): fopen(memout.txt) failed - ");
		perror(NULL);
		exit(-1);
	}
	for (int i = 0; i < 4096 - 1; i++) { //initialize empty memout.txt
		fprintf(memout, "%.5X\n", memory[i] & 0xfffff); //thats 20 bits long maximum
	}fprintf(memout, "%.5X", memory[4095] & 0xfffff); // memout is pointing to last row! no \n

	FILE* regout;// open regout.txt and print the current final regs values to it.
	regout = fopen(argv[3], "w"); // argv[3] = regout.txt
	if (!regout)
	{
		fprintf(stderr, "main(): fopen(regout.txt) failed - ");
		perror(NULL);
		exit(-1);
	}
	for (int i = 2; i < 16 - 1; i++) { //print regs to regout.txt
		fprintf(regout, "%.8X\n", regs[i]);
	}fprintf(regout, "%.8X", regs[15]); // regout is pointing to last row! no "\n"
	FILE* cycles;// open cycles.txt and print current pc into it
	cycles = fopen(argv[5], "w"); // argv[5] = cycles.txt
	if (!cycles)
	{
		fprintf(stderr, "main(): fopen(cycles.txt) failed - ");
		perror(NULL);
		exit(-1);
	}
	fprintf(cycles, "%d", ic); // cycles has only one row and one print!

	fclose(cycles);
	fclose(trace);
	fclose(regout);
	fclose(memin);
	fclose(memout);

	return 0;
}