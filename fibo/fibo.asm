	add $t0, $zero, $zero, 0			#$t0 = 0
	add $s0, $imm, $zero, 0x100			#$s0 = 0x100. the beginning of memory saves.
	sw $t0, $zero, $s0, 0				#storing 0 in 0x100
	add $t1, $zero, $imm, 1			# $t1 = 1
	add $s0, $s0, $imm, 1			# $s0 = 0x101 (mem++)
	sw $t1, $zero, $s0, 0				#storing 0 in 0x101
	add $s0, $s0, $imm, 1			# $s0 = 0x102 (mem++)
	add $s1, $imm, $zero, 524287			#$s1 = 524,287. constant for later checks.
loop:
	add $s2, $t0, $t1, 0				# $s2=$t0+$t1
	add $t0, $t1, $zero, 0			# $t0=$t1
	add $t1, $s2, $zero, 0			# $t1=$s2
	bgt $imm, $s2, $s1, halt		# if $s2 >524,000 (overflow), then jump to halt (end program)

	sw $s2, $zero, $s0, 0		# store fib(n) in $s1. storing in (0x100 + n)
	add $s0, $s0, $imm, 1		#$s1 += 1 (memory++)
	beq $imm, $zero, $zero, loop		#looping "forever" unless overflow
halt:	
	halt $zero, $zero, $zero, 0			# halt
	