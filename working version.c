#include<stdio.h>
#include<stdbool.h>
/* ARM code to analyze */

int max_iter_pointer(int *array, int count);
int max_iter_pointer_end();

int load_and_max(int a, int b);
int load_and_max_end();

int factorial(int a);
int factorial_end();

/* Print binary representation of a C data type.
   size is the number of bytes of the type. 
void print_bits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr; // 8 bit char. In contrast with 7 bit char?
    unsigned char byte;
    int i, j;

    for (i=size-1;i>=0;i--)
    {  
        for (j=7;j>=0;j--)
        {  
            byte = b[i] & (1<<j);
            byte >>= j;
            printf("%u", byte);
        }
    }
    puts("");
}
*/

unsigned get_bits(unsigned value, unsigned start, unsigned length)
{
    unsigned bits;
    unsigned mask;

    bits = value >> start;
    mask = (1 << length) - 1;

    bits = bits & mask;

    return bits;
}

unsigned* findData(unsigned *value, unsigned data_addr[]){
    unsigned offset = get_bits(*value, 0, 12);
    int ip_offset;
    struct {int x:24;} sign_helper; //declare sign_helpeer to be a struct object, int x:24, sign extended to 32 bits
    sign_helper.x = offset;
    ip_offset = sign_helper.x;// extended to 32 bits
   //printf("ip_offset = %d\n", ip_offset);
    unsigned *dest;
    dest = value + 2 + ip_offset/4; //pc=pc+2, due to the pipe thing...
    int i;
    for(i = 0; i<100; i++){
        if(data_addr[i] == 0){
            data_addr[i] = (unsigned) dest;
            break;
        }
    }
    return dest;
}

bool isData(unsigned *value, unsigned data_addr[]){
    int i;
    for(i = 0; i < 100; i++){
        if ((unsigned)value == data_addr[i]){
            return true;
        }
        if(data_addr[i] == 0){
            break;
        }
    }
    return false;
}

unsigned* findLabel(unsigned *value, unsigned label_addr[]){
    unsigned offset = get_bits(*value, 0, 24);
    int ip_offset;
    struct {int x:24;} sign_helper; //declare sign_helper to be a struct object, int x:24, sign extended to 32 bits
    sign_helper.x = offset;
    ip_offset = sign_helper.x;// extended to 32 bits
   // printf("ip_offset = %d\n", ip_offset);
    unsigned *dest;
    dest = value + 2 + ip_offset; //pc=pc+2, due to the  pipe thing...
  //  printf("ip_offset = %d, dest = %p\n", ip_offset, dest);
    int i;
    for(i = 0; i < 100; i++){
        if (label_addr[i] == 0){
            label_addr[i] = (unsigned) dest;
            break;
        }
    }
    return dest;
}

bool isLabel(unsigned *value, unsigned label_addr[]){
    int i;
    for(i = 0; i < 100; i++){
        if ((unsigned)value == label_addr[i]){
            return true;
        }
        if(label_addr[i] == 0){
            break;
        }
    }
    return false;
}

/*Data Processing/PSR Transfer instructions*/
bool isDP(unsigned *value){
    unsigned bits27_25 = get_bits(*value, 25, 3);
    unsigned bit4 = get_bits(*value, 4, 1);
    unsigned bit7 = get_bits(*value, 7, 1);
    if(bits27_25 == 0b001
        || (bits27_25 == 0b000 && (bit4 & bit7) == 0))
        return true;
    else
        return false;
}

