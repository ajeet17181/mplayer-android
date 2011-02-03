/*
Copyright (c) 2001 Lionel Ulmer (lionel.ulmer@free.fr / bbrox@bbrox.org)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/* WARNING : this function only works when stride_U == stride_V (I use some hacks to
	     not have to do too many computations at line's end)...

   C-like prototype :
	void convert_yuv420_rgb565(AVPicture *picture, unsigned char *results, int w, int h) ;

*/

#ifdef __arm__

	.text
	.align
	
	.global convert_yuv420_rgb565
convert_yuv420_rgb565:
	stmdb   sp!, { r4 - r12, lr }   @ all callee saved regs
	ldr r7,  [r0,  #0]       @ Y ptr
	ldr r9,  [r0,  #4]       @ U ptr
	ldr r10, [r0,  #8]       @ V ptr
	subs r10, r10, r9        @ V ptr - U ptr
	ldr r8,  [r0, #12]
	add r8, r8, r7           @ Y + stride_Y
	ldr r4,  [r0, #12]       @ Stride_Y
	mov r4, r4, lsl #1
	sub r4, r4, r2           @ (2 * Stride_Y) - width
	ldr r5,  [r0, #16]       @ Stride_U
	sub r5, r5, r2, lsr #1   @ Stride_U - (width / 2)
	ldr r6,  [r0, #20]       @ Stride_V
	sub r6, r6, r2, lsr #1   @ Stride_V - (width / 2)
	add r0, r1, r2, lsl #1   @ RGB + 1
	stmdb   sp!, { r0-r10 }
	@ Stack description :
	@ (sp+ 0) RGB + one line
	@ (sp+ 4) RGB
	@ (sp+ 8) width (save)
	@ (sp+12) height
	@ (sp+16) (2 * stride_Y) - width
	@ (sp+20) stride_U - (width / 2)
	@ (sp+24) stride_V - (width / 2) !!! UNUSED !!!
	@ (sp+28) Y ptr
	@ (sp+32) Y ptr + one line
	@ (sp+36) U ptr
	@ (sp+40) V - U
	mov lr, r2                         @ Initialize the width counter
	add r0, pc, #(const_storage-.-8)   @ r0 = base pointer to the constants array
	ldr r8, [r0, #(4*4)]               @ r8 = multy	
yuv_loop:
	add r0, pc, #(const_storage-.-8)   @ r0 = base pointer to the constants array
	ldr r10, [sp, #28]                 @ r10 = Y
	ldr r1, [sp, #36]                  @ r1 = U
	ldrb r9, [r10, #0]                 @ r9 = *Y
	ldrb r11, [r1]                     @ r11 = *U
	add r1, r1, #1                     @ r1 = U++
	ldr r2, [sp, #40]                  @ r2 = V - U
	str r1, [sp, #36]                  @ store U++
	add r2, r1, r2                     @ r2 = V+1
	ldrb r12, [r2, #-1]                @ r12 = *V
	sub r11, r11, #128                 @ r11 = *U - 128
	sub r12, r12, #128                 @ r12 = *V - 128
	ldr r1, [r0, #(4*0)]               @ r1 = crv
	mov r7, #32768                     @ r7 = 32768 (for additions in MLA)
	ldr r2, [r0, #(4*3)]               @ r2 = -cgv
	mla r6, r1, r12, r7                @ r6 = nonyc_r = crv * (*V - 128) + 32768
	ldr r3, [r0, #(4*1)]               @ r3 = cbu
	mla r4, r2, r12, r7                @ r4 = - cgv * (*V - 128) + 32768
	sub r9, r9, #16                    @ r9 = *Y - 16
	mla r5, r3, r11, r7                @ r5 = nonyc_b = cbu * (*U - 128) + 32768	
	ldr r0, [r0, #(4*2)]               @ r0 = -cgu
	mla r7, r8, r9, r6                 @ r7 = (*Y - 16) * multy + nonyc_r
	add r10, r10, #2                   @ r10 = Y + 2
	mla r4, r0, r11, r4                @ r4 = nonyc_g = - cgu * (*U - 128) + r4 = - cgu * (*U - 128) - cgv * (*V - 128) + 32768
	add r0, pc, #(rb_clip-.-8)         @ r0 contains the pointer to the R and B clipping array
	mla r12, r8, r9, r5                @ r12 = (*Y - 16) * multy + nonyc_b
	ldrb r7, [r0, r7, asr #(16+3)]     @ r7 = R composant
	mla r1, r8, r9, r4                 @ r1 = (*Y - 16) * multy + nonyc_g
	ldrb r9, [r10, #-1]                @ r9 = *(Y+1)
	str r10, [sp, #28]                 @ save Y + 2
	ldrb r12, [r0, r12, asr #(16+3)]   @ r12 = B composant (and the start of the RGB word)
	add r11, pc, #(g_clip-.-8)         @ r11 now contains the pointer to the G clipping array
	ldrb r1, [r11, r1, asr #(16+2)]    @ r1 contains the G part of the RGB triplet
	sub r9, r9, #16                    @ r9 = *(Y+1) - 16
	mla r10, r8, r9, r6                @ r10 is the Red part of the RGB triplet
	add r12, r12, r7, lsl #11          @ r12 = .GB ...
	mla r7, r8, r9, r5                 @ r7 is the Blue part of the RGB triplet
	add r12, r12, r1, lsl #5           @ r12 = RGB ... (ie the first pixel (half-word) is done)
	mla r2, r8, r9, r4                 @ r2 is the Green part of the RGB triplet
	ldrb r10, [r0, r10, asr #(16+3)]   @ r10 = R composant
	ldrb r7, [r0, r7, asr #(16+3)]     @ r7 = B composant
	ldr r1, [sp, #32]                  @ r1 = Ynext
	ldrb r2, [r11, r2, asr #(16+2)]    @ r2 = G composant
	ldrb r9, [r1]                      @ r9 = *Ynext
	add r12, r12, r2, lsl #(5+16)      @ r12 = RGB .G.
	sub r9, r9, #16                    @ r9 = *Ynext - 16
	mla r2, r8, r9, r4                 @ r2 is the Green part of the RGB triplet
	add r12, r12, r7, lsl #(0+16)      @ r12 = RGB .GB
	mla r7, r8, r9, r5                 @ r7 is the Blue part of the RGB triplet
	add r12, r12, r10, lsl #(11+16)    @ r12 = RGB RGB
	ldr r3, [sp, #4]                   @ r3 = RGB
	mla r10, r8, r9, r6                @ r10 is the Red part of the RGB triplet
	str r12, [r3]                      @ store the rgb pixel at *RGB
	add r3, r3, #4                     @ r3 = RGB++ (ie next double-pixel)
	str r3, [sp, #4]                   @ store the RGB pointer
	ldrb r9, [r1, #1]                  @ r9 = *(Ynext+1)
	add r1, r1, #2                     @ r1 = Ynext + 2
	sub r9, r9, #16                    @ r9 = *(Ynext+1) - 16
	ldrb r12, [r0, r7, asr #(16+3)]    @ r12 = ..B ...
	ldrb r10, [r0, r10, asr #(16+3)]   @ r10 = B composant
	mla r7, r8, r9, r5                 @ r7 is the Blue part of the RGB triplet
	add r12, r12, r10, lsl #11         @ r12 = R.B ...
	ldrb r2, [r11, r2, asr #(16+2)]    @ r2 = G composant
	mla r10, r8, r9, r6                @ r10 is the Red part of the RGB triplet
	add r12, r12, r2, lsl #5           @ r12 = RGB ...
	mla r2, r8, r9, r4                 @ r2 is the Green part of the RGB triplet
	ldrb r7, [r0, r7, asr #(16+3)]     @ r7 = B composant
	str r1, [sp, #32]                  @ store the increased Ynext pointer
	add r12, r12, r7, lsl #(16+0)      @ r12 = RGB ..B
	ldrb r10, [r0, r10, asr #(16+3)]   @ r10 = R composant
	ldr r3, [sp, #0]                   @ r3 = RGBnext pointer
	add r12, r12, r10, lsl #(16+11)    @ r12 = RGB R.B
	ldrb r2, [r11, r2, asr #(16+2)]    @ r2 = G composant
	add r3, r3, #4                     @ r3 = next pixel on the RGBnext line
	add r12, r12, r2, lsl #(16+5)      @ r12 = RGB RGB
	str r12, [r3, #-4]                 @ store the next pixel
	str r3, [sp, #0]                   @ store the increased 'next line' pixel pointer
	subs lr, lr, #2                    @ decrement the line counter
	bne yuv_loop                       @ and restart if not at the end of the line

	ldr r0, [sp, #8]                   @ r0 = saved width
	ldr r1, [sp, #0]                   @ r1 = RGBnext pointer
	mov lr, r0                         @ lr = saved width (to restart the line counter)
	str r1, [sp, #4]                   @ current RGBnext pointer is next iteration RGB pointer
	add r1, r1, r0, lsl #1             @ r1 = update RGBnext to next line
	str r1, [sp, #0]                   @ store updated RGBnext pointer

	ldr r3, [sp, #16]                  @ r3 = (2 * stride_Y) - width
	ldr r4, [sp, #28]                  @ r4 = Y ptr
	ldr r5, [sp, #32]                  @ r5 = Ynext ptr
	add r4, r4, r3                     @ r4 = Y ptr for the next two lines
	add r5, r5, r3                     @ r5 = Ynext ptr for the next two lines
	str r4, [sp, #28]                  @ store updated Y pointer
	str r5, [sp, #32]                  @ store update Ynext pointer

	ldr r1, [sp, #20]                  @ r1 = stride_U - (width / 2)
	ldr r2, [sp, #36]                  @ r2 = U ptr

	ldr r6, [sp, #12]                  @ get height counter
	
	add r2, r2, r1                     @ update U ptr
	str r2, [sp, #36]                  @ store updated U ptr (and update 'V' at the same time :-) )

	subs r6, r6, #2
	str r6, [sp, #12]
	bne yuv_loop
	
	@ Exit cleanly :-)
	add sp, sp, #(11*4)             @ remove all custom things from stack
	ldmia   sp!, { r4 - r12, pc }   @ restore callee saved regs and return


const_storage:
	@ In order : crv, cbu, - cgu, - cgv, multy
	.word 0x00019895, 0x00020469, 0xffff9bb5, 0xffff2fe1, 0x00012A15
rb_clip_dummy:	
        .byte 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
        .byte 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
        .byte 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
rb_clip:
        .byte 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f
        .byte 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f
        .byte 0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f
        .byte 0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f
        .byte 0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f
g_clip_dummy:	
        .byte 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
        .byte 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
        .byte 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
        .byte 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
        .byte 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
        .byte 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
g_clip:	
        .byte 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f
        .byte 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f
        .byte 0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f
        .byte 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f
        .byte 0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f
        .byte 0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f
        .byte 0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f
        .byte 0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f
        .byte 0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f
        .byte 0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f

#endif

