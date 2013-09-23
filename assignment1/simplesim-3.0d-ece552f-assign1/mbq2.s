	.file	1 "mbq2.c"

 # GNU C 2.7.2.3 [AL 1.1, MM 40, tma 0.1] SimpleScalar running sstrix compiled by GNU C

 # Cc1 defaults:
 # -mgas -mgpOPT

 # Cc1 arguments (-G value = 8, Cpu = default, ISA = 1):
 # -quiet -dumpbase -O1 -o

gcc2_compiled.:
__gnu_compiled_c:
	.text
	.align	2
	.globl	main

	.text

	.loc	1 4
	.ent	main
main:
	.frame	$sp,40,$31		# vars= 16, regs= 1/0, args= 16, extra= 0
	.mask	0x80000000,-8
	.fmask	0x00000000,0
	subu	$sp,$sp,40
	sw	$31,32($sp)
	jal	__main
	li	$10,0x00000001		# 1
	li	$2,0x00000005		# 5
	li	$11,0x00000009		# 9
	li	$6,0x00000007		# 7
	li	$12,0x00000002		# 2
	li	$13,0x00000003		# 3
	li	$5,0x00000007		# 7
	li	$8,0x00000004		# 4
	move	$3,$0
	addu	$4,$11,3
	li	$7,0x0007a120		# 500000
	#.set	volatile
	sw	$10,16($sp)
	#.set	novolatile
	#.set	volatile
	sw	$10,20($sp)
	#.set	novolatile
	#.set	volatile
	sw	$2,24($sp)
	#.set	novolatile
	#.set	volatile
	sw	$2,28($sp)
	#.set	novolatile
$L4:
	.set	noreorder
	#.set	volatile
	lw	$2,20($sp)
	#.set	novolatile
	.set	reorder
	addu	$6,$6,3
	addu	$3,$3,$2
	#.set	volatile
	sw	$4,24($sp)
	#.set	novolatile
	bne	$3,$7,$L4
	move	$3,$0
	addu	$7,$11,3
	addu	$4,$6,2
	li	$9,0x000186a0		# 100000
$L8:
	.set	noreorder
	#.set	volatile
	lw	$2,16($sp)
	#.set	novolatile
	#nop
	.set	reorder
	addu	$3,$3,$2
	#.set	volatile
	sw	$7,24($sp)
	#.set	novolatile
	#.set	volatile
	sw	$4,28($sp)
	#.set	novolatile
	bne	$3,$9,$L8
	move	$3,$0
	li	$2,0x0003d090		# 250000
$L12:
	addu	$4,$12,$5
	addu	$7,$4,$13
	addu	$3,$3,1
	addu	$5,$5,3
	addu	$8,$8,2
	bne	$3,$2,$L12
	addu	$2,$10,$3
	.set	noreorder
	#.set	volatile
	lw	$3,24($sp)
	#.set	novolatile
	#nop
	.set	reorder
	addu	$2,$2,$3
	.set	noreorder
	#.set	volatile
	lw	$3,28($sp)
	#.set	novolatile
	#nop
	.set	reorder
	addu	$2,$2,$3
	addu	$2,$2,$6
	addu	$2,$2,$11
	addu	$2,$2,$4
	addu	$2,$2,$12
	addu	$2,$2,$7
	addu	$2,$2,$13
	addu	$2,$2,$5
	addu	$2,$2,$8
	lw	$31,32($sp)
	addu	$sp,$sp,40
	j	$31
	.end	main
