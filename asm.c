#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>

#define MAX_LABEL_LEN	50 // defined
#define MAX_LINE_LEN	300 // defined
#define CS_LEN			4096 // max num of lines in memory

#define OP_CODES_LEN 19
static char* op_names[] = { // all opcode names - 19 total
	"add",
	"sub",
	"mul",
	"and",
	"or",
	"xor",
	"sll",
	"sra",
	"srl",
	"beq",
	"bne",
	"blt",
	"bgt",
	"ble",
	"bge",
	"jal",
	"lw",
	"sw",
	"halt"
};
#define REG_NAMES_LEN 16
static char* reg_names[] = { // all the registers names - 16 total
	"$zero",
	"$imm",
	"$v0",
	"$a0",
	"$a1",
	"$a2",
	"$a3",
	"$t0",
	"$t1",
	"$t2",
	"$s0",
	"$s1",
	"$s2",
	"$gp",
	"$sp",
	"$ra"
};

static unsigned short pc = 0; // including imm lines
static unsigned short ic = 0; // just number of instructions
static int memory[CS_LEN] = { 0 }; //memory array in size of 4096

typedef struct
{
	char name[MAX_LABEL_LEN + 1]; // includes room for '\0'
	unsigned short addr;
} SymbolEntry;

typedef struct
{
	/* instruction line */
	char opcode[6]; // .word is the longest opcode name (+1 for \0)
	char rd[6]; // $zero is the longest reg name (+1 for \0)
	char rs[10]; // to have room for ".word" imm_val
	char rt[6];
	char imm_val[50]; // why 50? max length of label!
	bool imm; // imm exists in inst
	int imm_int; // translated imm val to int or converted label to absolute address

} broken_inst; // instruction broken to tokens

typedef struct //table of instructions
{
	broken_inst inst[CS_LEN]; //max num of instructions
}inst_table;

static inst_table str_inst_table; // table to store the found instructions
static SymbolEntry sym_table[CS_LEN];
static int label_counter = 0;
unsigned sym_table_len = 0;

//function declerations
int convert_string_to_int(char* str);
void remove_comment(char* line);
void get_label(char* line);
void split_line_to_tokens(char* ptr, broken_inst* inst);
void first_stage(FILE* program, inst_table* str_inst_table, unsigned short* pc, unsigned short* ic);
void remove_extra_spaces(char* str);
int convert_string_to_int(char* str);
int labelcomp(char* label, char* label_in_inst);
int store_inst_in_mem(inst_table* str_inst_table, int i, int imm_counter);
void second_stage(inst_table* str_inst_table, unsigned short* pc, unsigned short* ic);


void remove_comment(char* line) { //gets a pointer to the head of the line and removes comment (anything after '#')
	char* ptr = line;
	while (*ptr != '#' && *ptr != '\n' && *ptr != '\0') {
		ptr++;
	}
	if (*ptr == '\n') //reached end of line!
	{
		return;
	}
	else if (*ptr == '\0') 
	{
		return; 
	}
	else if (*ptr == '#')
	{
		*ptr = '\0'; // stop the line at '#'
		return;
	}
}
void get_label(char* line) {
	int i = 0;
	if (!isalpha(*line)) //first char is not alphabet -> illegal label!
	{
		printf("trying to store illegal label!\n");
		exit(-1);
	}

	while (*line != ':') { // fill one char at a time...
		sym_table[sym_table_len].name[i] = *line;
		line++;
		i++;
	}
	sym_table[sym_table_len].name[i] = '\0';
	label_counter++;
}

