#include <stdio.h>
#include <stdlib.h>
#define N 15 
#define DEBUG 0
int load01();
int load01_end();
int load_and_max();
int load_and_max_end();
int factorial();
int factorial_end();
int add();
int add_end();

typedef enum Type{
    DATA = -1,
    BASIC,
    BRANCH,
    LABEL
}codeType;

typedef struct CodeInformation{
    size_t i_count; //the number of instructions
    size_t dp_count;// the number of data processing instructions
    size_t b_count;// the numebr of brach instrucitons
    size_t dt_count;// the number of data transfer instructions
    size_t c_count;// the number of coprocessor instrucitons
    size_t registers_count[16];// the read or write of registers
    unsigned int write_regs;// the write flag
    codeType * instructions;
    unsigned * start;
    unsigned * end;
}codeInfo;

struct {int x:24;} sign_helper;

unsigned get_bits(unsigned value, unsigned start, unsigned length){
    unsigned bits;
    unsigned mask;
    bits = value >> start;
    mask = (1 << length) - 1;
    bits = bits & mask;
    return bits;
}

void print_bits(size_t const size, void const * const ptr)
{
    unsigned char * b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;
    for( i = size-1; i>=0 ; i--){
	for(j=7;j>=0;j--){
	    byte = b[i] & (1<<j);
	    byte >>= j;
	    printf("%u", byte);
	} 
	printf(" ");
    } 
    puts("");
}

int outputAllResults(codeInfo * cip){
    printf("Total number of instruction is %u\n", cip -> i_count);
    printf("Total number of data processing instruction is %u\n", cip ->
	    dp_count);
    printf("Total number of branch instruction is %u\n", cip -> b_count);
    printf("Total number of data transfer instruction is %u\n", cip -> dt_count);
    printf("Total number of coprocessor instruction is %u\n", cip -> c_count);
    int i;
    for(i = 0; i < 13 ;i++){
	if(cip->registers_count[i] != 0)
	    printf("The r%u is be used %u times\n",
		    i,cip->registers_count[i]); 
    }
	if(cip->registers_count[13] != 0)
    printf("The sp is be used %u times\n", cip->registers_count[13]);
	if(cip->registers_count[14] != 0)
    printf("The lr is be used %u times\n", cip->registers_count[14]);
	if(cip->registers_count[15] != 0)
    printf("The pc is be used %u times\n", cip->registers_count[15]);
    printf("These registers are be written:");
    for(i = 0; i < 13;i++){
	if((cip->write_regs & (1<<i)) != 0)
	    printf("r%u,",i);
    }
    if((cip->write_regs & (1<<13)) != 0) printf("sp,");
    if((cip->write_regs & (1<<14)) != 0) printf("lr,");
    if((cip->write_regs & (1<<15)) != 0) printf("pc.\n");
    printf("Callee saved registers are : ");
    for(i = 4; i < 12;i++){
	if((cip->write_regs & (1<<i)) != 0)
	    printf("r%u,",i);
    }
    printf(".\n");
    int instructionSize = cip->end - cip->start;
    *(cip->instructions + instructionSize + 1) = 2;
    i=0;
    int start = 0;
    int length = 0;
    for(i = 0;i<=instructionSize+1;i++){
	codeType flag = *(cip->instructions + i); 
	if(flag == LABEL){
	    if(length != 0){
		printf("Basic blocks : (%p, %d)\n",cip->start + start, length);	
		start = i;
		length = 1;
	    }else
		length += 1;
	}else if(flag == BRANCH){
	    printf("Basic blocks : (%p, %d)\n",cip->start + start, length + 1);	
	    start = i+1;
	    length = 0;
	}else if(flag == DATA){
	    if(start == i)
		start += 1;
	}else
	    length += 1;
    }
    return 1;
}

int analysisSoftwareInterrupt(unsigned* ip, codeInfo* cip){
    if(get_bits(*ip, 24, 4) == 0b1111){
	if(DEBUG)
	    printf("This is a software interrupt instruction!\n");
	return 1;
    }
    return 0;
}

