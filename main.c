#include <stdint.h>
#include <string.h>
#include "DolFile.h"
#include "Gekko_SPR.h"
#include <stdio.h>
#include <stdlib.h>

uint32_t Fix_Big_Endian(uint32_t VTF)
{
    // Todo: don't use this function on possible big-endian targets
    uint32_t Fixed_Val = ((((VTF & 0xFF000000) >> 24) + ((VTF & 0x00FF0000) >> 8)) + ((VTF & 0xFF00) << 8)) + ((VTF << 24));
    return Fixed_Val;
}
void DisASM(uint32_t Instruction, uint32_t Inj_Addr)
{
    // The intended bits 0-5 identifier for most PPC instructions
    uint8_t Identifier1 = Instruction >> 26;
    char InstructionName[7];
    char GPR = 'r';
    char FPR = 'f';
    // source register
    uint8_t RA = (((1 << 5) - 1) & (Instruction >> (17 - 1)));
    // target register
    uint8_t RT = (((1 << 5) - 1) & (Instruction >> (22 - 1)));
    // for spr instructions (also used as bits 21-30 blank spot for some fp instructions)
    uint32_t SPR = ((Instruction << 11) >> 27) | (((Instruction << 16) >> 27) << 5);
    // for cmp instructions, cr field
    uint8_t crfD = (((1 << 3) - 1) & (Instruction >> (24 - 1)));
    // for cmpw instructions, this field changes between di/wi
    uint8_t L = (((1 << 1) - 1) & (Instruction >> (22 - 1)));
    // Update LR flag for branch instructions, reusing as Rc bit for the rest
    uint8_t LK = (Instruction << 31) >> 31;
    // for offset-based load/store, effective address
    uint32_t SID = (Instruction << 16) >> 16;
    // for spr/float instructions; identifier
    uint32_t Identifier2 = (Instruction << 22) >> 23;
    // Flag + SID clone for proper identification of negative SID (might be a stupid workaround)
    int NegativeSID;
    uint32_t SID2 = 0;
    if (SID >= 0x8000) { SID2 = 0x10000 - SID;
    NegativeSID = 1; } else NegativeSID = 0;

    // cmp(l)wi, cmp(l)di
    if (Identifier1 == 10) {
        if (L == 1) { printf("cmpldi cr%d, r%d, %d\n", crfD, RA, SID); }
        else printf("cmplwi cr%d, r%d, %d\n", crfD, RA, SID);
        return;
    }
    if (Identifier1 == 11) {
        if (L == 1) {
            if (NegativeSID == 1) {
                printf("cmpdi cr%d, r%d, -%d\n", crfD, RA, SID2); }
                else printf("cmpdi cr%d, r%d, %d\n", crfD, RA, SID);
                return;
        
        } else if (NegativeSID == 1) {
            printf("cmpwi cr%d, r%d, -%d\n", crfD, RA, SID2); }
            else printf("cmpwi cr%d, r%d, %d\n", crfD, RA, SID);
            return;
    }
        // li/addi
    if (Identifier1 == 14) {
        if (RA == 0) {
            if (NegativeSID == 1) { printf("li r%d, -%d\n", RT, SID2);
            } else printf("li r%d, %d\n", RT, SID);
           } else if (NegativeSID == 1) { printf("addi r%d, r%d, -%d\n", RT, RA, SID2);
        } else printf("addi r%d, r%d, %d\n", RT, RA, SID);
        return;
        }
        // lis/addis
        if (Identifier1 == 15) {
            if (RA != 0) {
                if (NegativeSID == 1) { printf("addis r%d, r%d, -%d\n", RT, RA, SID2); }
                else printf("addis r%d, r%d, %d\n", RT, RA, SID);
            } else printf("lis r%d, 0x%04X\n", RT, SID);
            return;
        }
        // b(l), b(l)a
    if (Identifier1 == 18) {
        uint32_t Target_Address = 0;
        // Offset
        uint32_t LI = ((Instruction << 6) >> 6) | 0b00;
        // "Absolute" flag
        uint8_t AA = (Instruction << 30) >> 31;
    
        if ((Instruction << 6) >> 31 == 1) { 
            if (AA == 0) {
                Target_Address = Inj_Addr - (0x1000000 - ((LI << 8) >> 8)); }
                 // Todo: determine whether this is a hacky way of reading LI for backwards absolute branch, probably is (?), mimicking Dolphin for now
                else LI = 0xFF000000 + ((LI << 8) >> 8);
        } else Target_Address = Inj_Addr + LI;
        
        // Branch type determination
        if (AA == 0 && LK == 0) { strcpy(InstructionName, "b"); }  
        if (AA == 1 && LK == 0) { strcpy(InstructionName, "ba"); 
        Target_Address = LI - 2; }
        if (AA == 0 && LK == 1) { strcpy(InstructionName, "bl");
        Target_Address = Target_Address - 1; }
        if (AA == 1 && LK == 1) { strcpy(InstructionName, "bla");
        Target_Address = LI - 3; }
        printf("%s 0x%x\n", InstructionName, Target_Address);
        return;
    }

    // blr(l) placeholders, likely more complex
    if (Instruction == 0x4E800020) { printf("blr\n"); return; }
    if (Instruction == 0x4E800021) { printf("blrl\n"); return; }
    // bctr(l) placeholders, likely more complex
    if (Instruction == 0x4E800420) { printf("bctr\n"); return; }
    if (Instruction == 0x4E800421) { printf("bctrl\n"); return; }
    // Classic nop
    if (Instruction == 0x60000000) { printf("nop\n"); return; }

    // ori(s), xori(s)
    if (Identifier1 >= 24 && Identifier1 <= 29) {
        if (Identifier1 == 24) { strcpy(InstructionName, "ori");  }
        if (Identifier1 == 25) { strcpy(InstructionName, "oris");  }
        if (Identifier1 == 26) { strcpy(InstructionName, "xori"); }
        if (Identifier1 == 27) { strcpy(InstructionName, "xoris");  }
        if (Identifier1 == 28) { strcpy(InstructionName, "andi.");  }
        if (Identifier1 == 29) { strcpy(InstructionName, "andis.");  }
        printf("%s r%d, r%d, 0x%X\n", InstructionName, RA, RT, SID);
        return;
    }

    // Obscure but simple "Enforce In-Order Execution of I/O"
    if (Instruction == 0x7C0006AC) { printf("eieio\n"); return; }

    // mfspr, mtspr
    if (Identifier1 == 31 && (Identifier2 == 339 || Identifier2 == 467)) {
    char Direction;
    if (Identifier2 == 339) { Direction = 'f'; } else Direction = 't';
        if (SPR == LR) { printf("m%clr r%d\n", Direction, RT);
        return; }
        if (SPR == CTR) { printf("m%cctr r%d\n", Direction, RT);
        return; }
        if (SPR == XER) { printf("m%cxer r%d\n", Direction, RT);
        return; }
        if (SPR == TBL) { printf("m%ctbl r%d\n", Direction, RT);
        return; }
        if (SPR == TBU) { printf("m%ctbu r%d\n", Direction, RT);
        return; }
        if (SPR == DSISR) { printf("m%cdsisr r%d\n", Direction, RT);
        return; }
        if (SPR == DAR) { printf("m%cdar r%d\n", Direction, RT);
        return; }
        else printf("m%cspr r%d, %d\n", Direction, RT, SPR);
        return;
    }
    
    // Offset-based integer/float load & store instructions
    if (Identifier1 >= 32 && Identifier1 <= 55) {
        if (Identifier1 == 32) { strcpy(InstructionName, "lwz");  }
        if (Identifier1 == 33) { strcpy(InstructionName, "lwzu");  }
        if (Identifier1 == 34) { strcpy(InstructionName, "lbz"); }
        if (Identifier1 == 35) { strcpy(InstructionName, "lbzu"); }
        if (Identifier1 == 36) { strcpy(InstructionName, "stw"); }
        if (Identifier1 == 37) { strcpy(InstructionName, "stwu"); }
        if (Identifier1 == 38) { strcpy(InstructionName, "stb"); }
        if (Identifier1 == 39) { strcpy(InstructionName, "stbu"); }
        if (Identifier1 == 40) { strcpy(InstructionName, "lhz"); }
        if (Identifier1 == 41) { strcpy(InstructionName, "lhzu"); }
        if (Identifier1 == 42) { strcpy(InstructionName, "lha"); }
        if (Identifier1 == 43) { strcpy(InstructionName, "lhau"); }
        if (Identifier1 == 44) { strcpy(InstructionName, "sth"); }
        if (Identifier1 == 45) { strcpy(InstructionName, "sthu"); }
        if (Identifier1 == 46) { strcpy(InstructionName, "lmw"); }
        if (Identifier1 == 47) { strcpy(InstructionName, "stmw"); }
        if (Identifier1 == 48) { strcpy(InstructionName, "lfs"); }
        if (Identifier1 == 49) { strcpy(InstructionName, "lfsu"); }
        if (Identifier1 == 50) { strcpy(InstructionName, "lfd"); }
        if (Identifier1 == 51) { strcpy(InstructionName, "lfdu"); }
        if (Identifier1 == 52) { strcpy(InstructionName, "stfs"); }
        if (Identifier1 == 53) { strcpy(InstructionName, "stfsu"); }
        if (Identifier1 == 54) { strcpy(InstructionName, "stfd"); }
        if (Identifier1 == 55) { strcpy(InstructionName, "stfdu"); }
        
        if (Identifier1 >= 48) { GPR = FPR; }
        if (NegativeSID == 1) { printf("%s %c%d, -0x%X (r%d)\n", InstructionName, GPR, RT, SID2, RA); }
        else printf("%s %c%d, 0x%X (r%d)\n", InstructionName, GPR, RT, SID, RA);
        return;
        }
        // WIP: Floating-Point Arithmetic Instructions
        if (Identifier1 == 59) {
            int Is_Valid = 0;
            uint8_t D = (Instruction << 6) >> 27;
            uint8_t A = (Instruction << 11) >> 27;
            uint8_t B = (Instruction << 16) >> 27;
            uint8_t Bits16_To_20 = (((1 << 5) - 1) & (Instruction >> (21 - 1)));
            uint8_t Bits21_To_25 = (Instruction << 21) >> 27;
            uint8_t Float_Identifier = (Instruction << 26) >> 27;
            if (Bits21_To_25 == 0 && Float_Identifier == 21) {
                strcpy(InstructionName, "fadds");
                Is_Valid = 1;  }
            if (Bits21_To_25 == 0 && Float_Identifier == 18) {
                strcpy(InstructionName, "fdivs");
                Is_Valid = 1;  }
            if (Bits16_To_20 == 0 && Float_Identifier == 25) {
                strcpy(InstructionName, "fmuls");
                Is_Valid = 1;  }
            if (Bits21_To_25 == 0 && Float_Identifier == 20) {
                strcpy(InstructionName, "fsubs");
                Is_Valid = 1;  }
                 
            if (Is_Valid == 1) {
                // Use LK as Rc here
                if (LK == 1) { printf("%s. f%d, f%d, f%d\n", InstructionName, D, A, B); }
                else printf("%s f%d, f%d, f%d\n", InstructionName, D, A, B);
                return; }
       }
    
    printf("(unk)\n");
    return;
}