showDP(unsigned *value, unsigned r_counts[], unsigned written[]){
    unsigned I = get_bits(*value, 25,1);
    unsigned opcode = get_bits(*value, 21, 4);
    unsigned Rd = get_bits(*value, 12, 4);
    unsigned Rn_1st = get_bits(*value, 16, 4);
 //   printf("This is DP with opcode = %d\n", opcode);
    if(I == 0){
        unsigned Rm_noshift = get_bits(*value, 0, 4);
        unsigned shift = get_bits(*value, 4, 8);
        r_counts[Rm_noshift]++;
        //MOV, MVN, Rd op op2
        if(opcode == 0b1101 || opcode == 0b1111){
            r_counts[Rd]++;
            if(written[Rd] == 0) written[Rd] = 1;
   //         printf("R%d = op R%d with %d shift\n", Rd, Rm_noshift, shift);
        }
        //set condition code on op1 op op2
        else if(opcode >= 0b1000 && opcode <= 0b1011){
            r_counts[Rn_1st]++;
     //       printf("Set condition based on R%d op R%d with %d shift\n", Rn_1st, Rm_noshift, shift);
        }
        //Rd = op1 op op2
        else{
            r_counts[Rd]++; r_counts[Rn_1st]++;
            if(written[Rd] == 0) written[Rd] = 1;
     //       printf("R%d = R%d op R%d with %d shift\n", Rd, Rn_1st, Rm_noshift, shift);

        }
    }else{
        unsigned Imm_norotate = get_bits(*value, 0,8);
        unsigned rotate = get_bits(*value, 8, 4);
        //MOV, MVN, Rd op op2
        if(opcode == 0b1101 || opcode == 0b1111){
            r_counts[Rd]++;
            if(written[Rd] == 0) written[Rd] = 1;
 //           printf("R%d = op %d with %d rotate\n", Rd, Imm_norotate, rotate);
        }
        //set condition code on op1 op op2
        else if(opcode >= 0b1000 && opcode <= 0b1011){
            r_counts[Rn_1st]++;
     //       printf("Set condition based on R%d op %d with %d rotate\n", Rn_1st, Imm_norotate, rotate);
        }
        //Rd = op1 op op2
        else{
            r_counts[Rd]++; r_counts[Rn_1st]++;
            if(written[Rd] == 0) written[Rd] = 1;
    //        printf("R%d = R%d op %d with %d rotate\n", Rd, Rn_1st, Imm_norotate, rotate);
        }
    }

}

/*Multiply and Multiply Long instructions*/
bool isMULT(unsigned *value){
    unsigned bits27_22 = get_bits(*value, 22, 6);
    unsigned bits27_23 = get_bits(*value, 23, 5);
    unsigned bit4 = get_bits(*value, 4, 1);
    unsigned bit7 = get_bits(*value, 7, 1);
    if((bits27_22 == 0b0 || bits27_23 == 0b1) && (bit4 & bit7 == 1))
        return true;
    else
        return false;
}
void showMULT(unsigned *value, unsigned r_counts[], unsigned written[]){
    unsigned A = get_bits(*value, 21, 1);
    unsigned Rd = get_bits(*value, 16, 4);
    unsigned Rn = get_bits(*value, 12, 4);
    unsigned Rs = get_bits(*value, 8, 4);
    unsigned Rm = get_bits(*value, 0, 4);
    if(A == 0){
 //       printf("This is a multiply instruction: R%d = R%d*R%d.\n", Rd, Rm, Rs);
    }else{
        r_counts[Rn]++;
  //      printf("This is a multiply instruction: R%d = R%d*R%d+R%d.\n",Rd, Rm, Rs, Rn);
    }
    r_counts[Rm]++; r_counts[Rs]++; r_counts[Rd]++;
    if(written[Rd] == 0) written[Rd] = 1;
}

/*Branch and Exchange*/
bool isSDS(unsigned *value){
    unsigned bits27_23 = get_bits(*value, 23, 5);
    unsigned bits21_20 = get_bits(*value, 20, 2);
    unsigned bits11_4 = get_bits(*value, 4, 8);
    if(bits27_23 == 0b10 && bits21_20 == 0b0 &&bits11_4 == 0b1001)
        return true;
    else
        return false;
}

void showSDS(unsigned *value, unsigned r_counts[], unsigned written[]){
    unsigned Rn = get_bits(*value, 16, 4);
    unsigned Rd = get_bits(*value, 12, 4);
    unsigned Rm = get_bits(*value, 0, 4);
    r_counts[Rn]++; r_counts[Rd]++; r_counts[Rm]++;
    written[Rd] = 1;
 //   printf("This is a DT-SDS instruction. Rd = %d, Rm = %d, Rn = %d\n", Rd, Rm, Rn);
}


bool isBX(unsigned *value){
    unsigned bits27_4 = get_bits(*value, 4, 24);
    unsigned bits3_0 = get_bits(*value, 0,4); //R15, undefined instruction
    if (bits27_4 == 0b100101111111111110001 && bits3_0 != 0b1111)
        return true;
    else
        return false;
}

