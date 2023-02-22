# Lily Injector
WIP GameCube Action Replay Code Injector / Gekko Disassembler

Lily Injector is a command-line based tool designed for easy injection of AR codes into a DOL GameCube executable. It provides information on the target location of given codes as well as disassembly of instructions applied by them. To use it, open a command prompt in its directory, and make sure your DOL to be modified + the codes.txt file containing your AR codes, without newlines/spaces, are inside as well.

You can use the -s argument to preview the operations performed by the program without modifying the DOL file, like as such:
C:\Users\Windows\Documents\Tools\Lily_Injector.exe -s

The disassembler support roughly 150+ Gekko instructions/mnemonics (WIP). Not all instructions are recognized yet, but for the most part, a good portion of ASM is readable.

Currently Supported types: 04 (32-bit write), 02 (16-bit write), 01 (byte write)

DISCLAIMER: BACK UP YOUR DOL FILE BEFORE OPERATION (OR USE -s). THIS IS A WORK-IN-PROGRESS PROJECT. Report any bugs/feature requests ASAP in this repo's issue tracker or to LMFinish#7842 over at Discord

# Errors
"Not injectable; outside bounds of DOL":
The AR code line has been detected to not be within any of the text/data sections listed by the DOL's header, making it impossible to apply the edit. This could be due to the targeted location being a volatile area storing non-static data, or data from another file within RAM. If the address is certainly within range, check for any new lines or accidental spaces in code.txt and remove them.

"Unaligned %d-bit store attempt, skipping"
Make sure your 04-type code is aligned to 4, and your 02-type code is aligned to 2. 8-bit stores don't need alignment.

"Out of unsigned %d-bit integer range, skipping"
If your code is a 02/01 type, make sure the value applied by it doesn't exceed 0xFFFF/0xFF respectively.

"Cannot read instruction/value from line %d. Halting job":
Make sure all lines point to and target correct addresses/values, are correctly aligned and don't contain any extra spaces.

# To-Do
- A GUI version
- Implement other code types as documented in WiiRD
- Add more Gekko instructions
- Code cleanup (might be messy/hacky)


