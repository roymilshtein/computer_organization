# computer_organization
In this project we will develop a simulator for a RISC type processor named SIMP.

The schematic below illustrates the project:

![Project Diagram](projectdiagram.png)

The parts of the project which are illustrated in red are coded and prepared. The output files
are shown in green. These output files will be generated automatically by the software.

The project consists of 3 parts:
1. writing assembly code to be simulated (fib.asm)
2. writing the assembler that converts it to machine code
3. writing te simulator that executes the machine code

The assembly code is written for the SIMP processor. For more information, read project "Project Documentation.pdf".

The assembler is written in the C programming language and translates the assembler code written in text in assembler format to machine code.
Just like the simulator the assembler is executed using the command line as shown below:
asm.exe program.asm memin.txt
The input file program.asm contains the assembly program, the output file file memin.txt contains the memory image and is input to the simulator.

The simulator simulates the fetch-decode-execute loop. The run concludes when the instruction HALT is executed.
The simulator is written in the C programming language and will be run from a command line application that receives 5 command line parameters as written in the following execution line:
sim.exe memin.txt memout.txt regout.txt trace.txt cycles.txt