void split_line_to_tokens(char* ptr, broken_inst* inst) { //gets a line and creates a list of tokens to fill fields of inst
	char* ptr2 = ptr;
	while (*ptr2 != '\n') // replacing commas with spaces
	{
		if (*ptr2 == ',')
		{
			*ptr2 = ' ';
		}
		ptr2++;
	}

	char* token = strtok(ptr, " ");
	int i = 0;
	inst->imm = false; //not imm unless found $imm later (default is R-type)
	int word_flag = 0; //flag for .word 'instruction'
	while (token) {
		switch (i)
		{
		case 0:
			if (strlen(token) > 6)
			{
				printf("ERROR! opcode too long: \"%s\"", token);
				exit(-1);
			}
			strcpy(inst->opcode, token);
			if (!strcmp(token, ".word"))
			{
				word_flag = 1;
			}
			break;
		case 1:
			if (strlen(token) > 6)
			{
				printf("ERROR! reg name (rd) too long: \"%s\"", token);
				exit(-1);
			}
			strcpy(inst->rd, token);
			if (!strcmp(token, "$imm"))
			{
				inst->imm = true;
			}
			break;
		case 2:
			//if (strlen(token) > 6)
			//{
			//	printf("ERROR! reg name (rs) too long: \"%s\"", token);
			//	exit(-1);
			//}
			if (word_flag == 0)
			{
				strcpy(inst->rs, token);
				if (!strcmp(token, "$imm"))
				{
					inst->imm = true;
				}
			}
			else {
				strcpy(inst->imm_val, token); // storing value into imm_val
			}
			break;
		case 3:
			if (strlen(token) > 6)
			{
				printf("ERROR! reg name (rt) too long: \"%s\"", token);
				exit(-1);
			}
			if (word_flag == 0)
			{
				strcpy(inst->rt, token);
				if (!strcmp(token, "$imm"))
				{
					inst->imm = true;
				}
			}
			break;
		case 4:
			if (word_flag == 0)
			{
				//store token in imm_val
				strcpy(inst->imm_val, token);
				//if label, stop here.
				// if int, enter int to imm_int:
				if (!isalpha(token[0]))
				{
					int int_token = convert_string_to_int(token);
					inst->imm_int = int_token;
				}
				else {
					inst->imm_int = -1;
				}
			}

			break;
		default: // 6th token and up: if alpha/num - error. else - ignore.
			if (isalpha(token[0]) || (token[0] >= '0' && token[0] <= '9'))
			{
				printf("ERROR! FOUND AN EXTRA TOKEN IN INSTRUCTION: \"%s\"", token);
				exit(-1);
			}
			else 
				break;
		}
		i++;
		token = strtok(NULL, " ");
	}
}

void first_stage(FILE* program, inst_table* str_inst_table, unsigned short* pc, unsigned short* ic) {
	//go over the program line by line, extract labels and build inst_table.
	char line[MAX_LINE_LEN];
	size_t len = 0;
	char* ptr=NULL;

	while ((fgets(line, sizeof(line), program)) != NULL) { //running on all the lines in program
		remove_comment(line);
		ptr = line;
		while (*ptr != '\0' && *ptr != ':') {
			ptr++;
		}
		if (*ptr == ':') // found a label!
		{
			ptr = line;
			get_label(ptr); // stores label name
			sym_table[sym_table_len].addr = *pc;  
			sym_table_len += 1;
			// store label addr and name in table ^
			while (*ptr != ':') {	// move line to after the ":"
				ptr++;
			}ptr++;
		}
		else // no label found
			ptr = line; // move to start of line
			// so now, ptr points to right after the label (if exists), or beginning of line.
			// move ptr to next non-whitespace char, so it points to beginning of instruction (if exists)
		while (isspace(*ptr))
			++ptr;

		if (*ptr == '\0' || *ptr == '\n')
		{
			/* line has no instruction or psuedo-instuction */
			continue;
		}

		// split line to tokens:
		broken_inst inst;

		split_line_to_tokens(ptr, &inst);
		if (!strcmp(inst.opcode, ".word")) // storing ".word" inst in memory
		{
			int conv_rd = convert_string_to_int(inst.rd);
			int conv_imm = convert_string_to_int(inst.imm_val);
			memory[conv_rd % 4096] = conv_imm & 0xfffff; // mod4096 to avoid accessing outside memory
			// opcode, address,             value
			// opcode,    rd,     rs,  rt,   imm
			continue;
		}

		str_inst_table->inst[*ic] = inst;

		if (inst.imm == true) // increase pc depending on I/R type inst
			(*pc) += 2; // raise pc by 2 (cause 2 rows)
		else // R type inst
			(*pc)++; // raise pc by 1

		(*ic)++; // and ic by 1

	}

}

void remove_extra_spaces(char* str) //removes extra spaces at the end of the string
{
	int i = 0;
	while (*str != ' ' && *str != '\n' && *str != '\0' && *str != '\t')
	{
		str++;
	}
	*str = '\0';
}

int convert_string_to_int(char* str)
{
	int result;
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
		// The string is in hex form
		sscanf(str, "%x", &result);
	}
	else {
		// The string is in decimal form
		sscanf(str, "%d", &result);
	}
	return result;
}


int labelcomp(char* label, char* label_in_inst) //comparing labels char by char. used to find label in label array
{
	int i = 0;
	while (label_in_inst[i] != '\0') {
		if (label_in_inst[i] != label[i])
		{
			return 0;
		}
		i++;
	}
	return 1;
}