void showBX(unsigned *value, unsigned r_counts[],unsigned written[]){
    unsigned Rn = get_bits(*value, 0,4);
    r_counts[Rn] += 1;
//    printf("BX to address in Rn = %d\n", Rn);
    //Rn is read and written into pc
}

/*Halfword Data Transfer-register offset*/
bool isHDT_r(unsigned *value){
    if(get_bits(*value, 25, 3)== 0 && get_bits(*value, 22, 1) == 0
        && get_bits(*value, 7, 5) == 1 && get_bits(*value, 4, 1) == 1)
        return true;
    else
        return false;
}
void showHDT_r(unsigned *value, unsigned r_counts[], unsigned written[]){
    unsigned L = get_bits(*value, 20, 1);
    unsigned W = get_bits(*value, 21, 1);
    unsigned P = get_bits(*value, 24, 1);
    unsigned Rn = get_bits(*value, 16, 4);
    unsigned Rd = get_bits(*value, 12, 4);
    unsigned Rm = get_bits(*value, 0, 4);

    if(W ==1 || (W == 0 && P == 0)) { //Write back to Rn
        written[Rn] = 1;
        r_counts[Rn] += 1;
    }
    r_counts[Rn]+=1; // Rn always gets read
   if(L == 0){ //STR
  //         printf("This is a HDT_r (STR) instruction from R%d to R%d.\n", Rd, Rn);
    }else{ //LDR
        written[Rd] = 1;
  //      printf("This is a HDT_r (LDR) instruction from R%d to R%d.\n", Rn, Rd);
    }
    r_counts[Rd]+=1; // read for str, load for ldr
    r_counts[Rm]+=1;  //offset specified in Rm
}


/*Halfword Data Transfer-immediate offset*/
bool isHDT_i(unsigned *value){
    if(get_bits(*value, 25, 3)== 0 && get_bits(*value, 22, 1) == 1
        && get_bits(*value, 7, 1) == 1 && get_bits(*value, 4, 1) == 1)
        return true;
    else
        return false;
}

unsigned* findData_HDTi(unsigned *value, unsigned data_addr[]){
    unsigned U = get_bits(*value, 23, 1); // 0 substract from, 1 add to base
    unsigned low_nibble = get_bits(*value, 0, 4);
    unsigned high_nibble = get_bits(*value, 8, 4);
//    printf("low = %d, high = %d\n",low_nibble, high_nibble);
    unsigned offset = (high_nibble << 4) + low_nibble;
 //   printf("offset = %d\n", offset);
    unsigned *dest;
    if(U == 0)
        dest = value + 2 - offset/4;
    else
        dest = value + 2 + offset/4;
  //  printf("dest = %p\n",dest);
    int i;
    for(i = 0; i<100; i++){
        if(data_addr[i] == 0){
            data_addr[i] = (unsigned) dest;
            break;
        }
    }
    return dest;
}

void showHDT_i(unsigned *value, unsigned r_counts[], unsigned written[], unsigned data_addr[]){
    unsigned L = get_bits(*value, 20, 1);
    unsigned W = get_bits(*value, 21, 1);
    unsigned P = get_bits(*value, 24, 1);
    unsigned Rn = get_bits(*value, 16, 4);
    unsigned Rd = get_bits(*value, 12, 4);

    if(W ==1 || (W == 0 && P == 0)) { //Write back to Rn
        written[Rn] = 1;
        r_counts[Rn] += 1;
    }
    r_counts[Rn]+=1; // Rn always gets read

   if(L == 0){ //STR
        if(Rn == 0b1111){
            unsigned *dest;
            dest = findData_HDTi(value, data_addr);
  //          printf("This is a HDT_i (STR) instruction from  R%d to address%p .\n",Rd, dest);
        }else{
  //          printf("This is a HDT_i (STR) instruction from R%d to R%d.\n", Rd, Rn);
        }
    }else{ //LDR
        written[Rd] = 1;
        if(Rn == 0b1111){
            unsigned *dest;
            dest = findData_HDTi(value, data_addr);
  //          printf("This is a HDT_i (LDR) instruction from address%p to R%d.\n", dest, Rd);
        }else{
   //         printf("This is a HDT_i (LDR) instruction from R%d to R%d.\n", Rn, Rd);
        }
    }
    r_counts[Rd]+=1; // read for str, write for ldr
}

