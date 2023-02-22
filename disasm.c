#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void Print_CRI(char** CRInsts, int ThisCRI, uint32_t crbD, uint32_t crbA, uint32_t crbB)
{
    printf("%s\t %d, %d, %d\n", CRInsts[ThisCRI], crbD, crbA, crbB);
    return;
}
void Print7C_Form1(char** SevC_Insts, int Inst, uint32_t Rc, uint32_t rB, uint32_t rA, uint32_t rS)
{
    printf("%s%s%d, r%d, r%d\n", SevC_Insts[Inst], Rc == 1? ".\t r" : "\t r", rA, rS, rB);
    return;
}
void Print7C_IndexedLS(char* IndexedLS, int Float, int B31, uint32_t rS, uint32_t rA, uint32_t rB)
{
    if (B31 == 0) {
    printf("%sx\t %c%d, r%d, r%d\n", IndexedLS, Float == 1? 'f' : 'r', rS, rA, rB);
    } else printf("(unk)\n");
    return;
}
void Print7C_Math(char* Inst, char** MathModes, int Mode, uint32_t rD, uint32_t rA, uint32_t rB)
{
    printf("%s%s\t r%d, r%d, r%d\n", Inst, MathModes[Mode], rD, rA, rB);
    return;
}
void Print7C_DCache(char** CacheInsts, int This_Inst, uint32_t rA, uint32_t rB, int B6_5, int B31)
{
    if (B6_5 == 0 && B31 == 0) {
    printf("dcb");
    printf("%s\t r%d, r%d\n", CacheInsts[This_Inst], rA, rB);
    } else printf("(unk)\n");
    return;
}
void Print_FCMP(char FCType, uint8_t crfD, uint32_t frA, uint32_t frB, uint8_t B9, uint8_t B31)
{
    if (B9 == 0 && B31 == 0) {
    printf("fcmp%c\t cr%d, f%d, f%d\n", FCType, crfD, frA, frB);
    } else printf("(unk)\n");
    return;
}
void Parse_MSPR(char Direction, uint32_t Split, uint32_t rD)
{
    switch(Split)
    {
    case 1:
    printf("m%cxer\t r%d", Direction, rD);
    break;

    case 8:
    printf("m%clr\t r%d", Direction, rD);
    break;

    case 9:
    printf("m%cctr\t r%d", Direction, rD);
    break;

    case 18:
    printf("m%cdsisr\t r%d", Direction, rD);
    break;

    case 19:
    printf("m%cdar\t r%d", Direction, rD);
    break;

    default:
    printf("m%cspr\t r%d, %d", Direction, rD, Split);
    break;
    }

    printf("\n");
    return;

}
void Print_FCat1(char* FCat1, uint32_t frD, uint32_t frA, uint32_t frB, uint32_t B21_5, int Rc)
{
    if (B21_5 == 0) {
        printf("%s%s%d, f%d, f%d\n", FCat1, Rc == 1? ".\t f" : "\t f", frD, frA, frB);
    } else printf("(unk)\n");
    return;
}

