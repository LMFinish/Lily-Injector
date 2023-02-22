#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include "DolFile.h"
#include <stdio.h>
#include <stdlib.h>

uint32_t Fix_Big_Endian(uint32_t VTF)
{
    // Todo: don't use this function on possible big-endian targets
    uint32_t Fixed_Val = ((((VTF & 0xFF000000) >> 24) + ((VTF & 0x00FF0000) >> 8)) + ((VTF & 0xFF00) << 8)) + ((VTF << 24));
    return Fixed_Val;
}

int main(int argc, char *argv[])
{
    /* v0.1: Working single-line injection of a Text1 code
       v0.2: Working multi-line injection of Text1 codes
       v0.3: Can now decompile 12 PowerPC instructions
       v0.4: Can now run in simulation mode
       v0.5: Can now decompile 38 PowerPC instructions
       v0.6: Can now inject into any text section
       v0.7: PPC decomp rewrite with support for about 60 instructions/mnemonics; can now inject into any data section
       v1.0: PPC decomp now supports well over 150 instructions, code types 1/2 now supported */

    printf("\n");
    printf("Lily Injector v1.0 - WIP GameCube AR Code Tool & Gekko Disassembler by LMFinish\n");
    FILE *dolfile;
    FILE *codefile;
    char dolname[30];
    printf("Specify DOL for patching: ");
    scanf("%29s", dolname);

    codefile = fopen("codes.txt", "r");
    dolfile = fopen(dolname, "rb+");

    if (dolfile == NULL) {
        printf("Specified DOL was not found! Aborting.\n");
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
    int tool_arg;

    // Scan any arguments
    while ((tool_arg = getopt(argc, argv, "s")) != -1) {
        switch(tool_arg) {
            case 's':
            printf("Running in simulation mode (no injection)\n");
            Simulation_Mode = 1;
            break;
        }
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
    printf("L%03d: ", Processed_Lines +1);

    // Set these flags up for sect scanning
    int Analyzed_Sections = 0,
        Use_Data = 0;

    // Begin scan; the limit for text secs is 7, but code within this loop is handling switch to data sect scan mode
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

            uint32_t Spot = (Inj_Addr_Physical << 28) >> 28;

            if (Code_Type == 4 && (Spot << 30) >> 30 >= 1) {
            printf("Unaligned 32-bit store attempt, skipping.\n");
            break;
            }

            if ((Code_Type == 1 && Instruction > 0xFF) || (Code_Type == 2 && Instruction > 0xFFFF)) {
            printf("Out of unsigned %d-bit integer range, skipping.\n", Code_Type == 1? 8 : 16);
            break;
            }
            if (Code_Type == 2) {
                struct Dol_File Access;

                // The reading bloat in this mightn't be likable, but it'll do for now
                if (Spot == 0 || Spot == 4 || Spot == 8 || Spot == 0xC) {
                fseek(dolfile, Inj_Addr_Physical, SEEK_SET);
                fread(&Access, sizeof(Access), 1, dolfile);
                Instruction = (Fix_Big_Endian(Access.Text_Physical[0]) << 16) >> 16 | Instruction << 16;

                } else if (Spot == 2 || Spot == 6 || Spot == 0xA || Spot == 0xE) {
                Inj_Addr_Physical = Inj_Addr_Physical - 2;
                fseek(dolfile, Inj_Addr_Physical, SEEK_SET);
                fread(&Access, sizeof(Access), 1, dolfile);
                Instruction = (Fix_Big_Endian(Access.Text_Physical[0]) >> 16) << 16 | Instruction;
                } else { printf("Unaligned 16-bit store attempt, skipping.\n");
                break;
                }
            }

            printf("Writing %08X, code type %d at DOL 0x%06X | %X", Instruction, Code_Type, Inj_Addr_Physical, Inj_Addr);
            if (Code_Type == 4) { printf(": ");
            DisASM(Instruction, Inj_Addr);
            } else printf("\n");
            // Now disassemble and print instruction

            if (Simulation_Mode == 0) {

            // The message above will display fixed instruction properly but need to "unfix" for injecting, unless if code type 1
            if (Code_Type != 1) {
            Instruction = Fix_Big_Endian(Instruction);
            }
            fseek(dolfile, Inj_Addr_Physical, SEEK_SET);
            fwrite(&Instruction, Code_Type == 1? 1 : sizeof(Instruction), 1, dolfile);

            if(ferror(dolfile)) {
                printf("Cannot apply changes to DOL file. Aborting\n");
                fclose(dolfile);
                fclose(codefile);
                return 0;
            }
        }   break;
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
