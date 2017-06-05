.ORIG x3000

      JSR PRINT
      JSR PRINT
      TRAP x50


PRINT ST R7, SAV7
      LEA R0, HI
      TRAP x48
      LEA R0, NL
      TRAP x48
      LD  R0, U
      TRAP x40
      LD  R0, W
      TRAP x40
      LD  R0, T
      TRAP x40
      LEA R0, NL
      TRAP x48
      LD R7, SAV7
      RET


HI   .STRINGZ "HELLO"
NL   .STRINGZ "\n"
U    .FILL  x55
W    .FILL  x57
T    .FILL  x54
SAV7 .FILL  0
.END 