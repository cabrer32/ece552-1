	.file	1 "mbq1.c"

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
	.frame	$sp,24,$31		# vars= 0, regs= 1/0, args= 16, extra= 0
	.mask	0x80000000,-8
	.fmask	0x00000000,0
	subu	$sp,$sp,24
	sw	$31,16($sp)
	jal	__main
	li	$8,0x00000002		# 2
	li	$9,0x00000003		# 3
	li	$4,0x00000005		# 5
	li	$5,0x00000005		# 5
	li	$7,0x00000009		# 9
	move	$3,$0
	li	$6,0x02faf080		# 50000000
$L4:
	addu	$2,$8,$4
	addu	$12,$2,$9
	addu	$3,$3,1
	addu	$4,$4,3
	addu	$5,$5,2
	bne	$3,$6,$L4
	move	$3,$0
	li	$11,0x00989680		# 10000000
$L8:
	addu	$6,$8,$4
	addu	$7,$7,2
	addu	$10,$6,$9
	addu	$3,$3,1
	addu	$4,$4,3
	addu	$5,$5,2
	bne	$3,$11,$L8
	addu	$2,$2,$8
	addu	$2,$2,$9
	addu	$2,$2,$12
	addu	$2,$2,$4
	addu	$2,$2,$5
	addu	$2,$2,$7
	addu	$2,$2,$6
	addu	$2,$2,$10
	lw	$31,16($sp)
	addu	$sp,$sp,24
	j	$31
	.end	main
