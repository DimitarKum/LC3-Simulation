.ORIG x0250
         ST   R7, SAV7
         ST   R0, SAV0
         ST   R1, SAV1
         ST   R2, SAV2
CHARS    LDR  R1, R0, #0
         BRZ  DONE  
LOOPDSR  LDI  R2, DSR 
         BRZP LOOPDSR 
         STI  R1, DDR
         ADD  R0, R0, #1
         BR   CHARS
DONE     LD   R0, SAV0
         LD   R1, SAV1
         LD   R2, SAV2
         LD   R7, SAV7
         RET
DSR      .FILL xFE04
DDR
      .FILL xFE06
SAV7     .FILL 0
SAV0     .FILL 0
SAV1     .FILL 0
SAV2     .FILL 0
.END