void DisASM(uint32_t Instruction, uint32_t Inj_Addr)
{
static char *LDST[] = {
  "lwz","lwzu","lbz","lbzu","stw","stwu","stb","stbu","lhz","lhzu",
  "lha","lhau","sth","sthu","lmw","stmw","lfs","lfsu","lfd","lfdu",
  "stfs","stfsu","stfd","stfdu"
},

*Bitwise_6[] = { "ori","oris","xori","xoris","andi.","andis." },
*Types[] = { "\0","l","a","la" },
*Basic_Imms[] = { "ic","ic.","i","is" },
*MathModes[] = { "\0",".","o","o." },
*CMS[] = { "pl","p" },

*SevC_Insts[] = {
  "slw","and","andc","eqv","eciwx","xor","orc","ecowx","or",
  "nand","srw","sraw","nor"
},

*CRInsts[] = {
  "crnor","crandc","crxor","crnand","crand","creqv",
  "crorc","cror"
},

*IndexedLS[] = {
  "lwar","lsw","lwbr","stsw","stwbr","lhbr","sthbr","stfiw"
},

*FCat1[] = { "fdiv","fsub","fadd","fdivs","fsubs","fadds" },
*CondSet1[] = { "ge","le","ne","ns" },
*CondSet2[] = { "lt","gt","eq","so" },
*DCache_Insts[] = { "st","f","tst","t","i","z","z_l" };

char GPR = 'r',
FPR = 'f';

// Global reusable flag
uint32_t FLG = 0;

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
uint32_t L = (Instruction << 10) >> 31,

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
int32_t B16_16 = (Instruction << 16) >> 16,
LIS_B16_16 = B16_16;

// This field: I field 1 (specify GQR control register used by PS load/store)
uint8_t B17_3 = (Instruction << 18) >> 29;

// Extended opcode field 1
uint32_t XO1 = (Instruction << 21) >> 22;

// This field: frC (specify FPR as source) and MB (rotate instruction mask)
uint8_t B21_5 = (Instruction << 21) >> 27,

// OE for 7C-math instructions
OE = (Instruction << 21) >> 31;

// Extended opcode field 2
uint32_t XO2 = (Instruction << 22) >> 23,

// Mask end (rotate left instructions)
B26_5 = (Instruction << 26) >> 27;

// Absolute address bit for branches
uint8_t AA = (Instruction << 30) >> 31,

// LK for branches, Rc for some instructions
B31 = (Instruction << 31) >> 31;

// Branch type definition
uint32_t BLK = 0;
if (AA == 1 && B31 == 1) { BLK = 3; }
if (AA == 1 && B31 == 0) { BLK = 2; }
if (AA == 0 && B31 == 1) { BLK = 1; }

// Illegal instructions
if (OP == 1 || OP == 2 || OP == 5 || OP == 6 || OP == 9 || OP == 22 || OP == 30 || OP == 58 || OP == 62) {
    printf("illegal opcode\n");
    return;
}

if (OP == 3) {
    printf("twi %d, r%d%s%d\n", B6_5, B11_5, B16_16 >= 0x8000? ", -" : ", \0", B16_16 >= 0x8000? (0x10000 - B16_16) : B16_16);
    return;
}

if (OP == 4) {
    if (B6_5 == 0 && XO1 == 1014 && B31 == 0) {
    Print7C_DCache(DCache_Insts, 6, B11_5, B16_5, B6_5, B31);
    return;
    }
}

if (OP == 7) {
    printf("mulli\t r%d, r%d%s0x%X\n", B6_5, B11_5, B16_16 >= 0x8000? ", -" : ", \0", B16_16 >= 0x8000? (0x10000 - B16_16) : B16_16);
    return;
}

if (OP == 8) {
    printf("subfic\t r%d, r%d%s%d\n", B6_5, B11_5, B16_16 >= 0x8000? ", -" : ", \0", B16_16 >= 0x8000? (0x10000 - B16_16) : B16_16);
    return;
}

// cmpi/cmpli
if (OP == 10 || OP == 11) {

    if (B16_16 >= 0x8000 && OP == 11) { FLG = 0x10000 - B16_16; } else FLG = B16_16;
    if (B9 == 0) {
    printf("cm%s%ci\t cr%d, r%d%s0x%X\n", CMS[OP - 10], L == 1? 'd' : 'w', crfD, B11_5, B16_16 >= 0x8000 && OP == 11? ", -" : ", \0", FLG);
    return;
    }
}

if (Instruction == 0x44000002) { printf("sc\n");
    return;
}

//Add/sub immediate
if (OP >= 12 && OP <= 15) {

    // B16_16 = Immediate, B6_5 = Target, B11_5 = Source
    char *This_Imm[] = { "sub", "add" };
    int ImmID = OP - 12;
    if (B16_16 >= 0x8000) { B16_16 = 0x10000 - B16_16;
    } else FLG = 1;

    if (OP == 14 && B11_5 == 0) { printf("li\t r%d%s%d\n", B6_5, FLG == 0? ", -" : ", \0", B16_16);
    } else if (OP == 15 && B11_5 == 0) { printf("lis\t r%d, 0x%04X\n", B6_5, LIS_B16_16);
    } else printf("%s%s\t r%d, r%d, 0x%X\n", This_Imm[FLG], Basic_Imms[ImmID], B6_5, B11_5, B16_16);
    return;
}

// Conditional branches (incomplete)
if (OP == 16) {

    uint32_t Target = B16_16 >= 0x8000? (Inj_Addr - (0x10000 - B16_16)) : (Inj_Addr + B16_16);
    // For Absolute types
    if (BLK >= 2) { Target = B16_16 >= 0x8000? (0xFFFF0000 + B16_16) : B16_16; }
    uint32_t Cond = B11_5 - (4 * (B11_5 / 4));

    if (B6_5 == 4 || B6_5 == 12) {
        if (B11_5 / 4 == 0) {
        printf("b%s%s\t 0x%X\n", B6_5 == 4? CondSet1[Cond] : CondSet2[Cond], Types[BLK], Target - BLK);
        } else printf("b%s%s\t cr%d ->0x%X\n", B6_5 == 4? CondSet1[Cond] : CondSet2[Cond], Types[BLK], B11_5 / 4, Target - BLK);
        return;
    }
    printf("bc%s\t %d, %d ->0x%X\n", Types[BLK], B6_5, B11_5, Target - BLK);
    return;
}

// Non-conditional branches
if (OP == 18) {

    // B31 = LK, LI = JumpAddress
    // Using this as a method to identify backward direction for now, but it might be different (not described in table 12.1.2/12.2 of 750CL Manual)
    int BW = (Instruction << 5) >> 30;
    uint32_t Relative_Addr;

    if (BW == 0) { Relative_Addr = Inj_Addr + (LI - BLK);
    } else Relative_Addr = Inj_Addr - ((0x1000000 - LI) + BLK);

    printf("b%s\t 0x%X\n", Types[BLK], BLK <= 1? Relative_Addr : BW == 1? (LI - BLK) + 0xFF000000 : (LI - BLK));
    return;
}

if (Instruction == 0x4C000064) { printf("rfi\n");
    return;
}
if (Instruction == 0x4C00012C) { printf("isync\n");
    return;
}
// Temporary workaround
if (Instruction == 0x4E800020) { printf("blr\n\n");
    return;
}
// Temporary workaround
if (Instruction == 0x4E800021) { printf("blrl\n");
    return;
}
// Temporary workaround
if (Instruction == 0x4E800420) { printf("bctr\n\n");
    return;
}
// Temporary workaround
if (Instruction == 0x4E800421) { printf("bctrl\n\n");
    return;
}

if (OP == 19 && B31 == 0) {
    switch(XO1) {

    case 33:
    Print_CRI(CRInsts, 0, B6_5, B11_5, B16_5);
    return;

    case 129:
    Print_CRI(CRInsts, 1, B6_5, B11_5, B16_5);
    return;

    case 193:
    if (B11_5 == B16_5) {
        printf("crclr\t %d, %d\n", B6_5, B11_5);
        return;
    } else Print_CRI(CRInsts, 2, B6_5, B11_5, B16_5);
    return;

    case 225:
    Print_CRI(CRInsts, 3, B6_5, B11_5, B16_5);
    return;

    case 257:
    Print_CRI(CRInsts, 4, B6_5, B11_5, B16_5);
    return;

    case 289:
    Print_CRI(CRInsts, 5, B6_5, B11_5, B16_5);
    return;

    case 417:
    Print_CRI(CRInsts, 6, B6_5, B11_5, B16_5);
    return;

    case 449:
    Print_CRI(CRInsts, 7, B6_5, B11_5, B16_5);
    return;
    }
}

if (OP == 20 || OP == 21)
{
    char *RLWInsts[] = { "rlwimi", "rlwinm" };
    printf("%s%s%d, r%d, %d, %d, %d\n", RLWInsts[OP - 20], B31 == 1? ".\t r" : "\t r", B11_5, B6_5, B16_5, B21_5, B26_5);
    return;
}

// 6 logical operations
if (OP >= 24 && OP <= 29) {

    // B11_5 = Target, B6_5 = Source, B16_16 = Immediate
    int BT_ID = OP - 24;
    if (Instruction == 0x60000000) { printf("nop\n"); }
    else printf("%s\t r%d, r%d, 0x%X\n", Bitwise_6[BT_ID], B11_5, B6_5, B16_16);
    return;
}

if (Instruction == 0x7C00046C) { printf("tlbsync\n");
    return;
}
if (Instruction == 0x7C0004AC) { printf("sync\n");
    return;
}
if (Instruction == 0x7C0006AC) { printf("eieio\n");
    return;
}

// literally all the 0x7C-category, I guess
if (OP == 31) {

// Determine math mode for OE instructions
if (OE == 0 && B31 == 1) { FLG = 1; }
if (OE == 1 && B31 == 0) { FLG = 2; }
if (OE == 1 && B31 == 1) { FLG = 3; }

    switch(XO2)
    {
    case 8:
    Print7C_Math("subc", MathModes, FLG, B6_5, B11_5, B16_5);
    return;

    case 10:
    Print7C_Math("addc", MathModes, FLG, B6_5, B11_5, B16_5);
    return;

    case 11:
    if (OE == 0) {
        Print7C_Math("mulhwu", MathModes, FLG, B6_5, B11_5, B16_5);
        return;
    }   break;

    case 40:
    Print7C_Math("sub", MathModes, FLG, B6_5, B11_5, B16_5);
    return;

    case 75:
    // Guess it doesn't use OE
    if (OE == 0) {
        Print7C_Math("mulhw", MathModes, FLG, B6_5, B11_5, B16_5);
        return;
    }   break;

    case 136:
    Print7C_Math("subfe", MathModes, FLG, B6_5, B11_5, B16_5);
    return;

    case 138:
    Print7C_Math("adde", MathModes, FLG, B6_5, B11_5, B16_5);
    return;

    case 235:
    Print7C_Math("mulhw", MathModes, FLG, B6_5, B11_5, B16_5);
    return;

    case 266:
    Print7C_Math("add", MathModes, FLG, B6_5, B11_5, B16_5);
    return;

    case 459:
    Print7C_Math("divwu", MathModes, FLG, B6_5, B11_5, B16_5);
    return;

    case 491:
    Print7C_Math("divw", MathModes, FLG, B6_5, B11_5, B16_5);
    return;
    }

    switch(XO1)
    {
    // dcbz
    case 1014:
        Print7C_DCache(DCache_Insts, 5, B11_5, B16_5, B6_5, B31);
        return;

    case 954:
        if (B16_5 == 0) {
        printf("extsb%c\t r%d, r%d\n", B31 == 1? '.' : '\0', B6_5, B11_5);
        return;
    }   break;

    case 922:
        if (B16_5 == 0) {
        printf("extsh%c\t r%d, r%d\n", B31 == 1? '.' : '\0', B6_5, B11_5);
        return;
    }   break;

    // sthbrx
    case 918:
        Print7C_IndexedLS(IndexedLS[6], 0, B31, B6_5, B11_5, B16_5);
        return;

    case 824:
        printf("srawi%c\t r%d, r%d, %d\n", B31 == 1? '.' : ' ', B6_5, B11_5, B16_5);
        return;

    // sraw
    case 792:
        Print7C_Form1(SevC_Insts, 11, B31, B16_5, B11_5, B6_5);
        return;

    // lhbrx
    case 790:
        Print7C_IndexedLS(IndexedLS[5], 0, B31, B6_5, B11_5, B16_5);
        return;

    // stfsux
    case 695:
        Print7C_IndexedLS(LDST[21], 1, B31, B6_5, B11_5, B16_5);
        return;

    // stfsx
    case 663:
        Print7C_IndexedLS(LDST[20], 1, B31, B6_5, B11_5, B16_5);
        return;

    // stwbrx
    case 662:
        Print7C_IndexedLS(IndexedLS[4], 0, B31, B6_5, B11_5, B16_5);
        return;

    // stswx
    case 661:
        Print7C_IndexedLS(IndexedLS[3], 0, B31, B6_5, B11_5, B16_5);
        return;

    // lfdux
    case 631:
        Print7C_IndexedLS(LDST[19], 1, B31, B6_5, B11_5, B16_5);
        return;

    // lfdx
    case 599:
        Print7C_IndexedLS(LDST[18], 1, B31, B6_5, B11_5, B16_5);
        return;

    case 597:
        if (B31 == 0) {
        printf("lswi\t r%d, r%d, %d\n", B6_5, B11_5, B16_5);
        return;
    }   break;

    // lfsux
    case 567:
        Print7C_IndexedLS(LDST[17], 1, B31, B6_5, B11_5, B16_5);
        return;

    // srw
    case 536:
        Print7C_Form1(SevC_Insts, 10, B31, B16_5, B11_5, B6_5);
        return;

    // lfsx
    case 535:
        Print7C_IndexedLS(LDST[16], 1, B31, B6_5, B11_5, B16_5);
        return;

    // lwbrx
    case 534:
        Print7C_IndexedLS(IndexedLS[2], 0, B31, B6_5, B11_5, B16_5);
        return;

    // nand
    case 476:
        Print7C_Form1(SevC_Insts, 9, B31, B16_5, B11_5, B6_5);
        return;

    // dcbi
    case 470:
        Print7C_DCache(DCache_Insts, 4, B11_5, B16_5, B6_5, B31);
        return;

    // mtspr
    case 467:
        if (B31 == 0) { Parse_MSPR('t', Split, B6_5);
        return;
    }   break;

    // or (with mr)
    case 444:
        if (B6_5 == B16_5) {
        printf("mr%s%d, r%d\n", B31 == 1? ".\t r" : "\t r", B11_5, B6_5);
        } else Print7C_Form1(SevC_Insts, 8, B31, B16_5, B11_5, B6_5);
        return;

    // sthux
    case 439:
        Print7C_IndexedLS(LDST[13], 0, B31, B6_5, B11_5, B16_5);
        return;

    // ecowx
    case 438:
        if (B31 == 0) {
        Print7C_Form1(SevC_Insts, 7, B31, B16_5, B6_5, B11_5);
        return;
    }   break;

    // orc
    case 412:
        Print7C_Form1(SevC_Insts, 6, B31, B16_5, B11_5, B6_5);
        return;

    // sthx
    case 407:
        Print7C_IndexedLS(LDST[12], 0, B31, B6_5, B11_5, B16_5);
        return;

    // lhaux
    case 375:
        Print7C_IndexedLS(LDST[11], 0, B31, B6_5, B11_5, B16_5);
        return;

    case 371:
        if (B31 == 0) {
        if (Split == 268 || Split == 269) {
        printf("mftb%c\t r%d\n", Split == 268? 'l' : 'u', B6_5);
        return;
      }
    }   break;

    // lhax
    case 343:
        Print7C_IndexedLS(LDST[10], 0, B31, B6_5, B11_5, B16_5);
        return;

    // mfspr
    case 339:
        if (B31 == 0) { Parse_MSPR('f', Split, B6_5);
        return;
    }   break;

    // xor
    case 316:
        Print7C_Form1(SevC_Insts, 5, B31, B16_5, B11_5, B6_5);
        return;

    // lhzux
    case 311:
        Print7C_IndexedLS(LDST[9], 0, B31, B6_5, B11_5, B16_5);
        return;

    // eciwx
    case 310:
        if (B31 == 0) {
        Print7C_Form1(SevC_Insts, 4, B31, B16_5, B6_5, B11_5);
        return;
    }   break;

    // tlbie
    case 306:
        if (B6_5 == 0 && B11_5 == 0 && B31 == 0) {
        printf("tlbie\t r%d\n", B16_5);
        return;
    }   break;

    // eqv
    case 284:
        Print7C_Form1(SevC_Insts, 3, B31, B16_5, B11_5, B6_5);
        return;

    // lhzx
    case 279:
        Print7C_IndexedLS(LDST[8], 0, B31, B6_5, B11_5, B16_5);
        return;

    // dcbt
    case 278:
        Print7C_DCache(DCache_Insts, 3, B11_5, B16_5, B6_5, B31);
        return;

    // stbux
    case 247:
        Print7C_IndexedLS(LDST[7], 0, B31, B6_5, B11_5, B16_5);
        return;

    // dcbtst
    case 246:
        Print7C_DCache(DCache_Insts, 2, B11_5, B16_5, B6_5, B31);
        return;

    // mtsrin
    case 242:
        if (B11_5 == 0 && B31 == 0) { printf("mtsrin\t r%d, r%d\n", B6_5, B16_5);
        return;
    }   break;

    // stbx
    case 215:
        Print7C_IndexedLS(LDST[6], 0, B31, B6_5, B11_5, B16_5);
        return;

    // stwux
    case 183:
        Print7C_IndexedLS(LDST[5], 0, B31, B6_5, B11_5, B16_5);
        return;

    // stwx
    case 151:
        Print7C_IndexedLS(LDST[4], 0, B31, B6_5, B11_5, B16_5);
        return;

    // stwcx.
    case 150:
        if (B31 == 1) {
        printf("stwcx.\t r%d, r%d, r%d\n", B6_5, B11_5, B16_5);
        return;
    }   break;

    // Late addition - nor/not
    case 124:
    if (B6_5 == B16_5) {
        printf("not%s%d, r%d\n", B31 == 1? ".\t r" : "\t r", B6_5, B16_5);
    } else Print7C_Form1(SevC_Insts, 12, B31, B16_5, B11_5, B6_5);
      return;

    // lbzux
    case 119:
        Print7C_IndexedLS(LDST[3], 0, B31, B6_5, B11_5, B16_5);
        return;

    // lbzx
    case 87:
        Print7C_IndexedLS(LDST[2], 0, B31, B6_5, B11_5, B16_5);
        return;

    // dcbf
    case 86:
        Print7C_DCache(DCache_Insts, 1, B11_5, B16_5, B6_5, B31);
        return;

    // andc
    case 60:
        Print7C_Form1(SevC_Insts, 2, B31, B16_5, B11_5, B6_5);
        return;

    // lwzux
    case 55:
        Print7C_IndexedLS(LDST[1], 0, B31, B6_5, B11_5, B16_5);
        return;

    // dcbst
    case 54:
        Print7C_DCache(DCache_Insts, 0, B11_5, B16_5, B6_5, B31);
        return;

    case 32:
        if (B9 == 0 && B31 == 0) { printf("cmpl%c\t cr%d, r%d, r%d\n", L == 1? 'd' : 'w', crfD, B11_5, B16_5);
        return;
    }   break;

    // and
    case 28:
        Print7C_Form1(SevC_Insts, 1, B31, B16_5, B11_5, B6_5);
        return;

    case 26:
        if (B16_5 == 0) {
        printf("cntlzw%c\t r%d, r%d\n", B31 == 1? '.' : '\0', B6_5, B11_5);
        return;
    }   break;

    // slw
    case 24:
        Print7C_Form1(SevC_Insts, 0, B31, B16_5, B11_5, B6_5);
        return;

    // lwzx
    case 23:
        Print7C_IndexedLS(LDST[0], 0, B31, B6_5, B11_5, B16_5);
        return;

    // lwarx
    case 20:
        Print7C_IndexedLS(IndexedLS[0], 0, B31, B6_5, B11_5, B16_5);
        return;

    case 0:
        if (B9 == 0 && B31 == 0) { printf("cmp%c\t cr%d, r%d, r%d\n", L == 1? 'd' : 'w', crfD, B11_5, B16_5);
        return;
      } break;
    }
}

// load & store
if (OP >= 32 && OP <= 55) {

    // B6_5 = Target, B16_16 = Offset, B11_5 = Source
	if (OP >= 48) { GPR = FPR; }
	int LS_ID = OP - 32;
	printf("%s\t %c%d, %sx%X (r%d)\n", LDST[LS_ID], GPR, B6_5, B16_16 >= 0x8000? "-0" : "0", B16_16 >= 0x8000? (0x10000 - B16_16) : B16_16, B11_5);
	return;
}

// Floating-Point Arithmetic Instructions 1
if (OP == 59) {
    switch(B26_5)
    {
    // fdivs
    case 18:
    Print_FCat1(FCat1[3], B6_5, B11_5, B16_5, B21_5, B31);
    return;

    // fsubs
    case 20:
    Print_FCat1(FCat1[4], B6_5, B11_5, B16_5, B21_5, B31);
    return;

    // fadds
    case 21:
    Print_FCat1(FCat1[5], B6_5, B11_5, B16_5, B21_5, B31);
    return;
    }
}

if (OP == 63) {

    if (XO1 == 0) {
    Print_FCMP('u', crfD, B11_5, B16_5, B9, B31);
    return;

    } if (XO1 == 32) {
    Print_FCMP('o', crfD, B11_5, B16_5, B9, B31);
    return;
    }
}

printf("(unk)\n");
return;
}