int analysisUndefinedInstruction(unsigned* ip, codeInfo* cip){
    if(get_bits(*ip, 25, 3)== 0b1111 && get_bits(*ip, 4, 1) == 0b1){
	if(DEBUG)
	    printf("This is a undefined instruciton!\n");
	return 1;
    }
    return 0;
}

/* this funciton can analysis the b/bl instructions.
 * firstly match the 4 - 27 bits.
 * and then get the destination register.
 * */
int analysisBranchAndExchange(unsigned* ip, codeInfo* cip){
    if(get_bits(*ip, 4, 24) == 0b000100101111111111110001){
	if(DEBUG)
	    printf("This is a branch and exchange instruciton!\n");
	cip->b_count++;
	int Rn = get_bits(*ip, 0, 4);
	cip->registers_count[Rn]++;
	int instructionNumber = ip - (cip->start);
	*(cip->instructions + instructionNumber) = BRANCH;
	return 1;
    }
    return 0;
}
int analysisSWP(unsigned * ip, codeInfo* cip){
    if(get_bits(*ip, 23, 5) == 0b00010 &&
	    get_bits(*ip, 20, 2) == 0b00 &&
	    get_bits(*ip,  4, 8) == 0b00001001){
	if(DEBUG)
	    printf("This is a swp instruciton!\n");
	cip->dt_count++;
	unsigned Rm = get_bits(*ip, 0, 4);
	unsigned Rn = get_bits(*ip, 12, 4);
	unsigned Rd = get_bits(*ip, 16, 4);
	cip->registers_count[Rn]++;
	cip->registers_count[Rm]++;
	cip->registers_count[Rd]++;
	cip->write_regs |= (1<<Rd);
	return 1;
    }
    return 0;
}

int analysisMultiply(unsigned * ip, codeInfo* cip){
    if(get_bits(*ip, 22, 6) == 0b000000 &&
	    get_bits(*ip, 4, 4) == 0b1001){
	if(DEBUG)
	    printf("This is a Multiply Instruction!\n");
	cip->dp_count++;
	unsigned Rd = get_bits(*ip, 16, 4);
	unsigned Rn = get_bits(*ip, 12, 4);
	unsigned Rs = get_bits(*ip, 8, 4);
	unsigned Rm = get_bits(*ip, 0, 4);
	cip->registers_count[Rd]++;
	if(get_bits(*ip, 21, 1) == 1)
	    cip->registers_count[Rn]++;
	cip->registers_count[Rs]++;
	cip->registers_count[Rm]++;
	cip->write_regs |= (1<<Rd);
	return 1;
    }
    return 0;
}

int analysisHalfwordDataTransferRegister(unsigned* ip, codeInfo* cip){
    if(get_bits(*ip, 25, 3) == 0b000 &&
	    get_bits(*ip, 22, 1) == 0b0 &&
	    get_bits(*ip, 7, 5) == 0b00001 &&
	    get_bits(*ip, 4, 1) == 0b1){
	if(DEBUG)
	    printf("This is a Halfword Data Transfer based on Registers !\n");
	cip->dt_count++;
	unsigned Rn = get_bits(*ip, 16, 4);
	unsigned Rd = get_bits(*ip, 12, 4);
	unsigned Rm = get_bits(*ip, 0, 4);
	cip->registers_count[Rn]++;
	cip->registers_count[Rd]++;
	cip->registers_count[Rm]++;
	if(get_bits(*ip, 21, 1) == 0b1 || get_bits(*ip, 24, 1) == 0b0){
	    cip->registers_count[Rn]++;
	    cip->write_regs |= (1<<Rn);
	}
	if(get_bits(*ip, 20, 1) == 0b1){
	    cip->write_regs |= (1<<Rd);
	}
	return 1;
    }
    return 0;
}

