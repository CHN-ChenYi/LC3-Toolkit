.ORIG x0800 ; interrupt service routine
        ST  R0, R0_SAVE
        ST  R1, R1_SAVE
        GETC
        LD  R1, NLF
        ADD R1, R0, R1
        BRnp    NOT_LF
        LD  R1  NZERO ; is LF
        ADD R1, R1, R5
        BRz END_OF_INT ; R5 == '0'
        ADD R5, R5, #-1

        BRnzp   END_OF_INT
NOT_LF  LD  R1, NZERO
        ADD R1, R0, R1
        BRn NOT_NUM ; < 0
        LD  R1, NNINE
        ADD R1, R0, R1
        BRp NOT_NUM ; > 9
        ADD R5, R0, #0
        BRnzp   END_OF_INT
NOT_NUM        AND R4, R4, R4
        BRz INT_LOOP_INIT ; end of a line
        ADD R1, R0, #0
        LD  R0, INT_LF
        OUT
        ADD R0, R1, #0
INT_LOOP_INIT        LD  R1, INT_MAX_COUNT
INT_LOOP        OUT
        ADD R1, R1, #-1
        BRp INT_LOOP
        LD  R0, INT_LF
        OUT

END_OF_INT        LD  R1, R1_SAVE
        LD  R0, R0_SAVE
        RTI

TEST            .FILL   x0030
R0_SAVE         .BLKW   #1
R1_SAVE         .BLKW   #1
NLF             .FILL   xFFF6
NZERO           .FILL   xFFD0
NNINE           .FILL   xFFC7
INT_LF          .FILL   x000A
INT_MAX_COUNT   .FILL   x0028
.END
