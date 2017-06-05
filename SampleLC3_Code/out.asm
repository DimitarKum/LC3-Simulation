.ORIG x0200
        ST R7, SAV7
        ST R1, SAV1
LOOPDSR LDI R1, DSR
        BRZP LOOPDSR 
        STI R0, DDR
        LD  R1, SAV1
        LD  R7, SAV7
        RET
DSR    .FILL  xFE04
DDR    .FILL  xFE06
SAV7   .FILL  0
SAV1   .FILL  0
.END
