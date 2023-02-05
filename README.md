# Lily Injector
WIP GameCube AR Code Injector / Gekko Disassembler

Lily Injector is a command-line based tool designed for easy injection of AR codes into a DOL GameCube executable. It provides information on the target location of given codes as well as disassembly of instructions applied by them. To use it, open a command prompt in its directory, and make sure your DOL to be modified + the codes.txt file containing your AR codes, without newlines/spaces, are inside as well.

You can use the -simulate argument to preview the operations performed by the program without modifying the DOL file, like as such:
C:\Users\Windows\Documents\Tools\Lily_Injector.exe -simulate

The disassembler currently supports about 60 Gekko instructions/mnemonics (WIP). Not all instructions are recognized yet.

Currently Supported types: 04/14

Report any bugs/feature requests in this repo's issue tracker or to LMFinish#7842 over at Discord

# Errors
"Not injectable; outside bounds of DOL":

The AR code line has been detected to not be within any of the text/data sections listed by the DOL's header, making it impossible to apply the edit. This could be due to the targeted location being a volatile area storing non-static data, or data from another file within RAM. If the address is certainly within range, check for any new lines or accidental spaces in code.txt and remove them.

"Cannot read instruction/value from line %d. Halting job":

Make sure all lines point to and target correct addresses/values, are correctly aligned and don't contain any extra spaces.