int main(int argc, char *argv[])
{
    /* v0.1: Working single-line injection of a Text1 code
       v0.2: Working multi-line injection of Text1 codes
       v0.3: Can now decompile 12 PowerPC instructions
       v0.4: Can now run in simulation mode
       v0.5: Can now decompile 38 PowerPC instructions
       v0.6: Can now inject into any text section
       v0.7: PPC decomp rewrite with support for about 60 instructions/mnemonics; can now inject into any data section */

    printf("\n");
    printf("Lily Injector v0.7 Beta - WIP GameCube AR Code Tool & Gekko Disassembler\n");
    FILE *dolfile;
    FILE *codefile;
    char dolname[30];
    printf("Specify DOL for patching: ");
    scanf("%29s", dolname);

    codefile = fopen("codes.txt", "r");
    dolfile = fopen(dolname, "rb+");

    if (dolfile == NULL) {
        printf("Specified DOL was not found! Aborting.");
        return 0;
      } else printf("Opened %s; ", dolname);

    if (codefile == NULL) {
        printf("codes.txt not found! Aborting.\n");
        return 0;
    } else printf("Opened codes.txt\n");

    printf("\n");
    struct Dol_File Dol;
    fread(&Dol, sizeof(Dol), 1, dolfile);

    // AR code halves are usually 8 characters
    char Code_Half1[9];
    int Processed_Lines = 0, 
    Simulation_Mode = 0;
    int Instruction_Shift = 19;
    
    if (argc >= 2) {
        // If -simulate arg has been passed
        if (strcmp(argv[9], "-simulate") == 0) {
        printf("Running in simulation mode (no injection)\n");
        Simulation_Mode = 1;
        } else printf("Unknown argument\n");
    }
    // Begin line reading procedure, read the injection address
    while (fgets(Code_Half1, 9, codefile) != NULL) {

    // Perform conversion from string in txt to injection address
    uint32_t Inj_Addr = (uint32_t)strtoul(Code_Half1, NULL, 16);
    // Fetch the instruction string and convert it, or stop if reached hard end
    fseek(codefile, (9 + (Instruction_Shift * Processed_Lines)), SEEK_SET);
    if (fgets(Code_Half1, 9, codefile) == NULL) {
        printf("Cannot read instruction/value from line %d. Halting job\n", Processed_Lines + 1);
        break;
    }
    uint32_t Instruction = (uint32_t)strtoul(Code_Half1, NULL, 16);

    // Shift 3 bytes to isolate the code type
    uint8_t Code_Type = Inj_Addr >> 24;
    // Make the injection address a proper memory location
    Inj_Addr = ((Inj_Addr << 8) >> 8) + 0x80000000;
    printf("L%02d: ", Processed_Lines +1);

    // Begin scanning of text sections
    int Analyzed_Sections = 0, 
        Use_Data = 0;
    // The limit for text secs is 7, but code below is handling switch to data sect analysis
    while (Analyzed_Sections < 11) {
    uint32_t CurSect_Virtual, CurSect_MaxBounds = 0; 
    
    if (Use_Data == 0) {
    CurSect_Virtual = Fix_Big_Endian(Dol.Text_Virtual[Analyzed_Sections]);
    CurSect_MaxBounds = CurSect_Virtual + Fix_Big_Endian(Dol.Text_Lengths[Analyzed_Sections]);
    } else {
    CurSect_Virtual = Fix_Big_Endian(Dol.Data_Virtual[Analyzed_Sections]);
    CurSect_MaxBounds = CurSect_Virtual + Fix_Big_Endian(Dol.Data_Lengths[Analyzed_Sections]);
    }
    // Scenarios where it's considered not injectable: after scanning all text sects, when a null data sect is reached or no fitting data sects are found
    if ((CurSect_Virtual == 0 && Use_Data == 1) || (Analyzed_Sections == 10 && Use_Data == 1) ) {
        printf("Not injectable; outside bounds of DOL\n");
        break;
    }
    
    if (Inj_Addr >= CurSect_Virtual && Inj_Addr <= CurSect_MaxBounds) {
            // Fetch DOL address
            uint32_t Inj_Addr_Physical = 0;
            
            // Switch between using data/text sects' physical address
            if (Use_Data == 0) {
            Inj_Addr_Physical = (Inj_Addr - CurSect_Virtual) + Fix_Big_Endian(Dol.Text_Physical[Analyzed_Sections]);
            } else Inj_Addr_Physical = (Inj_Addr - CurSect_Virtual) + Fix_Big_Endian(Dol.Data_Physical[Analyzed_Sections]);
            printf("Writing %X at DOL 0x%X (%X) from code type %d: ", Instruction, Inj_Addr_Physical, Inj_Addr, Code_Type);
            
            // Now disassemble and print instruction
            DisASM(Instruction, Inj_Addr);
            // The message above will display fixed instruction properly but need to "unfix" for injecting:
            Instruction = Fix_Big_Endian(Instruction);
            // Write instruction
            fseek(dolfile, Inj_Addr_Physical, SEEK_SET);
            if (Simulation_Mode == 0) { fwrite(&Instruction, sizeof(Instruction), 1, dolfile); }
            break;
    }
    
    if (Analyzed_Sections++ == 7 && Use_Data == 0) {
        Use_Data = 1;
        Analyzed_Sections = 0;
    }
}
        Processed_Lines++;
        fseek(codefile, (19 * Processed_Lines), SEEK_SET);
}
    printf("Job complete!\n");
    fclose(dolfile);
    fclose(codefile);
    return 0;
}

