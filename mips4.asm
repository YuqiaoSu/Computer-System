# Compute the given number's factorial
      .data
head1:.asciiz  "Positive Integer: "          
head2: .asciiz "The value of factorial("
head3: .asciiz ") is "
	.text
main:   la $a0, head1
	 li   $v0, 4           # specify Print String service
        syscall               # print head1
        li $v0, 5  #read from input
        syscall
        sw $v0,0($a0)
        jal factorial
        
print:	add $t1, $zero, $a0  #factorial number
	add $t2, $zero, $a1  #return answer


        la $a0, head2	
        li   $v0, 4           # specify Print String service
        syscall               # print head2
        la $a0, 0($t1) 
        li   $v0, 1           # specify Print Integer service
        syscall               # print factorial number
        la $a0,head3	
        li   $v0, 4           # specify Print String service
        syscall               # print head2
        la $a0, 0($t2)
        li $v0, 1
        syscall
        li $v0, 10       # system call 10 is exit()
        li $a0, 0          # setting return code of program to 0 (success)
	syscall	       # Equivalent to "return" statement in main.
	
      .text
factorial: la $t0, 0($a0)
       lw $t0, 0($t0)
       addi $t1, $zero, 1
       li $t3, 1
loop:  addi $t2, $t1, 1
       	mul $t3, $t2, $t3
       	addi $t1,$t1,1
       	blt $t1, $t0, loop
       	la $a0, 0($t0)  # print factorial number
       	add $a1, $zero, $t3 # print the answer stored in t3
       jal  print            # call print routine. 
       li   $v0, 10          # system call for exit
       li $a0, 0          # setting return code of program to 0 (success)
       syscall               # we are out of here.
       	
      

	
	