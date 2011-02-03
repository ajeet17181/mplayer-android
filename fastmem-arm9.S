/* fastmem-arm9.S - memset/memcpy functions optimized for ARM926EJ-S
   and probably all other cores from ARM9 family.

   Copyright (C) 2006  Siarhei Siamashka <ssvb@users.sourceforge.net>

   This software is provided 'as-is', without any express or implied
   warranty.  In no event will the authors be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
      claim that you wrote the original software. If you use this software
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.
   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original software.
   3. This notice may not be removed or altered from any source distribution.
*/
        .text

#ifndef ARM9_NO_STD_REPLACEMENT
        .global memset
        .global memcpy
#endif
        
/**
 * void *arm9_memset(void *_dst, int _c, size_t count)
 *
 * Faster replacement for standard memset function.
 * @param _dst   - pointer to data buffer
 * @param _c     - filler 8-bit value
 * @param _count - number of filler 8-bit values to be saved into buffer
 */        
        .align
        .global arm9_memset
        .func   arm9_memset
arm9_memset:
memset:
        mov     ip, r0
        
        cmp     r2, #4
        blt     3f
        /* Alignment is unknown */
        tst     ip, #1
        strneb  r1, [ip], #1
        subne   r2, r2, #1
        /* Now we are 16-bit aligned (need to upgrade 'c' to 16-bit) */
        and     r1, r1, #255
        orr     r1, r1, r1, asl #8
        tst     ip, #2
        strneh  r1, [ip], #2
        subne   r2, r2, #2
        /* Now we are 32-bit aligned (need to upgrade 'c' to 32-bit) */
        orr     r1, r1, r1, asl #16
        mov     r3, r1
        cmp     r2, #16
        blt     2f
        tst     ip, #4
        strne   r1, [ip], #4
        subne   r2, r2, #4
        tst     ip, #8
        stmneia ip!, {r1, r3}
        subne   r2, r2, #8
        /* Now we are 16-byte aligned */
        stmfd   sp!, {r4, r5}
        mov     r4, r1
        mov     r5, r1
1:      /* Copy 4 32-bit values twice per loop iteration */
        subs    r2, r2, #16
        stmgeia ip!, {r1, r3, r4, r5}
        subges  r2, r2, #16
        stmgeia ip!, {r1, r3, r4, r5}
        bge     1b
        add     r2, r2, #16
        ldmfd	sp!, {r4, r5}
2:      /* Copy up to 3 remaining 32-bit values */
        tst     r2, #8
        stmneia ip!, {r1, r3}
        tst     r2, #4
        strne   r1, [ip], #4
        and     r2, r2, #3
3:      /* Copy up to 3 remaining bytes */
        subs    r2, r2, #1
        strgeb  r1, [ip], #1
        subs    r2, r2, #1
        strgeb  r1, [ip], #1
        subs    r2, r2, #1
        strgeb  r1, [ip], #1

        bx      lr
        .endfunc
        
/**
 * uint16_t *arm9_memcpy16(uint16_t *_dst, uint16_t *_src, size_t _count)
 *
 * Function that is similar to standard memcpy function, but works with 16-bit data.
 * @param _dst   - pointer to destination buffer (16-bit aligned)
 * @param _src   - pointer to source buffer
 * @param _count - number of 16-bit values to be copied
 */        
        .align
        .global arm9_memcpy16
        .func   arm9_memcpy16
arm9_memcpy16:
        stmfd   sp!, {r0, r4, r5, r6, r7, r8, lr}
        cmp   r2, #2
        blt   6f
        /* Alignment is known to be at least 16-bit */
        tst   r0, #2
        ldrneh r4, [r1], #2
        strneh r4, [r0], #2
        subne  r2, r2, #1
        /* Now destination address is 32-bit aligned, still need to check whether */
        /* source is 32-bit aligned or not */
        tst   r1, #2
        bne   3f
        /* Both destination and source are 32-bit aligned */
        cmp   r2, #8
        blt   2f
        tst   r0, #4
        ldrne r4, [r1], #4
        strne r4, [r0], #4
        subne r2, r2, #2
        tst   r0, #8
        ldmneia r1!, {r4-r5}
        stmneia r0!, {r4-r5}
        subne r2, r2, #4
        /* Destination address is 128-bit aligned, source address is 32-bit aligned */
1:      subs  r2, r2, #8
        ldmgeia r1!, {r4-r7}
        stmgeia r0!, {r4-r7}
        bge   1b
        add   r2, r2, #8
        /* Copy up to 3 remaining aligned 32-bit values */
2:      tst   r2, #4
        ldmneia r1!, {r4-r5}
        stmneia r0!, {r4-r5}
        tst   r2, #2
        ldrne r4, [r1], #4
        strne r4, [r0], #4
        and  r2, r2, #1
        b      6f
        /* Destination is 32-bit aligned, but source is only 16-bit aligned */
