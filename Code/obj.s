.data
_prompt: .asciiz "Enter an integer:"
_ret: .asciiz "\n"
.globl main
.text

read:
subu $sp, $sp, 4
sw $ra, 0($sp)
subu $sp, $sp, 4
sw $fp, 0($sp)
addi $fp, $sp, 8
li $v0, 4
la $a0, _prompt
syscall
li $v0, 5
syscall
subu $sp, $fp, 8
lw $fp, 0($sp)
addi $sp, $sp, 4
lw $ra, 0($sp)
addi $sp, $sp, 4
jr $ra

write:
subu $sp, $sp, 4
sw $ra, 0($sp)
subu $sp, $sp, 4
sw $fp, 0($sp)
addi $fp, $sp, 8
li $v0, 1
syscall
li $v0, 4
la $a0, _ret
syscall
move $v0, $0
subu $sp, $fp, 8
lw $fp, 0($sp)
addi $sp, $sp, 4
lw $ra, 0($sp)
addi $sp, $sp, 4
jr $ra


f:
subu $sp, $sp, 4
sw $ra, 0($sp)
subu $sp, $sp, 4
sw $fp, 0($sp)
addi $fp, $sp, 8
subu $sp,$sp,36
move $t0, $a0
move $t1 ,$t0
subu $sp, $fp, 8
lw $fp, 0($sp)
addi $sp, $sp, 4
lw $ra, 0($sp)
addi $sp, $sp, 4
move $v0,$t1
jr $ra

main:
subu $sp, $sp, 4
sw $ra, 0($sp)
subu $sp, $sp, 4
sw $fp, 0($sp)
addi $fp, $sp, 8
subu $sp,$sp,120
jal read
move $t0, $v0
move $a0, $t0
jal write
li $t1 ,3
subu  $v1 ,$fp , 12
sw $t0, 0($v1)
subu  $v1 ,$fp , 16
sw $t1, 0($v1)
move $a0, $t1
jal f
addi $sp, $sp, 0
move $t0, $v0
move $a0, $t0
jal write
li $t1 ,16
move $a0, $t1
jal write
li $t2 ,16
subu  $v1 ,$fp , 20
sw $t0, 0($v1)
subu  $v1 ,$fp , 24
sw $t1, 0($v1)
subu  $v1 ,$fp , 28
sw $t2, 0($v1)
move $a0, $t2
jal f
addi $sp, $sp, 0
move $t0, $v0
move $a0, $t0
jal write
li $t1 ,0
subu $sp, $fp, 8
lw $fp, 0($sp)
addi $sp, $sp, 4
lw $ra, 0($sp)
addi $sp, $sp, 4
move $v0,$t1
jr $ra
