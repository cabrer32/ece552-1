	.file	1 "mb.c"

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

	.loc	1 6
	.ent	main
main:
	.frame	$sp,24,$31		# vars= 0, regs= 1/0, args= 16, extra= 0
	.mask	0x80000000,-8
	.fmask	0x00000000,0
	subu	$sp,$sp,24
	sw	$31,16($sp)
	jal	__main
	move	$5,$0
	move	$6,$0
	move	$4,$0
$L5:
	move	$3,$0
$L9:
	addu	$5,$5,1
	addu	$6,$6,1
	addu	$3,$3,1
	slt	$2,$3,3
	bne	$2,$0,$L9
	addu	$4,$4,1
	slt	$2,$4,5000
	bne	$2,$0,$L5
	addu	$2,$4,$3
	addu	$2,$2,$5
	addu	$2,$2,$6
	lw	$31,16($sp)
	addu	$sp,$sp,24
	j	$31
	.end	main