/*Single Data Transfer Instruction*/
bool isSDT(unsigned *value){
    unsigned bits27_26 = get_bits(*value, 26,2);
    return  bits27_26 == 0b1;
}
void showSDT(unsigned *value, unsigned r_counts[], unsigned written[], unsigned data_addr[]){
    unsigned I = get_bits(*value, 25, 1);
    unsigned P = get_bits(*value, 24, 1);
    unsigned U = get_bits(*value, 23, 1);
    unsigned B = get_bits(*value, 22, 1);
    unsigned W = get_bits(*value, 21, 1);
    unsigned L = get_bits(*value, 20, 1);
    unsigned Rn = get_bits(*value, 16, 4);
    unsigned Rd = get_bits(*value, 12, 4);
    unsigned offset = get_bits(*value, 0, 12);
    //below covers only the Rd, Rn transfer, not to data sections
    if(W ==1 || (W == 0 && P == 0)) {
        written[Rn] = 1;
        r_counts[Rn]+=1;
    }
    if(L == 0){ //STR
        if(Rn == 0b1111){ //load from data section, calculate address
            unsigned *dest;
            dest = findData(value, data_addr);
   //         printf("This is a STR instruction from R%d to address%p.\n", Rd, dest);
        }else{ // is this even possible?
    //        printf("This is a STR instruction from R%d to R%d.\n", Rd, Rn);
        }
    }else{ //LDR
        if(written[Rd] == 0) written[Rd] = 1;
        if(Rn == 0b1111){ //load from data section, calculate address
            unsigned *dest;
            dest = findData(value, data_addr);
     //       printf("This is a LDR instruction from address%p to R%d.\n", dest, Rd);
        }else{
    //        printf("This is a LDR instruction from R%d to R%d.\n", Rn, Rd);
        }

    }

    if(I == 1){ //offset determined by the value in a register
            unsigned Rm = get_bits(*value, 0, 4);
            r_counts[Rm]+=1;
    }
    r_counts[Rd]+=1; r_counts[Rn]+=1; // even if ldr from data, Rn = 1111 will be read once
}
/*Block Data Transfer*/
bool isBDT(unsigned *value){
    unsigned bits27_25 = get_bits(*value, 25, 3);
    unsigned Rn = get_bits(*value, 16, 4);
    if (bits27_25 == 0b100 && Rn != 0b1111) //binary 100, Rn cannot be R15
        return true;
    else
        return false;
}

void showBDT(unsigned *value, unsigned r_counts[], unsigned written[]){
    unsigned Rn = get_bits(*value, 16, 4);
    unsigned L = get_bits(*value, 20, 1);
    unsigned W = get_bits(*value, 21, 1);
    if(W == 1){
        written[Rn] = 1;
        r_counts[Rn] += 1;//write
    }
    r_counts[Rn]+=1; // always read

    unsigned list[16];
    /* initialize register list array */
    int i;
    for ( i = 0; i < 16; i++ ){
      list[i] = 0;
      if(get_bits(*value, i, 1) == 1)
          list[i] = 1;
   }

    if(L == 0){// store to memory
     //   printf("This is a Block Data Transfer, store to R%d\n", Rn);
        int k;
        for(k = 0; k < 16; k++){
            if(list[k] == 1)
                r_counts[k] +=1;
        }
    }else{//ldr from memory
  // printf("This is a Block Data Transfer, load from R%d\n", Rn);
        int k;
        for(k = 0; k < 16; k++){
            if(list[k] == 1){
                written[k] = 1;
                r_counts[k] +=1;
            }
        }
    }
}

/*Branch*/
bool isBranch(unsigned *value){
    unsigned bits27_25 = get_bits(*value, 25, 3);
    if (bits27_25 == 0b101) //binary 101
        return true;
    else
        return false;
}

