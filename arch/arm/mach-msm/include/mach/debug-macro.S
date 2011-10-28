/*
 *
 * Copyright (C) 2007 Google, Inc.
 * Author: Brian Swetland <swetland@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */



#include <mach/hardware.h>
#include <mach/msm_iomap.h>
#include <mach/msm_serial_hsl_regs.h>

<<<<<<< HEAD
#ifdef MSM_DEBUG_UART_PHYS
       .macro  addruart, rp, rv
	ldr     \rp, =MSM_DEBUG_UART_PHYS
	ldr     \rv, =MSM_DEBUG_UART_BASE
=======
#if defined(CONFIG_HAS_MSM_DEBUG_UART_PHYS) && !defined(CONFIG_MSM_DEBUG_UART_NONE)
	.macro	addruart, rp, rv, tmp
	ldr	\rp, =MSM_DEBUG_UART_PHYS
	ldr	\rv, =MSM_DEBUG_UART_BASE
>>>>>>> 1fdb24e... Merge branch 'devel-stable' of http://ftp.arm.linux.org.uk/pub/linux/arm/kernel/git-cur/linux-2.6-arm
	.endm

	.macro	senduart,rd,rx
#ifdef CONFIG_MSM_HAS_DEBUG_UART_HS
	@ Clear TX_READY by writing to the UARTDM_CR register
	mov	r12, #0x300
	str	r12, [\rx, #UARTDM_CR_OFFSET]
	@ Write 0x1 to NCF register
	mov 	r12, #0x1
	str	r12, [\rx, #UARTDM_NCF_TX_OFFSET]
	@ UARTDM reg. Read to induce delay
	ldr	r12, [\rx, #UARTDM_SR_OFFSET]
	@ Write the 1 character to UARTDM_TF
	str	\rd, [\rx, #UARTDM_TF_OFFSET]
#else
	teq     \rx, #0
	strne   \rd, [\rx, #0x0C]
#endif
	.endm

	.macro	waituart,rd,rx
#ifdef CONFIG_MSM_HAS_DEBUG_UART_HS
	@ check for TX_EMT in UARTDM_SR
	ldr	\rd, [\rx, #UARTDM_SR_OFFSET]
	tst	\rd, #0x08
	bne	1002f
	@ wait for TXREADY in UARTDM_ISR
1001:	ldreq	\rd, [\rx, #UARTDM_ISR_OFFSET]
	tst	\rd, #0x80
	dsb
	beq 	1001b
#else
	@ wait for TX_READY
1001:   ldr \rd, [\rx, #0x08]
	tst     \rd, #0x04
	beq     1001b
#endif
1002:
	.endm

#else
<<<<<<< HEAD

	.macro  addruart, rp, rv
=======
	.macro  addruart, rp, rv, tmp
	mov	\rv, #0xff000000
	orr	\rv, \rv, #0x00f00000
>>>>>>> 1fdb24e... Merge branch 'devel-stable' of http://ftp.arm.linux.org.uk/pub/linux/arm/kernel/git-cur/linux-2.6-arm
	.endm

	.macro	senduart,rd,rx
	.endm

	.macro	waituart,rd,rx
	.endm
#endif

	.macro	busyuart,rd,rx
	.endm
