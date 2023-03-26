# Lily Injector
WIP GameCube Action Replay Code Injector / Gekko Disassembler

![Lily Injector Preview](https://i.ibb.co/bz2Mh73/Lily-v1-1.png)

Lily Injector is a command-line based tool designed for easy injection of AR codes into a DOL GameCube executable. It provides information on the target location of given codes as well as disassembly of instructions applied by them. To use it, open a command prompt in its directory, and make sure your DOL to be modified + the codes.txt file containing your AR codes, without newlines/spaces, are inside as well.

- To **inject** codes, run injection.bat or open the tool in a command prompt of your choice.
- To **preview** injection of codes, run preview_inject.bat or use the -s parameter in a command prompt & make sure codes.txt contains Action Replay code lines.
- To **disassemble** PowerPC Gekko bytecode, run disasm.bat or use the -d parameter in a command prompt & make sure codes.txt contains only PowerPC Gekko bytecode.
When in disassembly mode, specify the entry point.

The disassembler supports roughly 160+ Gekko instructions/mnemonics (WIP). Not all instructions are recognized yet, but for the most part, a good portion of ASM is readable, including an array of Paired Singles (GC/Wii exclusive) instructions.

Currently Supported types: 04 (32-bit write), 02 (16-bit write), 01 (byte write)

Tested extensively, stable injection of codes 100% of the time. However, do report any bugs/feature requests ASAP in this repo's issue tracker or to LMFinish#7842 over at Discord

# Errors
"Not injectable; outside bounds of DOL":
The AR code line has been detected to not be within any of the text/data sections listed by the DOL's header, making it impossible to apply the edit. This could be due to the targeted location being a volatile area storing non-static data, or data from another file within RAM. If the address is certainly within range, check for any new lines or accidental spaces in code.txt and remove them.

"Unaligned %d-bit store attempt, skipping"
Make sure your 04-type code is aligned to 4, and your 02-type code is aligned to 2. 8-bit stores don't need alignment.

"Out of unsigned %d-bit integer range, skipping"
If your code is a 02/01 type, make sure the value applied by it doesn't exceed 0xFFFF/0xFF respectively.

"Cannot read instruction/value from line %d. Halting job":
Make sure all lines point to and target correct addresses/values, are correctly aligned and don't contain any extra spaces.

"Possible whitespace, junk data, or non-bytecode:"
Make sure that codes.txt contains PowerPC Gekko bytecode only, without spaces or other invalid characters.

# To-Do
- A GUI version
- Implement other code types as documented in WiiRD
- Add more Gekko instructions
- Code cleanup (might be messy/hacky)


