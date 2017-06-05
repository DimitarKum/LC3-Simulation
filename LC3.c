/*
	A simple simulation of LC3.
	Includes all instructions except JMP, JSRR and RTI.
	Supports DSR/DDR display output but not KBSR/KBDR keyboard output.
	Authors: John Mayer, Dimitar Kumanov
	Version: 5/26/2017
*/
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <inttypes.h>

//Determines whether the simulation will print additional information during execution.
#define PRINT_ON (0)

#define OPCODE(instr)  ((instr) >> 12 & 0x000F)
#define REG1(instr)  ((instr) >> 9 & 0x0007)	//inst[11:9]
#define REG2(instr)  ((instr) >> 6 & 0x0007)	//inst[8:6]
#define REG3(instr)  ((instr) & 0x0007)			//inst[2:0]
#define IMMBIT(instr)  (((instr) & 0x0020) >> 5)
#define IMMVAL(instr)  (((instr) << 27 ) >> 27)
#define PCOFFSET11(instr) (((instr) << 21) >> 21)
#define PCOFFSET9(instr) (((instr) << 23) >> 23)
#define PCOFFSET6(instr) (((instr) << 26) >> 26)
#define BRN(instr)  ((instr) >> 11 & 0x0001)
#define BRZ(instr)  ((instr) >> 10 & 0x0001)
#define BRP(instr)  ((instr) >> 9 & 0x0001)
#define TRPVECT8(instr) (((instr) << 24) >> 24)

#define REG_COUNT (8)
#define ADD_OP (1)
#define AND_OP (5)
#define LD_OP (2)
#define LDI_OP (10)
#define LDR_OP (6)
#define BR_OP (0)
#define ST_OP (3)
#define STI_OP (11)
#define STR_OP (7)
#define LEA_OP (14)
#define JSR_OP (4)
#define RET_OP (12)
#define TRAP_OP (15)

#define DSR ((int16_t) 0xFE04)
#define DISPLAY_READY (0x8000)
#define DISPLAY_SET (0x0000)
#define DDR ((int16_t) 0xFE06)

#define MCR_ADRESS ((int16_t) 0xFFFE)
#define MCR_POWER(mcr) (((mcr) & 0x8000) >> 15)

//Machine Control Register. When mcr[15] == 0b machine turns off.
int16_t mcr;

int16_t  memory[65536];
int16_t regs[REG_COUNT];
int16_t pc, ir;

struct {
    int junk:13;
    unsigned int p:1;
    unsigned int n:1;
    unsigned int z:1;
} psr;   // process status register

struct {
	int16_t status;		//ready bit at status[15]
	int16_t data;		//char to display
} display;	//display io

//Loads the file with the given name to LC3 mem and sets the pc to the starting adress given.
//Returns the starting adress provided.
int16_t loadFile(const char*);

//initializes LC3
void init();

//Prints LC3 state such as: registers, PC, PSR, CC
void printState(void);

//Prints memory from memory[arg1](inclusive) to memory[arg2](exclusive)
void printMemory(int16_t, int16_t);

//Updates the psr's CC using the sign of the value passed
void updatePSR_CC(int16_t);

//These perform the specified instruction:
void addImm();
void addRegs();
void andImm();
void andRegs();
void not();
void br();
void ld();
void ldi();
void ldr();
void st();
void sti();
void str();
void lea();
void jsr();
void ret();
void trap();