int analysisMultiplyLong(unsigned* ip, codeInfo* cip){
    if(get_bits(*ip, 23, 5) == 0b00001 &&
	    get_bits(*ip, 4, 4) == 0b1001){
	if(DEBUG)
	    printf("This is a multiplyLong instruction!\n");
	cip->dp_count++;
	unsigned RdHi = get_bits(*ip, 16, 4);
	unsigned RdLo = get_bits(*ip, 12, 4);
	unsigned Rs = get_bits(*ip, 8, 4);
	unsigned Rm = get_bits(*ip, 0, 4);
	cip->registers_count[RdHi]++;
	cip->registers_count[RdLo]++;
	cip->registers_count[Rs]++;
	cip->registers_count[Rm]++;
	if(get_bits(*ip, 21, 1) == 1){
	    cip->registers_count[RdHi]++;
	    cip->registers_count[RdLo]++;
	}
	cip->write_regs |= (1<<RdHi);
	cip->write_regs |= (1<<RdLo);
	return 1;
    }
    return 0;
}

int analysisHalfwordTransferImmediate(unsigned* ip, codeInfo* cip){
    if(get_bits(*ip, 25, 3) == 0b000 &&
	    get_bits(*ip, 22, 1) == 0b1 &&
	    get_bits(*ip, 7, 1) == 0b1 &&
	    get_bits(*ip, 4 ,1) == 0b1){
	if(DEBUG)
	    printf("This is the Halfword data transfer instrucion!\n");
	cip->dt_count++;
	unsigned Rn = get_bits(*ip, 16, 4);
	unsigned Rd = get_bits(*ip, 12, 4);
	cip->registers_count[Rn]++;
	cip->registers_count[Rd]++;
	if(get_bits(*ip, 21, 1) == 0b1 || get_bits(*ip, 24, 1) == 0b0){
	    cip->registers_count[Rn]++;
	    cip->write_regs |= (1<<Rn);
	}
	if(get_bits(*ip, 20, 1) == 0b1)
	    cip->write_regs |= (1<<Rd);
	return 1;
    }
    return 0;
}

int analysisCoprocessorDataOperation(unsigned* ip,codeInfo* cip){
    if(get_bits(*ip, 24, 4) == 0b1110 &&
	    get_bits(*ip, 4, 1) == 0b0){
	if(DEBUG)
	    printf("This is an Coprocessor Data Operation instruciton!\n");
	cip->c_count++;
	return 1;
    }
    return 0;
}

int analysisCoprocessorRegisterTransfer(unsigned* ip, codeInfo* cip){
    if(get_bits(*ip, 24, 4) == 0b1110 &&
	    get_bits(*ip, 4, 1) == 0b1){
	if(DEBUG)
	    printf("This is an coprocessor Register Transfer instruction!\n");
	cip->c_count++;
	int Rd = get_bits(*ip, 12, 4);
	if(get_bits(*ip, 20, 1) == 0b1 && Rd!=0b1111)  
	{
	    cip->write_regs |= (1<<Rd);
	    cip->registers_count[Rd]++;
	}else if(get_bits(*ip, 12, 1) == 0b0){
	    cip->registers_count[Rd]++;
	}

	return 1;
    }
    return 0;
}

int analysisBlockDataTransfer(unsigned* ip, codeInfo* cip){
    if(get_bits(*ip, 25, 3) == 0b100){
	if(DEBUG)
	    printf("This is a block data transfer instrction!\n");
	cip->dt_count++;
	unsigned Rn = get_bits(*ip, 16, 4);
	cip->registers_count[Rn]++;
	if(get_bits(*ip, 21, 1) == 0b1){
	    cip->write_regs |= (1<<Rn);
	    cip->registers_count[Rn]++;}
	int loadFlag = get_bits(*ip, 20, 1);
	int i;
	unsigned data = get_bits(*ip, 0, 15);
	for(i = 0;i<15;i++){
	    if(data & (1<<i)){
		cip->registers_count[i]++;
		if(loadFlag) cip->write_regs |= (1<<i); 
	    }	
	}
	return 1;
    }
    return 0;
}

