; sample assembly file test 2
MAIN:   mov r3, LENGTH
        add r2, r3
        sub #5, r1
        lea STR, r4
        prn r4
LOOP:   cmp #0, r2
        bne &LOOP
        stop

LENGTH: .data 6
STR:    .string "hello"