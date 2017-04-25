COPY   START  0
FIRST  STL    RETADR
       LDB    #LENGTH
       BASE   LENGTH
CLOOP  +JSUB  RDREC
       LDA    LENGTH
       COMP   #0
       JEQ    ENDFIL
       +JSUB  WRREC
       J      CLOOP
ENDFIL LDA    EOF
       STA    BUFFER
       LDA    #3
       STA    LENGTH
       +JSUB  WRREC
       J      @RETADR
EOF    BYTE   C'EOF'
RETADR RESW   1
LENGTH RESW   1
BUFFER RESB   4096
.
.      SUBROUTINE TO READ RECORD INTO BUFFER
.
RDREC  CLEAR  X
       CLEAR  A
       CLEAR  S
       +LDT   #4096
RLOOP  TD     INPUT
       JEQ    RLOOP
       RD     INPUT
       COMPR  A, S
       JEQ    EXIT
       STCH   BUFFER, X
       TIXR   T
       JLT    RLOOP
EXIT   STX    LENGTH
       RSUB
INPUT  BYTE   X'F1'
.
.      SUBROUTINE TO WRITE RECORD FROM BUFFER
.
WRREC  CLEAR  X
       LDT    LENGTH
WLOOP  TD     OUTPUT
       JEQ    WLOOP
       LDCH   BUFFER, X
       WD     OUTPUT
       TIXR   T
       JLT    WLOOP
       RSUB
OUTPUT BYTE   X'05'
       END    FIRST