void showBranch(unsigned *value, unsigned r_counts[], unsigned written[], unsigned label_addr[]){
    unsigned offset = get_bits(*value, 0, 24);
    unsigned L = get_bits(*value, 24, 1);
    if(L == 1){
        r_counts[14]++;
        if(written[14] == 0) written[14] = 1;
    }
    unsigned* dest;
    dest = findLabel(value, label_addr);
  //  printf("dest_addr = %p\n", dest);
}
/* Coprocessor Instructions*/
bool isCP(unsigned *value){
    if(get_bits(*value, 25, 3) == 0b110 || get_bits(*value, 24, 4) == 0b1110)
        return true;
    else
        return false;
}
void showCP(unsigned *value, unsigned r_counts[], unsigned written[]){
    unsigned bits27_25 = get_bits(*value, 25, 3);
    unsigned bits27_24 = get_bits(*value, 24, 4);

    if(bits27_25 == 0b110 ){ // CP Data transfer
        unsigned Rn = get_bits(*value, 16,4);
        unsigned W = get_bits(*value, 21,1);
        if(W == 1 && Rn != 0b1111){
            written[Rn] == 1;
            r_counts[Rn]+=1;
        }
        r_counts[Rn]+=1;
   //     printf("CP Data Transfer Rn = %d\n", Rn);
    }else if(bits27_24 == 0b1110 && get_bits(*value, 4, 1) ==1){ // CP register transfer
        unsigned Rd = get_bits(*value, 12,4);
        unsigned L = get_bits(*value, 20,1);
        if(L == 1 && Rd != 0b1111){
            written[Rd] = 1;
     //       printf("written[%d] = %d\n", Rd, written[Rd]);
            r_counts[Rd]+=1;
    //        printf("CP Register Transfer, Load from CP to Rd = %d\n", Rd);
        }else if(L == 0){
            r_counts[Rd]+=1;
    //        printf("CP Register Transfer, Store to CP from Rd = %d\n", Rd);
        }
    }
}

