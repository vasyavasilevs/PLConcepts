FBEGIN 1
MOV r1 *sp
ADDI r1 1
MOV r2 **r1
MOV r3 *r2
SUBI r3 1
JUMP_IF_GZ *r3 5
MOV rv *r2
MOV r1 **sp
POP
JUMP *r1
PUSH *r3
CALL 1
MOV r3 **sp
POP
SUBI r3 1
PUSH *rv
PUSH *r3
CALL 1
POP
ADD rv **sp
POP
MOV r1 **sp
POP
JUMP *r1
FEND
PRINTSTR Hello! It is a calculator of n-th member of the Fib-sequence [0, 1, 1, 2, 3, 5, 8, 13... ]
PRINTSTR Enter n:
READ r1
PUSH *r1
CALL 1
POP
PRINTSTR Your answer is:
PRINT *rv
TERM
