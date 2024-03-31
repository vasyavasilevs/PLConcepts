FBEGIN 1
MOV 3 *1
ADD 3 1
MOV 4 **3
MOV 5 *4
SUB 5 1
JUMP_IF_GZ *5 5
MOV 2 *4
MOV 3 **1
POP
JUMP *3
PUSH *5
CALL 1
MOV 5 **1
POP
SUB 5 1
PUSH *2
PUSH *5
CALL 1
POP
ADD 2 **1
POP
MOV 3 **1
POP
JUMP *3
FEND
PRINTSTR Hello! This programm will calclulate the n-th member of the fib-sequence [0, 1, 1, 2, 3, 5, 8, ... ]
PRINTSTR Enter number n:
READ 3
PUSH *3
CALL 1
POP
PRINTSTR Your answer is:
PRINT *2
TERM