int main(int argc, const char* argv[]) {
	const char* fName;
	int16_t load_start_addr;

	if(argc >= 2){  //File specified as console arg
		int i;
		for(i = 1; i < argc; ++i){
			fName = argv[i];
			load_start_addr = loadFile(fName);
			if(PRINT_ON) printf("Loaded file \"%s\" starting at x%04hX\n", fName, load_start_addr);
		}
	}else{  //Default file to run
		printf("Please provide at least 1 .obj file using commmand line arguments\n");
		return 1;
	}

	//Initialize LC3:
	init();
	
	// main loop for fetching and executing instructions
	   
	while (MCR_POWER(mcr)) {   // one instruction executed on each rep.
		if(display.status == DISPLAY_SET){
				printf("%c", (unsigned char) (0x00FF & display.data));
				display.status = DISPLAY_READY;
		}
		ir = memory[pc]; //fetched the instruction
		pc++; 

		int16_t opcode = OPCODE(ir);

		switch(opcode) {
			case ADD_OP:  // add instruction
				if(IMMBIT(ir)){ //ADD immediate
					addImm();
				}else{  //ADD two regs
					addRegs();
				}
				break;
			case AND_OP:
				if(IMMBIT(ir)){ //ADD immediate
					andImm();
				}else{  //ADD two regs
					andRegs();
				}
				break; 
			case LD_OP:
				ld();
				break;
			case LDI_OP:
				ldi();
				break;
			case LDR_OP:
				ldr();
				break;
			case BR_OP:
				br();
				break;
			case ST_OP:
				st();
				break;
			case STI_OP:
				sti();
				break;
			case STR_OP:
				str();
				break;
			case LEA_OP:
				lea();
				break;
			case JSR_OP:
				jsr();
				break;
			case RET_OP:
				ret();
				break;
			case TRAP_OP:
				trap();
				break;			
			default:
				fprintf(stderr, "\nUnrecognized instruction /w opcode %"PRId16"\nPC = x%04hX\nExiting...", opcode, pc);
				getchar();
				return 1;
				break;
		} // switch ends
	}
	if(PRINT_ON){
		printState();
		printf("Execution completed.\n");
		printMemory(load_start_addr, pc);

		printf("Print the next 20 memory locations? (Y/N)\n");
		char c;
		if((c = getchar()) == 'y' || c == 'Y')
			printMemory(pc, pc + 20);
	}
	return 0;
}

int16_t loadFile(const char* fName){
	// how big is the input file?
	struct stat stats;
	stat(fName, &stats);
	int size_in_bytes = stats.st_size;
	   
	FILE *infile = fopen(fName, "r");
	int16_t load_start_addr;

	// read in first two bytes to find out starting address of machine code
	//int words_read = 
	fread(&load_start_addr,sizeof(int16_t), 1, infile);
	// printf("Words read from input = %d,  value read = %hu\n", words_read, load_start_addr);
	char *cptr = (char *)&load_start_addr;
	char temp;

	// switch order of bytes
	temp = *cptr;
	*cptr = *(cptr+1);
	*(cptr+1) = temp;

	// printf("After switching bytes, value read = %hx\n", load_start_addr);   
	pc = load_start_addr;


	// now read in the remaining bytes of the object file
	int instrs_to_load = (size_in_bytes-2)/2;

	// words_read = 
	fread(&memory[load_start_addr], sizeof(int16_t),instrs_to_load, infile);
	// printf("Words read from input = %d\n", words_read);

	// again switch the bytes 
	int i;
	cptr = (char *)&memory[load_start_addr]; 
	for (i = 0; i < instrs_to_load; i++) {
		temp = *cptr;
		*cptr = *(cptr+1);
		*(cptr+1) = temp;
		cptr += 2;  // next pair
	}
	return load_start_addr;
}

void init(){
	display.status = 0x8000;
	display.data = 0x0000;
	psr.z = 1;
	mcr = 0x8000;
}

void updatePSR_CC(int16_t val){
	if(val < 0) {psr.z = 0; psr.n = 1; psr.p = 0;}			//n
	else if(val == 0) {psr.z = 1; psr.n = 0; psr.p = 0;}	//z
	else if(val > 0) {psr.z = 0; psr.n = 0; psr.p = 1;}		//p
}

void printState(){
	int i;
	for(i = 0; i < REG_COUNT; ++i)
		printf("Reg[%d]\t0x%04hX\t#%"PRId16"\n", i, regs[i] & 0xffff, regs[i]);
	printf("PC\t0x%04hX\n", pc & 0xffff);
	printf("PSR\t0x%04hX\n", ((psr.junk << 3) + (psr.n << 2) + (psr.z << 1) + psr.p) & 0xffff);
	printf("IR\t0x%04hX\n", ir & 0xffff);
	printf("CC\t%c\n", psr.n ? 'N' : psr.z ? 'Z' : psr.p ? 'P' : ' ');
}

void printMemory(int16_t from, int16_t to){
	printf("Memory Contents:\n");
	for(; from < to; ++from) printf("%04hX\t0x%04X\n", from & 0xffff, memory[from] & 0xffff);
}

void addImm(){
	int16_t dest_reg = REG1(ir),
		src_reg = REG2(ir),
		imm_val = IMMVAL(ir);
	if(PRINT_ON)	printf("ADD\tR%"PRId16"\tR%"PRId16"\t%d\n", dest_reg, src_reg, imm_val);
	regs[dest_reg] = regs[src_reg] + imm_val;
	updatePSR_CC(regs[dest_reg]);
}

