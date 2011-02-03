@ ARM code version of rotation routines.
@
@ @author Robin Watts (robin@wss.co.uk)
@
@ When rotating a block of memory to the screen, the key facts to bear in
@ mind are:
@  * Screen memory is uncached - therefore to get best performance we want
@    to write runs of horizontal pixels so the write buffer can kick in and
@    merge the buffered writes.
@  * Reading large numbers of pixels to be written out horizontally, pulls
@    in lots of cache lines. We need to be careful not to cache bust here.
@  * The 16 or 32 way set associativity for screens can hurt us here too.
@
@ A good compromise is therefore to write out in bursts of 4 horizontal
@ pixels at once.

	.text

	.global	ARM_rotate

        @ Reads block of w*h pixels from srcPtr (addressed by srcPixStep,
	@ srcLineStep) and stores them at dst (addressed by dstPixStep,
	@ dstLineStep), converting palette by table lookup in convertPalette.
ARM_rotate:
	@ r0 = destPtr
	@ r1 = srcPtr
	@ r2 = w
	@ r3 = h
	@ r4 = dstLineStep
	@ r5 = srcPixStep  - e.g. 480
	@ r6 = srcLineStep - e.g. 2 or -2
	MOV	r12,r13
	STMFD	r13!,{r4-r11,r14}
	LDMFD	r12,{r4-r6}

	@ For simplicity, we will think about width/height in terms of
	@ destination.

	AND	r7,r0,#6
	MOV	r7,r7,LSR #1
	AND	r7,r7,#3	@ r7 = Number over a multiple of 4 we start on
	RSB	r7,r7,#4	@ r7 = Number to do first time.
rotate_loop:
	CMP	r7,r2
	MOVGT	r7,r2		@ r7 = width to do this time

	SUBS	r7,r7,#4	@ r7 = width-4
	BLT	thin		@ less than 4 pixels wide
	SUB	r8,r4,#6	@ r8 = dstLineStep-6
x_loop_4:
	@ In this routine we will copy a 4 pixel wide stripe
	ADD	r9,r5,r5,LSL #1	@ r9 = 3*srcPixStep
	SUB	r9,r6,r9	@ r9 = srcLineStep-3*srcPixStep
	MOV	r7,r3		@ r7 = h
y_loop_4:
	@ r9 >= 0, so at least 4 to do.
	LDRH	r10,[r1],r5	@ r10 = *(src)
	LDRH	r11,[r1],r5	@ r11 = *(src+srcPixStep)
	LDRH	r12,[r1],r5	@ r12 = *(src+srcPixStep*2)
	LDRH	r14,[r1],r9	@ r14 = *(src+srcPixStep*3)  src+=srcLineStep
	STRH	r10,[r0],#2	@ *(ptr) = r10
	STRH	r11,[r0],#2	@ *(ptr+2) = r11
	STRH	r12,[r0],#2	@ *(ptr+4) = r12
	STRH	r14,[r0],r8	@ *(ptr+6) = r14    ptr += dstLineStep
	SUBS	r7,r7,#1	@ h--
	BGT	y_loop_4

	MUL	r10,r3,r6
	ADD	r1,r1,r5,LSL #2
	SUB	r1,r1,r10
	MUL	r10,r3,r4
	ADD	r0,r0,#8
	SUB	r0,r0,r10

	SUBS	r2,r2,#4	@ r2 = w -= 4
	BEQ	rotate_end	@ if w = 0, none left.
	SUBS	r7,r2,#4	@ r7 = w - 4
	BGE	x_loop_4	@ if 4 or more left, go back.
thin:
	MOV	r14,r3		@ r14 = h
thin_lp:
	ADDS	r7,r7,#2	@ Always do 1. GE => do 2. GT => do 3
	BGE	just_2
	BGT	just_3

	@ Just do a 1 pixel wide stripe. Either the last pixel stripe, or
	@ the first pixel stripe to align us.
y_loop_1:
	LDRH	r10,[r1],r6
	SUBS	r14,r14,#1
	STRH	r10,[r0],r4
	BGT	y_loop_1

	MUL	r10,r3,r6	@ How much to step r1 back to undo this line?
	ADD	r1,r1,r5	@ Move r1 on by srcPixStep
	SUB	r1,r1,r10	@ Move r1 back by amount just added on
	MUL	r10,r3,r4	@ How much to step r0 back to undo this line?
	ADD	r0,r0,#2	@ Move r0 on by dstPixStep
	SUB	r0,r0,r10	@ Move r0 back by amount just added on

	SUBS	r2,r2,#1	@ If we havent finished (i.e. we were doing
	MOV	r7,r2		@ the first pixel rather than the last one)
	BGT	rotate_loop	@ then jump back to do some more
rotate_end:
	LDMFD	r13!,{r4-r11,PC}

just_2:
	@ Just do a 2 pixel wide stripe. Either the last stripe, or
	@ the first stripe to align us.
	SUB	r9,r6,r5	@ r9 = srcLineStep - srcPixStep
	SUB	r8,r4,#2	@ r8 = dstLineStep - 2
y_loop_2:
	LDRH	r10,[r1],r5
	LDRH	r11,[r1],r9
	SUBS	r14,r14,#1
	STRH	r10,[r0],#2
	STRH	r11,[r0],r8
	BGT	y_loop_2

	MUL	r10,r3,r6	@ How much to step r1 back to undo this line?
	ADD	r1,r1,r5,LSL #1	@ Move r1 on by srcPixStep*2
	SUB	r1,r1,r10	@ Move r1 back by amount just added on
	MUL	r10,r3,r4	@ How much to step r0 back to undo this line?
	ADD	r0,r0,#4	@ Move r0 on by dstPixStep*2
	SUB	r0,r0,r10	@ Move r0 back by amount just added on

	SUBS	r2,r2,#2	@ If we havent finished (i.e. we were doing
	MOV	r7,r2		@ the first stripe rather than the last one)
	BGT	rotate_loop	@ then jump back to do some more

	LDMFD	r13!,{r4-r11,PC}
just_3:
	SUB	r9,r6,r5,LSL #1	@ r9 = srcLineStep - srcPixStep
	SUB	r8,r4,#4	@ r8 = dstLineStep - 2
y_loop_3:
	LDRH	r10,[r1],r5
	LDRH	r11,[r1],r5
	LDRH	r12,[r1],r9
	STRH	r10,[r0],#2
	STRH	r11,[r0],#2
	STRH	r12,[r0],r8
	SUBS	r14,r14,#1
	BGT	y_loop_3

	MUL	r10,r3,r6	@ How much to step r1 back to undo this line?
	ADD	r1,r1,r5	@ Move r1 on by srcPixStep*3
	ADD	r1,r1,r5,LSL #1
	SUB	r1,r1,r10	@ Move r1 back by amount just added on
	MUL	r10,r3,r4	@ How much to step r0 back to undo this line?
	ADD	r0,r0,#6	@ Move r0 on by dstPixStep*3
	SUB	r0,r0,r10	@ Move r0 back by amount just added on

	SUBS	r2,r2,#3	@ If we havent finished (i.e. we were doing
	MOV	r7,r2		@ the first stripe rather than the last one)
	BGT	rotate_loop	@ then jump back to do some more

	LDMFD	r13!,{r4-r11,PC}
