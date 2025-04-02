; 2 commas are invalid
X: .data 7, −57,, +17, 9
add #1,, r1
; comma after instruction is invalid
Y: .data 7, −57, +17, 9,
add #1, r1,
; comma before first number is invalid
Z: .data ,7, −57, +17, 9
add #,-100, r1
