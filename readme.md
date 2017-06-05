A C based simulation of the LC3 machine.
LC3 is a simple theoretical(and simulated) computer machine created by Yale N. Patt at the University of Texas at Austin and Sanjay J. Patel at the University of Illinois at Urbana–Champaign. LC3 has its own assembly language. This project includes only a simulator for running LC3 machine-code(no assembler provided).

Instructions currently implemented: ADD, AND, LD, LDI, LDR, BR, ST, STI, STR, LEA, JSR, RET, TRA.
Instructions not implemented: JMP, JSRR and RTI.

DDR(Display Data Register), DSR(Display Status Register) and MCR(Machine Control Register) are memory mapped and implemented.
KBDR(Keyboard Data Register) and KBSR(Keyboard Status Register) are currently not impemented. (no input possible, only output)

For usage pass the name of an .obj file as a command line argument.
The easiest way to run the Sample LC3 program provided is to compile and move the executable to ./SampleLC3 and run with:
"lc3.exe trapvectortable.obj out.obj puts.obj halt.obj trapcalls.obj"
The trapvectortable, out, puts and halt are in a sense part of the operating system. The trapcalls is the actual program that's loaded and ran.

For debugging compile with #define PRINT_ON (1) for extra messages.

Disclaimer: This code was developed using starting code as part of the curriculum of University of Washington Tacoma TCSS 371 Machine Organization as taught by Mayer John, Ph. D.