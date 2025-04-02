; vary operands number and case insensitive
.entry HELLO
.entry WORLD
.entry Hello
.extern B

HELLO: add r7, B
WORLD: bne &XYZ
Hello: stop
XYZ: .data 7