3:      cmp   r2, #8
        blt   5f
        tst   r0, #4
        ldrneh r4, [r1], #2
        ldrneh r5, [r1], #2
        orrne  r4, r4, r5, asl #16
        strne r4, [r0], #4
        subne r2, r2, #2
        tst   r0, #8
        ldrneh r4, [r1], #2
        ldrne  r5, [r1], #4
        ldrneh r6, [r1], #2
        orrne  r4, r4, r5, asl #16
        movne  r5, r5, lsr #16
        orrne  r5, r5, r6, asl #16
        stmneia r0!, {r4-r5}
        subne r2, r2, #4
        /* Destination is 128-bit aligned, but source is only 16-bit aligned */
4:      subs  r2, r2, #8
        ldrgeh r4, [r1], #2
        ldmgeia r1!, {r5-r7}
        ldrgeh r3, [r1], #2
        orrge r4, r4, r5, asl #16
        movge r5, r5, lsr #16
        orrge r5, r5, r6, asl #16
        movge r6, r6, lsr #16
        orrge r6, r6, r7, asl #16
        movge r7, r7, lsr #16
        orrge r7, r7, r3, asl #16
        stmgeia r0!, {r4-r7}
        bge    4b
        add    r2, r2, #8
        /* Copy up to 6 remaining 16-bit values (to 32-bit aligned destination) */
5:      subs   r2, r2, #2
        ldrgeh r4, [r1], #2
        ldrgeh r5, [r1], #2
        orrge  r4, r4, r5, asl #16
        strge  r4, [r0], #4
        bge    5b
        add    r2, r2, #2
        /* Copy the last remaining 16-bit value if any */
6:      subs   r2, r2, #1
        ldrgeh r4, [r1], #2
        strgeh r4, [r0], #2

        ldmfd   sp!, {r0, r4, r5, r6, r7, r8, pc}
        .endfunc
        
/**
 * Helper macro for memcpy function, it can copy data from source (r1) to 
 * destination (r0) buffers fixing alignment in the process. Destination
 * buffer should be aligned already (4 bytes alignment is required.
 * Size of the block to copy is in r2 register
 */        