void addRegs(){
	int16_t dest_reg = REG1(ir),
		src_reg1 = REG2(ir),
		src_reg2 = REG3(ir);
	if(PRINT_ON)	printf("ADD\tR%"PRId16"\tR%"PRId16"\tR%"PRId16"\n", dest_reg, src_reg1, src_reg2);
	regs[dest_reg] = regs[src_reg1] + regs[src_reg2];
	updatePSR_CC(regs[dest_reg]);
}

void andImm(){
	int16_t dest_reg = REG1(ir),
		src_reg1 = REG2(ir),
		imm_val = IMMVAL(ir);
	if(PRINT_ON)	printf("AND\tR%"PRId16"\tR%"PRId16"\t%"PRId16"\n", dest_reg, src_reg1, imm_val);
	regs[dest_reg] = regs[src_reg1] & imm_val;
	updatePSR_CC(regs[dest_reg]);
}

void andRegs(){
	if(PRINT_ON)	printf("AND R%"PRId16"\n", 0);
	int16_t dest_reg = REG1(ir),
		src_reg1 = REG2(ir),
		src_reg2 = REG3(ir);
	if(PRINT_ON)	printf("AND\tR%"PRId16"\tR%"PRId16"\tR%"PRId16"\n", dest_reg, src_reg1, src_reg2);
	regs[dest_reg] = regs[src_reg1] & regs[src_reg2];
	updatePSR_CC(regs[dest_reg]);
}

void not(){
	int16_t dest_reg = REG1(ir),
		src_reg = REG2(ir);
	if(PRINT_ON)	printf("NOT\tR%"PRId16"\tR%"PRId16"\n", dest_reg, src_reg);
	regs[dest_reg] = ~regs[src_reg];
	updatePSR_CC(regs[dest_reg]);
}

void br(){
	int16_t n = BRN(ir),
		z = BRZ(ir),
		p = BRP(ir),
		pcoffset = PCOFFSET9(ir);
	if(PRINT_ON)	printf("BR\t%c%c%c\t%"PRId16"\n", n ? 'n' : ' ', z ? 'z' : ' ', p ? 'p' : ' ', pcoffset);
	if((psr.n & n) | (psr.z & z) | (psr.p & p))
		pc += pcoffset;
}

void ld(){
	int16_t dest_reg = REG1(ir),
		pcoffset = PCOFFSET9(ir);
	if(PRINT_ON)	printf("LD\tR%"PRId16"\t%"PRId16"\n", dest_reg, pcoffset);
	int16_t src_adress = pc + pcoffset;

	//Memory mapping should go both ways(loads and stores).
	//The actual memory adresses are completely inaccessible, mem[DSR/DDR/mcr] maps us to our display device.
	if(src_adress == DSR)
		regs[dest_reg] = display.status;
	else if(src_adress == DDR)
		regs[dest_reg] = display.data;
	else if(src_adress == MCR_ADRESS
	)
		regs[dest_reg] = mcr;
	else
		regs[dest_reg] = memory[src_adress];

	// regs[dest_reg] = memory[pc + pcoffset];
	updatePSR_CC(regs[dest_reg]);
}

void ldi(){
	int16_t dest_reg = REG1(ir),
		pcoffset = PCOFFSET9(ir);
	if(PRINT_ON)	printf("LDI\tR%"PRId16"\t%"PRId16"\n", dest_reg, pcoffset);
	int16_t src_adress1 = pc + pcoffset, src_adress2;

	//Memory mapping should go both ways(loads and stores).
	//The actual memory adresses are completely inaccessible, mem[DSR/DDR/mcr] maps us to our display device.
	if(src_adress1 == DSR)
		src_adress2 = display.status;
	else if(src_adress1 == DDR)
		src_adress2 = display.data;
	else if(src_adress1 == MCR_ADRESS
	)
		src_adress2 = mcr;
	else
		src_adress2 = memory[src_adress1];

	if(src_adress2 == DSR)
		regs[dest_reg] = display.status;
	else if(src_adress2 == DDR)
		regs[dest_reg] = display.data;
	else if(src_adress2 == MCR_ADRESS
	)
		regs[dest_reg] = mcr;
	else
		regs[dest_reg] = memory[src_adress2];
	// regs[dest_reg] = memory[memory[pc + pcoffset]];
	updatePSR_CC(regs[dest_reg]);
}