int store_inst_in_mem(inst_table* str_inst_table, int i, int imm_counter) { // gets an inst and saves to mem
	int inst_to_store = 0; // 0000 0000 0000 0000 0000
	int imm_to_store = 0;
	// translate every part of inst to HEX
	int j = 0;
	for (j = 0; j < OP_CODES_LEN; ++j)
	{
		if (0 == strcmp(str_inst_table->inst[i].opcode, op_names[j]))
		{
			inst_to_store += j << 12;// |0000 0000| 0000 0000 0000
			break;
		}
	}
	if ((j == OP_CODES_LEN) && (0 != strcmp(str_inst_table->inst[i].opcode, "halt"))) //couldnt find legal op_code name
	{
		printf("\"%s\" is not a legal opcode name!", str_inst_table->inst[i].opcode);
		exit(-1);
	}

	j = 0;
	for (j = 0; j < REG_NAMES_LEN; ++j)
	{
		if (0 == strcmp(str_inst_table->inst[i].rd, reg_names[j])) // if "$0" won't change current 0
		{
			inst_to_store += j << 8;// 0000 0000 |0000| 0000 0000
			break;
		}
	}
	if ((j == REG_NAMES_LEN) && (0 != strcmp(str_inst_table->inst[i].rd, "$0"))) //couldnt find legal reg name
	{
		printf("\"%s\"is not a legal register name! (rd)", str_inst_table->inst[i].rd);
		exit(-1);
	}

	// add,sub,mul,and,or,xor,sll,sra,srl,jal,lw
	// 00, 01, 02, 03, 04, 05,06, 07, 08, 15, 16
	//$zero,$imm
	if (((inst_to_store >> 12) <= 8) || (inst_to_store >> 12) == 15 || (inst_to_store >> 12) == 16)// we have problematic opcode
	{
		if (((inst_to_store >> 8) & 0xf) <= 1) // we have rd = $zero/$imm
		{
			printf("trying to perform illegal operation! writing to %s with op: %s", str_inst_table->inst[i].rd, str_inst_table->inst[i].opcode);
			exit(-1);
		}

	}

	j = 0;
	for (j = 0; j < REG_NAMES_LEN; ++j)
	{
		if (0 == strcmp(str_inst_table->inst[i].rs, reg_names[j]))
		{
			inst_to_store += j << 4;// 0000 0000 0000 |0000| 0000
			break;
		}
	}
	if ((j == REG_NAMES_LEN) && (0 != strcmp(str_inst_table->inst[i].rs, "$0"))) //couldnt find legal reg name
	{
		printf("\"%s\"is not a legal register name! (rs)", str_inst_table->inst[i].rs);
		exit(-1);
	}

	j = 0;
	for (j = 0; j < REG_NAMES_LEN; ++j)
	{
		if (0 == strcmp(str_inst_table->inst[i].rt, reg_names[j]))
		{
			inst_to_store += j;// 0000 0000 0000 0000 |0000|
			break;
		}
	}

	if ((j == REG_NAMES_LEN) && (0 != strcmp(str_inst_table->inst[i].rt, "$0"))) //couldnt find legal reg name
	{
		printf("\"%s\"is not a legal register name! (rt)", str_inst_table->inst[i].rt);
		exit(-1);
	}

	memory[i + imm_counter] = inst_to_store;
	if (str_inst_table->inst[i].imm == true)
	{
		imm_to_store = str_inst_table->inst[i].imm_int & 0xfffff; //cutting negative numbers to 20 bits.
		memory[i + 1 + imm_counter] = imm_to_store;  // wont get to the end of array cause max lines is 300.
		imm_counter += 1;
	}
	return imm_counter;
}

void second_stage(inst_table* str_inst_table, unsigned short* pc, unsigned short* ic) {
	//convert labels (if exists) with absolute address:
	int i = 0;
	int imm_counter = 0;
	while (i < *ic)
	{
		if (isalpha(str_inst_table->inst[i].imm_val[0]))//found a label in imm_val!
		{
			int k = 0;
			while (k < label_counter)
			{
				remove_extra_spaces(str_inst_table->inst[i].imm_val);

				if (labelcomp(str_inst_table->inst[i].imm_val, sym_table[k].name))
				{
					str_inst_table->inst[i].imm_int = sym_table[k].addr + 1; // replace imm_int(-1) with absolute label address
				}
				k++;
			}

		}
		imm_counter = store_inst_in_mem(str_inst_table, i, imm_counter);

		i++;
	}

}

int main(int argc, char* argv[]) { //expects to get two args- fibo.asm and memin.txt

	FILE* program;
	program = fopen(argv[1], "r"); //argv[1] = fibo.asm (our program)
	if (!program)
	{
		fprintf(stderr, "main(): fopen(PROGRAM) failed - "); //printing an error to the screen- had a problem to open the program file!
		perror(NULL);
		exit(-1);
	}


	first_stage(program, &str_inst_table, &pc, &ic);

	second_stage(&str_inst_table, &pc, &ic);

	FILE* memin = fopen(argv[2], "w"); //argv[2] = memin.txt (our output)
	if (memin == NULL)
	{
		printf("could not open a \"memin.txt\" file to write to!");
		exit(-1);
	}
	else {
		for (int i = 0; i < 4096 - 1; i++)
		{
			fprintf(memin, "%.5X\n", memory[i]);
		}fprintf(memin, "%.5X", memory[4095]); // no need for \n in last row

		fclose(memin);
	}

	fclose(program);

	return 0;
}