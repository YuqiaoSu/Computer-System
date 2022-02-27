# Compute the given number's factorial
      .data
head1:.asciiz  "Positive Integer: "          
head2: .asciiz "The value of factorial("
head3: .asciiz ") is "
	.text
main:   la $a0, head1
	 li   $v0, 4           # printf("Positive integer: ");
        syscall               
        
        li $v0, 5  #scanf("%d", &number)
        syscall
        move   $t0,$v0    # store input in $t0

  	add    $a0, $zero,$t0         # move input to argument register $a0
  	jal     factorial         # call factorial
        move $t1, $v0

        la $a0, head2	
        li   $v0, 4           #  printf("The value of 'factorial(
        syscall               
        la $a0, 0($t0) 
        li   $v0, 1           # print number
        syscall             
        la $a0,head3	
        li   $v0, 4           # print )' is: "
        syscall               
        la $a0, 0($t1)		# print factorial(number)
        li $v0, 1
        syscall
        li $v0, 10       # system call 10 is exit()
        li $a0, 0          # setting return code of program to 0 (success)
	syscall	       # Equivalent to "return" statement in main.
	
      .text
factorial: 
	addi $sp,$sp,-8
	sw $ra, 0($sp)
	sw $s0, 4($sp)
	
	li $v0,1
	beq $a0,$zero, return #if (x == 0)    return 1;

       	move $s0,$a0
       	addi $a0,$a0,-1
       	jal factorial # return x * factorial(x-1);
       	mul $v0,$s0,$v0
       	
	return: lw $ra, 0($sp)
       		lw $s0, 4($sp)
       		addi $sp, $sp, 8
       		jr $ra #return the function
      

	
	