.macro  UNALIGNED_MEMCPY shift
        sub     r1, #(\shift)
        ldr     ip, [r1], #4

        tst     r0, #4
        movne   r3, ip, lsr #(\shift * 8)
        ldrne   ip, [r1], #4
        subne   r2, r2, #4
        orrne   r3, r3, ip, asl #(32 - \shift * 8)
        strne   r3, [r0], #4

        tst     r0, #8
        movne   r3, ip, lsr #(\shift * 8)
        ldmneia r1!, {r4, ip}
        subne   r2, r2, #8
        orrne   r3, r3, r4, asl #(32 - \shift * 8)
        movne   r4, r4, lsr #(\shift * 8)
        orrne   r4, r4, ip, asl #(32 - \shift * 8)
        stmneia r0!, {r3-r4}
        cmp     r2, #32
        blt     3f
        pld     [r1, #48]
        stmfd   sp!, {r7, r8, r10, r11}
1:
        pld     [r1, #80]
        subs    r2, r2, #32
        movge   r3, ip, lsr #(\shift * 8)
        ldmgeia r1!, {r4-r6, r7, r8, r10, r11, ip}
        orrge   r3, r3, r4, asl #(32 - \shift * 8)
        movge   r4, r4, lsr #(\shift * 8)
        orrge   r4, r4, r5, asl #(32 - \shift * 8)
        movge   r5, r5, lsr #(\shift * 8)
        orrge   r5, r5, r6, asl #(32 - \shift * 8)
        movge   r6, r6, lsr #(\shift * 8)
        orrge   r6, r6, r7, asl #(32 - \shift * 8)
        stmgeia r0!, {r3-r6}
        movge   r7, r7, lsr #(\shift * 8)
        orrge   r7, r7, r8, asl #(32 - \shift * 8)
        movge   r8, r8, lsr #(\shift * 8)
        orrge   r8, r8, r10, asl #(32 - \shift * 8)
        movge   r10, r10, lsr #(\shift * 8)
        orrge   r10, r10, r11, asl #(32 - \shift * 8)
        movge   r11, r11, lsr #(\shift * 8)
        orrge   r11, r11, ip, asl #(32 - \shift * 8)
        stmgeia r0!, {r7, r8, r10, r11}
        bgt     1b
2:
        ldmfd   sp!, {r7, r8, r10, r11}
3:      /* copy remaining data */
        tst     r2, #16
        movne   r3, ip, lsr #(\shift * 8)
        ldmneia r1!, {r4-r6, ip}
        orrne   r3, r3, r4, asl #(32 - \shift * 8)
        movne   r4, r4, lsr #(\shift * 8)
        orrne   r4, r4, r5, asl #(32 - \shift * 8)
        movge   r5, r5, lsr #(\shift * 8)
        orrge   r5, r5, r6, asl #(32 - \shift * 8)
        movge   r6, r6, lsr #(\shift * 8)
        orrge   r6, r6, ip, asl #(32 - \shift * 8)
        stmneia r0!, {r3-r6}

        tst     r2, #8
        movne   r3, ip, lsr #(\shift * 8)
        ldmneia r1!, {r4, ip}
        orrne   r3, r3, r4, asl #(32 - \shift * 8)
        movne   r4, r4, lsr #(\shift * 8)
        orrne   r4, r4, ip, asl #(32 - \shift * 8)
        stmneia r0!, {r3-r4}

        tst     r2, #4
        movne   r3, ip, lsr #(\shift * 8)
        ldrne   ip, [r1], #4
        sub     r1, r1, #(4 - \shift)
        orrne   r3, r3, ip, asl #(32 - \shift * 8)
        strne   r3, [r0], #4

        tst     r2, #2
        ldrneb  r3, [r1], #1
        ldrneb  r4, [r1], #1
        ldr     r5, [sp], #4
        strneb  r3, [r0], #1
        strneb  r4, [r0], #1

        tst     r2, #1
        ldrneb  r3, [r1], #1
        ldr     r6, [sp], #4
        strneb  r3, [r0], #1

        ldmfd   sp!, {r0, r4}

        bx      lr
.endm

/**
 * void *arm9_memcpy(void *_dst, void *_src, size_t _count)
 *
 * Function that is similar to standard memcpy function
 * @param _dst   - pointer to destination buffer
 * @param _src   - pointer to source buffer
 * @param _count - number of bytes to copy
 */        
        .align
        .global arm9_memcpy
        .func   arm9_memcpy
arm9_memcpy:
memcpy:
        cmp     r2, #20
        blt     memcpy_small_block
        /* copy data until destination address is 4 bytes aligned */
        tst     r0, #1
        ldrneb  r3, [r1], #1
        stmfd   sp!, {r0, r4}
        subne   r2, r2, #1
        strneb  r3, [r0], #1
        tst     r0, #2
        ldrneb  r3, [r1], #1
        ldrneb  r4, [r1], #1
        stmfd   sp!, {r5, r6}
        subne   r2, r2, #2
        orrne   r3, r3, r4, asl #8
        strneh  r3, [r0], #2
        /* destination address is 4 bytes aligned */
        /* now we should handle 4 cases of source address alignment */
        tst     r1, #1
        bne     memcpy_dst_4bytes_aligned_src1or3
        tst     r1, #2
        bne     memcpy_dst_4bytes_aligned_src2
memcpy_dst_4bytes_aligned_src0:
        /* both source and destination are 4 bytes aligned */
        stmfd   sp!, {r7, r8, r10, r11}
        tst     r0, #4
        ldrne   r4, [r1], #4
        subne   r2, r2, #4
        strne   r4, [r0], #4
        tst     r0, #8
        ldmneia r1!, {r3-r4}
        subne   r2, r2, #8
        stmneia r0!, {r3-r4}
1:
        subs    r2, r2, #32
        ldmgeia r1!, {r3-r6, r7, r8, r10, r11}
        pld     [r1, #48]
        stmgeia r0!, {r3-r6}
        stmgeia r0!, {r7, r8, r10, r11}
        bgt     1b
2:
        ldmfd   sp!, {r7, r8, r10, r11}
        tst     r2, #16
        ldmneia r1!, {r3-r6}
        stmneia r0!, {r3-r6}
        tst     r2, #8
        ldmneia r1!, {r3-r4}
        stmneia r0!, {r3-r4}
        tst     r2, #4
        ldrne   r3, [r1], #4
        mov     ip, r0
        strne   r3, [ip], #4
        tst     r2, #2
        ldrneh  r3, [r1], #2
        ldmfd   sp!, {r5, r6}
        strneh  r3, [ip], #2
        tst     r2, #1
        ldrneb  r3, [r1], #1
        ldmfd   sp!, {r0, r4}
        strneb  r3, [ip], #1

        bx      lr

memcpy_dst_4bytes_aligned_src1or3:
        tst    r1, #2
        bne    memcpy_dst_4bytes_aligned_src3
memcpy_dst_4bytes_aligned_src1:
        UNALIGNED_MEMCPY 1

memcpy_dst_4bytes_aligned_src2:
        UNALIGNED_MEMCPY 2

memcpy_dst_4bytes_aligned_src3:
        UNALIGNED_MEMCPY 3

memcpy_small_block:
        stmfd  sp!, {r0, r4}
1:      subs   r2, r2, #3
        ldrgeb ip, [r0]
        ldrgeb r3, [r1], #1
        ldrgeb r4, [r1], #1
        ldrgeb ip, [r1], #1
        strgeb r3, [r0], #1
        strgeb r4, [r0], #1
        strgeb ip, [r0], #1
        bge    1b
        adds   r2, r2, #2
        ldrgeb r3, [r1], #1
        mov    ip, r0
        ldr    r0, [sp], #4
        strgeb r3, [ip], #1
        ldrgtb r3, [r1], #1
        ldr    r4, [sp], #4
        strgtb r3, [ip], #1
        bx     lr
        .endfunc
