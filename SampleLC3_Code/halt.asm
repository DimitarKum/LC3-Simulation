.ORIG x0300
      LEA R0 MSSG
      TRAP x48
      
      LDI   R1, MCR
      LD    R0, MASK
      AND   R0, R1, R0
      STI   R0, MCR
 
MSSG  .STRINGZ "----- Halting the processor -----\n"
MCR   .FILL xFFFE
MASK  .FILL x7FFF
.END