int analysisBranch(unsigned* ip, codeInfo* cip){
    if(get_bits(*ip, 25, 3) == 0b101){
	if(DEBUG)
	    printf("This is a brach instruction!\n");
	cip->b_count++;
	int instructionNumber = ip - (cip->start);
	*(cip->instructions + instructionNumber) = BRANCH;
	unsigned offset = get_bits(*ip, 0, 24);
	sign_helper.x = offset;
	int ip_offset = sign_helper.x;
	*(cip->instructions + instructionNumber + ip_offset + 2) = LABEL;
	if(get_bits(*ip, 24, 1) == 1){
	    cip->write_regs |= (1<<14);
	    cip->registers_count[14]++;
	} 
	return 1;
    }
    return 0;
}

int analysisCoprocessorDataTransfer(unsigned* ip,codeInfo* cip){
    if(get_bits(*ip, 25, 3) == 0b110){
	if(DEBUG)
	    printf("This is a coprocessor data transfer instruction!\n");
	cip->c_count++;
	unsigned Rn = get_bits(*ip, 16, 4);
	cip->registers_count[Rn]++;
	if(get_bits(*ip, 21, 1) == 0b1){
	    cip->registers_count[Rn]++;
	    cip->write_regs |= (1<<Rn);
	}
	return 1;
    }	
    return 0;
}

int analysisDataProcessing(unsigned* ip, codeInfo* cip){
    if(get_bits(*ip, 26, 2) == 0b00){
	if(DEBUG)
	    printf("This is a data processing instruction!\n");
	cip->dp_count++;
	unsigned Rn = get_bits(*ip, 16, 4);
	unsigned Rd = get_bits(*ip, 12, 4);
	if((get_bits(*ip, 21, 4) & 0b1101) != 0b1101)
	    cip->registers_count[Rn]++;
	unsigned OpCode = get_bits(*ip, 23, 2);
	if(OpCode != 0b10){ 
	    cip->registers_count[Rd]++;
	    cip->write_regs |= (1<<Rd);
	}
	if(get_bits(*ip, 25, 1) == 0b0){
	    unsigned Rm = get_bits(*ip, 0, 4);
	    cip->registers_count[Rm]++;
	    if(get_bits(*ip, 4, 1) == 0b1){
		unsigned Rs = get_bits(*ip, 8, 4);
		cip->registers_count[Rs]++;
	    }
	} 
	return 1;
    }
    return 0;
}

int analysisSingleDataTransfer(unsigned* ip, codeInfo* cip){
    if(get_bits(*ip ,26 ,2) == 0b01){
	if(DEBUG)
	    printf("This is a single data transfer instruction!\n");
	cip->dt_count++;
	unsigned Rn = get_bits(*ip, 16, 4);
	unsigned Rd = get_bits(*ip, 12, 4);
	cip->registers_count[Rn]++;
	cip->registers_count[Rd]++;
	if(get_bits(*ip, 21, 1) == 0b1 || get_bits(*ip, 24, 1) == 0b0){
	    cip->write_regs |= (1<<Rn);
	    cip->registers_count[Rn]++;
	}
	if(get_bits(*ip, 20, 1) == 0b1) cip->write_regs |= (1<<Rd);
	if(get_bits(*ip, 25, 1) == 0b1){
	    unsigned Rm = get_bits(*ip, 0, 4);
	    cip->registers_count[Rm]++;
	}

	return 1;
    }
    return 0;
}

void preAnalysisSingleDataTransfer(unsigned* ip, codeInfo* cip){
    if(DEBUG)
	print_bits(4, ip);
    if(get_bits(*ip ,25 ,3) == 0b010 && get_bits(*ip, 16, 4) == 0b1111){
	if(DEBUG)
	    printf("This is a single data transfer instruction!\n");
	int flag = get_bits(*ip, 24, 1) == 1 ? 1 : -1;
	unsigned offset = get_bits(*ip, 0, 12);
	offset /= 4;
	int destination = (ip - (cip->start)) + offset * flag + 2;
	if(destination < cip->end - cip->start)
	    *(cip->instructions + destination) = DATA;
    }

    if(get_bits(*ip ,25 ,3) == 0b000 &&
	    get_bits(*ip, 22, 1) == 0b1 &&
	    get_bits(*ip, 7,  1) == 0b1 &&
	    get_bits(*ip, 4,  1) == 0b1 &&
	    get_bits(*ip, 5,  2) != 0b00 &&
	    get_bits(*ip, 16, 4) == 0b1111){
	if(DEBUG)
	    printf("This is a halfword data transfer instruction!\n");
	int flag = get_bits(*ip, 24, 1) == 1 ? 1 : -1;
	unsigned offset = get_bits(*ip, 8, 4) << 4;
	offset |= get_bits(*ip, 0, 4);
	offset /= 4;
	int destination = (ip - (cip->start)) + offset * flag + 2;
	if(destination >= 0 && destination < cip->end - cip->start)
	    *(cip->instructions + destination) = DATA;

    }
}