struct basic_block {
    unsigned *base_address;
    int length;
    struct basic_block *next;
};
struct basic_block* create_block(unsigned  *base_address, int length);
void add_block(struct basic_block** phead, struct basic_block** ptail, struct basic_block* new_block);
void print_blocks  (struct basic_block *head);
int main(int argc, char **argv){
    /* Get the start and end address of the ARM machine code to analyze */
    //the names of the start and end symbols as char **argv??
    unsigned *start = (unsigned *) factorial; //cast function pointer to be unsigned pointer
    unsigned *end = (unsigned *) factorial_end;
    //save the address of data in between codes
    unsigned data_addr[100];
    //save the address of labels
    unsigned label_addr[100] 
    int i;
    for(i = 0; i< 100; i++){
        data_addr[i] = 0;
        label_addr[i] = 0;
    }

    //save register access numbers and written register list
    unsigned r_counts[16] ;
    unsigned written[16];
    for ( i = 0; i < 16; i++ ){
        r_counts[i] = 0;
        written[i] = 0;
        if(i == 15) written[i] = 1;
    }

   //declare the counters
    unsigned i_count = 0; // instruction
    unsigned dp_count = 0; //data processing+multiply
    unsigned b_count = 0; //b and bl and bx
    unsigned dt_count = 0;//data transfer
    unsigned c_count = 0;//coprocessor

    //declare the linkedlist of basic_blocks
    struct basic_block *head ;//head of the block list
    struct basic_block *tail ;//tail of the list
    head= NULL;
    tail = NULL;
    unsigned *base = start; //base_address for the growing block
    int len = 0 ;//length of the growing block

    /*First pass (1) to get the data and label addresses*/
    for(ip = start; ip<= end; ip++){
        if(isSDT(ip) && get_bits(*ip, 16, 4) == 0b1111)
             findData(ip, data_addr);
        if(isHDT_i(ip) && get_bits(*ip, 16, 4) == 0b1111)
             findData_HDTi(ip, data_addr);
        if(isBranch(ip))
             findLabel(ip, label_addr);
    }

    /* Iterate through all the instructions */
    for (ip = start; ip <= end; ip++) {
        //skip undefined instruction ??? what does the blank region actually mean?
        if(get_bits(*ip, 25, 3) == 0b011 && get_bits(*ip, 0, 25) == 0b10000)
            continue;
        //skip software interruption // what does that mean?
        if(get_bits(*ip, 24, 4) == 0b1111)
            continue;
        //skip data section
        if(isData(ip,data_addr))
            continue;

        //one more instruction at address ip
        i_count++;
//        printf("%p : ", ip);
//        print_bits(sizeof(unsigned), ip);  //  print_bits( sizeof(unsigned), (void const* const)(&bits) );

        //read and update PC
        r_counts[15]+=2;
        if(written[15] == 0){
            written[15] == 1;
        }

        //define base address and increment length for basic block
        if(len == 0)
            base = ip;
        else if(isLabel(ip,label_addr)){
            struct basic_block* new_block  = create_block(base, len);
            add_block(&head, &tail, new_block);
//            printf("I just added a block: %p, %d\n", base, len);
            len = 0;
            base = ip;
        }
        len++;

      //evaluate instruction type
        if(isBX(ip)){
            b_count++;
            //terminate and add the basic_block to list
            struct basic_block* new_block  = create_block(base, len);
            add_block(&head, &tail, new_block);
//            printf("I just added a block: %p, %d\n", base, len);
            len = 0;
            showBX(ip, r_counts, written);
            continue;
        }

        if(isSDS(ip)){
            dt_count++;
            showSDS(ip, r_counts, written);
            continue;
        }

        if(isHDT_r(ip)){
            dt_count++;
            showHDT_r(ip, r_counts, written);
            continue;
        }

        if(isMULT(ip)){
            dp_count++;
            showMULT(ip, r_counts, written);
            continue;
        }

        if(isHDT_i(ip)){
            dt_count++;
            showHDT_i(ip,r_counts,written, data_addr);
            continue;
        }

        if(isDP(ip)){
            dp_count++;
            showDP(ip, r_counts, written);
            continue;
        }

        if(isSDT(ip)){
            dt_count++;
            showSDT(ip, r_counts, written, data_addr);
            continue;
        }

        if(isBDT(ip)){
            dt_count++;
            showBDT(ip, r_counts, written);
            continue;
        }

        if(isBranch(ip)){
            b_count++;
            struct basic_block* new_block  = create_block(base, len);
            add_block(&head, &tail, new_block);
//            printf("I just added a block: %p, %d\n", base, len);
            len = 0;
            showBranch(ip, r_counts, written, label_addr);
            continue;
        }

        if(isCP(ip)){
            c_count++;
            showCP(ip, r_counts, written);
            continue;
        }

    }

    printf("\nInstruction statistics:\n");
    printf("i_count = %d\n", i_count);
    printf("dp_count = %d\n", dp_count);
    printf("b_count = %d\n", b_count);
    printf("dt_count = %d\n", dt_count);
    printf("c_count = %d\n", c_count);

    printf("\nRegister access counts:\n");
    for( i = 0 ; i < 16; i++){
        if(r_counts[i] > 0)
            printf("R%d: %d; ", i, r_counts[i]);
    }
    printf("\nRegisters written by the function:\n");
    for(i = 0 ; i < 16; i++){
        if(written[i] == 1)
            printf("R%d; ", i);
    }

    printf("\nCallee saved registers:\n");
    for(i = 4 ; i <= 11; i++){
        if(written[i] == 1)
            printf("R%d; ", i);
    }
    puts("\n");
    print_blocks(head);
    return 0;
}

struct basic_block* create_block(unsigned  *base_address, int length)
{
  struct basic_block* n = (struct basic_block*) malloc(sizeof(struct basic_block));
  n->base_address = base_address;
  n->length = length;
  n->next = NULL;
  return n;
}

void add_block(struct basic_block** phead, struct basic_block** ptail, struct basic_block* new_block)
{   
    if(*phead == NULL){
        *phead = new_block;
        *ptail = new_block;
    }else{
        (*ptail)->next = new_block;
        (*ptail) = new_block;
    }
}

void  print_blocks  (struct basic_block *head){
    printf("Basic blocks: \n");
    if (head == NULL) {
        printf("No basic blocks!\n");
        return;
    }
    struct basic_block *tmp = head;
    while (tmp != NULL) {
        printf("(%p, %d)\n", tmp->base_address,tmp->length);
        tmp = tmp->next;
    }
    printf("\n");
}

