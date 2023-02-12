#include "Gekko_SPR.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void DisASM(uint32_t Instruction, uint32_t Inj_Addr)
{
static char *LDST[] = {
  "lwz","lwzu","lbz","lbzu","stw","stwu","stb","stbu","lhz","lhzu",
  "lha","lhau","sth","sthu","lmw","stmw","lfs","lfsu","lfd","lfdu",
  "stfs","stfsu","stfd","stfdu"
},

*Bitwise_6[] = { "ori","oris","xori","xoris","andi.","andis." },
*BCS[] = { "b","bl","ba","bla" },
*Basic_Imms[] = { "ic","ic.","i","is" },
*CMS[] = { "pl", "p" };

// Longest use case: ps_merge10. (with Rc bit on), 12 characters with terminator
char InstructionName[12];
char GPR = 'r',
FPR = 'f';

// Multi-purpose flag
uint32_t BLK = 0;

// Opcode value
uint8_t OP = Instruction >> 26,

// Specifies one of the CR/FPSCR fields as a destination
crfD = (Instruction << 6) >> 29,

// Format: BX_Y (X = starting slot, Y = bit amount)
/* This field: BO (used to specify options for branch conditional instructions)
crbD (used to specify CR/FPSCR bit as destination of instruction result)
frD (used to specify FPR as destination)
frS (used to specify FPR as source)
rD (used to specify GPR as destination)
rS (used to specify GPR as source)
TO (for trap instructions; used to specify the conditions on which to trap) */
B6_5 = (Instruction << 6) >> 27;

// Field mask used to identify FPSCR fields to be updated by mtfsf
uint32_t FM = (Instruction << 7) >> 24;

/* "Immediate field specifying a 24-bit signed two's complement integer that is concatenated on the right with 0b00 and sign-extended to 32 bits"
This thing doesn't make sense, I swear, so it might not be accurate for now */
uint32_t LI = (Instruction << 8) >> 8;

// Blank field required for completion of cmp instructions
uint8_t B9 = (Instruction << 9) >> 31;

// Not described by User Manual? "Must be set to 0 for 32-bit subset arcitecture" according to IBM site
uint8_t L = (Instruction << 10) >> 31,

// Specifies one of the CR/FPSCR fields as a source
crfS = (Instruction << 12) >> 29,

/* This field:
BI (used to specify CR bit for usage as conditional branch condition)
crbA (used to specify CR bit for use as source)
frA (used to specify FPR as source)
rA (used to specify GPR as source or destination) */
B11_5 = (Instruction << 11) >> 27;

// SPR, TBL/TBU
uint32_t Split = ((Instruction << 11) >> 27) | (((Instruction << 16) >> 27) << 5);

// Used to specify one of the 16 segment registers
uint8_t SR = (Instruction << 13) >> 28;

// Used to identify the CR fields to be updated by mtcrf
uint32_t CRM = (Instruction << 13) >> 24,

// Immediate field used as data to be placed into FPSCR field
IMM = (Instruction << 16) >> 28,

/* This field:
crbB (used to specify CR bit for use as source)
frB (specify FPR as source)
NB (specify the number of bytes to move in an immediate string load or store)
rB (specify GPR as source)
SH (specify shift amount) */
B16_5 = (Instruction << 16) >> 27;

/* This field:
SIMM (specify signed 16-bit int) and UIMM (specify 16-bit unsigned int)
d field 1 (signed two's complement integer that is sign-extended to 32 bits) */
int32_t B16_16 = (Instruction << 16) >> 16;

// This field: I field 1 (specify GQR control register used by PS load/store)
uint8_t B17_3 = (Instruction << 18) >> 29;

// Extended opcode field 1
uint32_t XO1 = (Instruction << 21) >> 22;

// This field: frC (specify FPR as source) and MB (rotate instruction mask)
uint8_t B21_5 = (Instruction << 22) >> 27;

// Extended opcode field 2
uint32_t XO2 = (Instruction << 23) >> 23;

// Absolute address bit for branches
uint8_t AA = (Instruction << 30) >> 31,

// LK for branches, Rc for some instructions
B31 = (Instruction << 31) >> 31;

// cmpi/cmpli
if (OP == 10 || OP == 11) {

    if (B16_16 >= 0x8000 && OP == 11) { BLK = 0x10000 - B16_16; } else BLK = B16_16;
    if (B9 == 0) {
    printf("cm%s%ci cr%d, r%d%s%d\n", CMS[OP - 10], L == 1? 'd' : 'w', crfD, B11_5, B16_16 >= 0x8000 && OP == 11? ", -" : ", \0", BLK);
    return;
    }
}

//Add/sub immediate
if (OP >= 12 && OP <= 15) {

    // B16_16 = Immediate, B6_5 = Target, B11_5 = Source
    char *This_Imm[] = { "sub", "add" };
    int ImmID = OP - 12;
    if (B16_16 >= 0x8000) { B16_16 = 0x10000 - B16_16;
    } else BLK = 1;

    if (OP == 14 && B11_5 == 0) { printf("li r%d%s%d\n", B6_5, BLK == 0? ", -" : ", \0", B16_16);
    } else if (OP == 15 && B11_5 == 0) { printf("lis r%d, 0x%04X\n", B6_5, B16_16);
    } else printf("%s%s r%d, r%d, %d\n", This_Imm[BLK], Basic_Imms[ImmID], B6_5, B11_5, B16_16);
    return;
}

// Non-conditional branches
if (OP == 18) {

    // B31 = LK, LI = JumpAddress
    // Using this as a method to identify backward direction for now, but it might be different (not described in table 12.1.2/12.2 of 750CL Manual)
    int BW = (Instruction << 5) >> 30;
    if (AA == 1 && B31 == 1) { BLK = 3; }
    if (AA == 1 && B31 == 0) { BLK = 2; }
    if (AA == 0 && B31 == 1) { BLK = 1; }

    uint32_t Relative_Addr;
    if (BW == 0) { Relative_Addr = Inj_Addr + (LI - BLK);
    } else Relative_Addr = Inj_Addr - (0x1000000 - LI);

    printf("%s 0x%X\n", BCS[BLK], BLK <= 1? Relative_Addr : BW == 1? (LI - BLK) + 0xFF000000 : (LI - BLK));
    return;
}

if (Instruction == 0x4C000064) { printf("rfi\n"); } return;

// 6 logical operations
if (OP >= 24 && OP <= 29) {

    // B11_5 = Target, B6_5 = Source, B16_16 = Immediate
    int BT_ID = OP - 24;
    if (Instruction == 0x60000000) { printf("nop\n"); }
    else printf("%s r%d, r%d, 0x%X\n", Bitwise_6[BT_ID], B11_5, B6_5, B16_16);
    return;
}

// literally all the 0x7C-category, I guess
if (OP == 31) {

    switch(XO1)
    {
    case 854:
        printf("eieio\n");
        return;

    case 536:
        printf("srw%s%d, r%d, r%d\n", B31 == 1? ". r" : " r", B11_5, B6_5, B16_5);
        return;

    case 444:
        if (B6_5 == B16_5) {
        printf("mr%s%d, r%d\n", B31 == 1? ". r" : " r", B11_5, B6_5, B16_5);
        } else printf("or%s%d, r%d, r%d\n", B31 == 1? ". r" : " r", B11_5, B6_5);
        return;

    case 412:
        printf("orc%s%d, r%d, r%d\n", B31 == 1? ". r" : " r", B11_5, B6_5);
        return;

    case 0:
        if (B9 == 0 && B31 == 0) { printf("cmp%c cr%d, r%d, r%d\n", L == 1? 'd' : 'w', crfD, B11_5, B16_5);
        return;
        }
    }
}

// load & store
if (OP >= 32 && OP <= 55) {

    // B6_5 = Target, B16_16 = Offset, B11_5 = Source
	if (OP >= 48) { GPR = FPR; }
	int LS_ID = OP - 32;
	printf("%s %c%d, %sx%X (r%d)\n", LDST[LS_ID], GPR, B6_5, B16_16 >= 0x8000? "-0" : "0", B16_16 >= 0x8000? (0x10000 - B16_16) : B16_16, B11_5);
	return;
}

printf("(unk)\n");
return;
}