int analysisCode(unsigned* ip, codeInfo* cip){
    int(* codeAnalysis[N])(unsigned* , codeInfo* );
    codeAnalysis[0] = analysisSoftwareInterrupt; 
    codeAnalysis[1] = analysisUndefinedInstruction; 
    codeAnalysis[2] = analysisBranchAndExchange; 
    codeAnalysis[3] = analysisSWP; 
    codeAnalysis[4] = analysisMultiply;
    codeAnalysis[5] = analysisHalfwordDataTransferRegister;
    codeAnalysis[6] = analysisMultiplyLong;
    codeAnalysis[7] = analysisHalfwordTransferImmediate;
    codeAnalysis[8] = analysisCoprocessorDataOperation;
    codeAnalysis[9] = analysisCoprocessorRegisterTransfer;
    codeAnalysis[10] = analysisBlockDataTransfer;
    codeAnalysis[11] = analysisBranch;
    codeAnalysis[12] = analysisCoprocessorDataTransfer;
    codeAnalysis[13] = analysisDataProcessing;
    codeAnalysis[14] = analysisSingleDataTransfer;
    int i;
    for(i = 0 ; i < N;i++){
	if(cip-> instructions[ip - (cip->start)]!=DATA){
	    if((* codeAnalysis[i])(ip,cip)){
		if(DEBUG)
		    print_bits(4, ip);
		cip->i_count++;
		cip->write_regs |= (1<<15);
		cip->registers_count[15] += 2;
		if(DEBUG)
		    outputAllResults(cip);
		return 1;
	    }	
	}else{
	    if(DEBUG){
		printf("This is a data line!\n");
		print_bits(4, ip);
	    }
	    return 1;
	}
    }
    if(DEBUG)
	printf("This instruciton cannot be decoded\n");
    return 0;
}

void analysisFile(char* name, unsigned* start, unsigned* end){
    printf("\n******   %s   ******\n\n", name);
    unsigned * ip;
    codeInfo ci = { 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ci.start = start;
    ci.end = end;
    //        printf("%p,%p\n",start, end);
    codeInfo* cip = &ci;
    cip -> instructions = (int*) malloc((end - start + 1) * sizeof(int)); 
    int i;
    for(i= 0;i<end-start+1;i++)
	*(cip->instructions + i) = 0;
    for(ip = start; ip <= end;ip++){
	preAnalysisSingleDataTransfer(ip,cip);			
	//	print_bits(4, ip);
    }
    if(DEBUG)
	for(i = 0; i <= end-start;i++)
	    printf("%d\n", *(cip->instructions + i));
    for(ip = start; ip <= end;ip++){
	analysisCode(ip,cip);			
    }
    if(DEBUG)
	for(i = 0; i <= end-start;i++)
	    printf("%d\n", *(cip->instructions + i));
    free(cip->instructions);
    outputAllResults(cip);
}

int main(){
    analysisFile("load_and_max",(unsigned *)load_and_max,(unsigned *)load_and_max_end);
    analysisFile("load01",(unsigned *)load01,(unsigned*) load01_end); 
    analysisFile("factorial",(unsigned*)factorial, (unsigned*)factorial_end);
    analysisFile("add",(unsigned*)add, (unsigned*)add_end);
    return 1;
}

