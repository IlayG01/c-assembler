; invalid extern and enrty directives in the same file
.entry HELLO
.entry 505
.entry mov

HELLO: add #1, r1
extern. HELLO
; invalid naming
505: add #1, r1
mov: add #1, r1