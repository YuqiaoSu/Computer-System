.data

print_line1: .asciiz "Initial array is:\n"

print_line2: .asciiz "Quick sort is finished!\n"

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
la $t0,dataAddr
addi $a0,$t0,0
addi $a1,$zero,0
add $a2,$zero,$t6
jal quicksort

li $v0, 4
la $a0, print_line2  # printf("Quick sort is finished!\n");
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


str_lt:

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

quicksort:	
	addi $sp, $sp, -100		
	sw $a0, 0($sp)			# dataAddr
	sw $a1, 4($sp)			# first
	sw $a2, 8($sp)			# last
	sw $ra, 12($sp)			# return address
	
	bge $a1,$a2,finish
	add $t0, $a1,$a2
	addi $t1, $zero,2
	div $t0,$t0,$t1 #first + last) /2
	sll $t0,$t0,2
	lw $t1,dataAddr($t0)  #x =a[(first + last) /2];
	
	add $s5, $zero,$a1   #i = first;
	add $s6, $zero,$a2 # j = last;
while_i:
	sll $t4,$s5,2 #i*4
	lw $t5,dataAddr($t4) #a[i]
	move $s1,$t5  #str_lt(a[i], x)
	move $s2,$t1
	jal str_lt
	beqz $v0, while_j
	addi $s5,$s5,1  #i++;
	j while_i
while_j:
	sll $t4,$s6,2 #j*4
	lw $t5,dataAddr($t4) #a[j]
	move $s1,$t1 # str_lt(x, a[j])
	move $s2,$t5
	jal str_lt
	beqz $v0, after_while
	addi $s6,$s6,-1  #j--;
	j while_j	
after_while:
	
	bge $s5,$s6,OuterLoop # if (i >= j) break;
	sll $t4,$s5,2
	sll $t5,$s6,2
	lw $t6, dataAddr($t4) #t = a[i];
	lw $t7,dataAddr($t5)  #a[j];

	sw $t7,dataAddr($t4)  #a[i] = a[j];
	sw $t6,dataAddr($t5)  #a[j] = t;
	addi $s5,$s5,1  #i++; 
	addi $s6,$s6,-1 #j--;
	j while_i

OuterLoop:
 	lw $a1, 4($sp)			
 	lw $a2, 8($sp)	
	add $t0,$zero,$a1 #first
	add $t1,$zero,$a2  #last
	addi $s5,$s5,-1  #i-1
	addi $s6,$s6,1  #j+1
	
	sw $s5,16($sp)
	sw $s6,20($sp)
	
	blt $t0,$s5,first_half # if(first < i-1) 
	blt $s6,$t1,second_half # if(j+1 < last) 
	bge $t0,$s5,finish
	bge $s6,$t1,finish
first_half:  #quicksort( a, first, i-1 );
	add $a2,$zero,$s5
	jal quicksort	

second_half: #quicksort( a, j+1, last );
	lw $s6,20($sp) #j
	lw $a2, 8($sp) #last
	lw $a1, 4($sp)	#first
	add $t1,$zero,$a2  
	bge $s6,$t1,finish
	add $a1,$zero,$s6
	jal quicksort		
	
finish:	
	lw $a0, 0($sp)		
 	lw $a1, 4($sp)			
 	lw $a2, 8($sp)			
 	lw $ra, 12($sp)	
 	lw $s5, 16($sp)
	lw $s6,20($sp)	
 	addi $sp, $sp, 100		
 	jr $ra	
	

	