void ldr(){
	int16_t dest_reg = REG1(ir),
		base_reg = REG2(ir),
		pcoffset = PCOFFSET6(ir);
	if(PRINT_ON)	printf("LDR\tR%"PRId16"\tR%"PRId16"\t%"PRId16"\n", dest_reg, base_reg, pcoffset);
	int16_t scr_adress = regs[base_reg] + pcoffset;

	//Memory mapping should go both ways(loads and stores).
	//The actual memory adresses are completely inaccessible, mem[DSR/DDR/mcr] maps us to our display device.
	if(scr_adress == DSR)
		regs[dest_reg] = display.status;
	else if(scr_adress == DDR)
		regs[dest_reg] = display.data;
	else if(scr_adress == MCR_ADRESS
	)
		regs[dest_reg] = mcr;
	else
		regs[dest_reg] = memory[scr_adress];

	// regs[dest_reg] = memory[regs[base_reg] + pcoffset];
	updatePSR_CC(regs[dest_reg]);
}

void st(){
	int16_t src_reg = REG1(ir),
		pcoffset = PCOFFSET9(ir);
	if(PRINT_ON)	printf("ST\tR%"PRId16"\t%"PRId16"\n", src_reg, pcoffset);
	int16_t dest_adress = pc + pcoffset;

	if(dest_adress == DSR)
		display.status = regs[src_reg];
	else if(dest_adress == DDR){
		display.data = regs[src_reg];
		display.status = DISPLAY_SET;
	}else if(dest_adress == MCR_ADRESS
	)
		mcr = regs[src_reg];
	else
		memory[dest_adress] = regs[src_reg];

	// memory[pc + pcoffset] = regs[src_reg];
}

void sti(){
	int16_t src_reg = REG1(ir),
		pcoffset = PCOFFSET9(ir);
	if(PRINT_ON)	printf("STI\tR%"PRId16"\t%"PRId16"\n", src_reg, pcoffset);

	int16_t dest_adress1 = pc + pcoffset, dest_adress2;

	//Although it wouldn't make sense to do it, the client is allowed to
	//treat DSR/DDR as a pointer and they are both memory mapped which means
	//they should not be accessed from memory but from the display device's data/status
	//Same goes for mcr(memory mapped)
	if(dest_adress1 == DSR)
		dest_adress2 = display.status;
	else if(dest_adress1 == DDR)
		dest_adress2 = display.data;
	else if(dest_adress1 == MCR_ADRESS
	)
		dest_adress2 = mcr;
	else
		dest_adress2 = memory[dest_adress1];

	if(dest_adress2 == DSR)
		display.status = regs[src_reg];
	else if(dest_adress2 == DDR){
		display.data = regs[src_reg];
		display.status = DISPLAY_SET;;
	}else if(dest_adress2 == MCR_ADRESS
	)
		mcr = regs[src_reg];
	else
		memory[dest_adress2] = regs[src_reg];

	// memory[memory[pc + pcoffset]] = regs[src_reg];
}

void str(){
	int16_t src_reg = REG1(ir),
		base_reg = REG2(ir),
		pcoffset = PCOFFSET6(ir);
	if(PRINT_ON)	printf("LDR\tR%"PRId16"\tR%"PRId16"\t%"PRId16"\n", src_reg, base_reg, pcoffset);
	int16_t dest_adress = regs[base_reg] + pcoffset;

	if(dest_adress == DSR)
		display.status = regs[src_reg];
	else if(dest_adress == DDR){
		display.data = regs[src_reg];
		display.status = DISPLAY_SET;
	}else if(dest_adress == MCR_ADRESS
	)
		mcr = regs[src_reg];
	else
		memory[dest_adress] = regs[src_reg];
	// memory[regs[base_reg] + pcoffset] = regs[src_reg];
}

void lea(){
	int16_t dest_reg = REG1(ir),
		pcoffset = PCOFFSET9(ir);
	if(PRINT_ON) printf("LEA\tR%"PRId16"\t%"PRId16"\n", dest_reg, pcoffset);
	regs[dest_reg] = pc + pcoffset;
	updatePSR_CC(regs[dest_reg]); //AFFECTS CC!
}

void jsr(){
	int16_t pcoffset = PCOFFSET11(ir);
	if(PRINT_ON) printf("JSR\t%"PRId16"\n", pcoffset);
	regs[7] = pc;	//Save PC into R7:
	pc += pcoffset;
}

void ret(){
	if(PRINT_ON) printf("RET\n");
	pc = regs[7];
}

void trap(){
	int16_t vect = TRPVECT8(ir);
	if(PRINT_ON) printf("TRAP\tx%04hX\n", vect);
	regs[7] = pc;	//Save PC into R7:
	pc = memory[vect];
}
