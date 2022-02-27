.data

print_line1: .asciiz "Initial array is:\n"

print_line2: .asciiz "Insertion sort is finished!\n"

leftBracket: .asciiz "["

blank:.asciiz  " "

rightBracket: .asciiz " ]\n"

dataName: .align 5  # initialize char * data[]
     .asciiz "Joe"
     .align 5
     .asciiz "Jenny"
     .align 5
     .asciiz "Jill"
     .align 5
     .asciiz "John"
     .align 5
     .asciiz "Jeff"
     .align 5
     .asciiz "Joyce"
     .align 5
     .asciiz "Jerry"
     .align 5
     .asciiz "Janice"
     .align 5
     .asciiz "Jake"
     .align 5
     .asciiz "Jonna"
     .align 5
     .asciiz "Jack"
     .align 5
     .asciiz "Jocelyn"
     .align 5
     .asciiz "Jessie"
     .align 5
     .asciiz "Jess"
     .align 5
     .asciiz "Janet"
     .align 5
     .asciiz "Jane"
          
          .align 2  # addresses should start on a word boundary
dataAddr: .space 64 # 16 pointers to strings: 16*4 = 64
iterator: .word 0
size: .word 16
.text

lw $t6, size   # int size = 16;
addi $t6,$t6,-1
li $v0, 4
la $a0, print_line1  #printf("Initial array is:\n");
syscall   

#  char * data[] = {"Joe", "Jenny", "Jill", "John", "Jeff", "Joyce",
#  "Jerry", "Janice", "Jake", "Jonna", "Jack", "Jocelyn",
#  "Jessie", "Jess", "Janet", "Jane"};

addi $t0, $zero,0

store_ptr:
sll $t2, $t0, 5 # 2^5 = 32
sll $t3,$t0, 2  #point to dataPtr
la $t4, dataName($t2) #get pointer of name
sw $t4, dataAddr($t3)  #save that pointer tp dataPtr
slt $t1, $t0, $t6
addi $t0,$t0,1
bgtz $t1, store_ptr


jal print_array #  print_array(data, size);
# insertSort(data, size);
jal InsertSort

li $v0, 4
la $a0, print_line2  # printf("Insertion sort is finished!\n");
syscall 

jal print_array #  print_array(data, size);

li $v0, 10  #exit(0)
la $a0, 0
syscall  


print_array:
addi $t2,$zero, 0
lw $t0, size   # int size = 16;
addi $t0,$t0,-1

li $v0, 4
la $a0, leftBracket # printf("[");
syscall

addi $sp, $sp, -4
sw $ra, 0($sp)

print_loop:
la $a0 blank
syscall   #printf("  %s"

slt $t1,$t2,$t0
sll $t3, $t2, 2
lw $t4, dataAddr($t3)
move $a0,$t4
syscall  #printf("  %s", a[i])

addi $t2, $t2, 1 # a[i++]
bgtz $t1, print_loop  #while(i < size)

la $a0, rightBracket #   printf(" ]\n");
syscall  

lw $ra, 0($sp)
addi $sp, $sp, 4 
jr $ra



InsertSort:
lw $t0,size
addi $t0,$t0,-1 #size - 1

addi $sp, $sp, -4
sw $ra, 0($sp)
# t0 = i, t1 = j
addi $t2, $zero, 1  # i = 1
OuterLoop:
# for(i = 1; i < length; i++)

#char *value = a[i];
sll $t4, $t2, 2  #i = i*4 
lw $t5, dataAddr($t4)# value = a[i]

addi $t1,$t2,-1  #j = i-1
InnerLoop:
blt $t1,0,GoOuter  # j< 0

sll $t3, $t1, 2  #a[j+1]
lw $t6,dataAddr($t3)  #a[j+1] = value

#str_lt(value, a[j])
move $a0,$t5  #pass address of value
move $a1,$t6  #pass address of a[j]
jal str_lt  
beq $v0,0, GoOuter

addi $t7, $t3,4  #[j+1]
sw  $t6, dataAddr($t7) #a[j+1] = a[j]

addi $t1, $t1, -1 #j--
jal InnerLoop

GoOuter:
addi $t1,$t1,1 # j + 1
sll $t7,$t1,2  #[j+1]

sw $t5,dataAddr($t7)  #a[j+1] = value
addi $t2,$t2,1  #i++
ble $t2, $t0, OuterLoop #i < length;

lw $ra, 0($sp)
addi $sp, $sp, 4
jr $ra

#int str_lt (char *x, char *y)

str_lt:
move  $s1,$a0
move $s2,$a1
addi $sp, $sp, -4
sw $ra, 0($sp)

#for (; *x!='\0' && *y!='\0'; x++, y++)
str_lt_for:

lb $s3,0($s1)  #x
lb $s4,0($s2)  #y

blt $s3, $s4, return_1 #if ( *x < *y ) return 1;
blt $s4, $s3, return_0 #if ( *y < *x ) return 0;

addi $s1,$s1,1  #x++
addi $s2,$s2,1  # y++
bnez $s3, str_lt_for #*x!='\0'
bnez $s4, str_lt_for #*y!='\0


beqz $s4, return_0 #if ( *y == '\0' ) return 0;
bnez $s4, return_1 #else return 1;

return_1: #return 1
addi $v0,$zero,1
lw $ra, 0($sp) 
addi $sp, $sp, 4
jr $ra

return_0:  #return 0
add $v0,$zero,$zero
lw $ra, 0($sp)
addi $sp, $sp, 4
jr $ra
