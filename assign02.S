#include "hardware/regs/addressmap.h"
#include "hardware/regs/io_bank0.h"
#include "hardware/regs/timer.h"
#include "hardware/regs/m0plus.h"

.syntax unified                                                 @ Specify unified assembly syntax
.cpu    cortex-m0plus                                           @ Specify CPU type is Cortex M0+
.thumb                                                          @ Specify thumb assembly for RP2040
.global main_asm                                                @ Provide program starting address to the linker
.align 4                                                        @ Specify code alignment

@ Define constants

.equ    DFLT_TIME, 0x0

.equ    GPIO_BTN_MSK_RISE_CLEAR, 0x00800000             @ Bit-23 for rising-edge event on GP21
.equ    GPIO_BTN_MSK_FALL_CLEAR, 0x00400000             @ Bit-22 for falling-edge event on GP21
.equ    GPIO_BTN_MSK_RISE, 0x00800000                   @ Bit-21 for falling-edge event on GP21
.equ    GPIO_BTN_MSK_FALL, 0x00400000                   @ Bit-20 for rising-edge event on GP21

.equ    GPIO_BTN_PIN,   21                              @ Specify pin for GPIO button 21
.equ    GPIO_DIR_IN,    0                               @ Specify input direction for a GPIO pin
.equ    GPIO_DIR_OUT,   1                               @ Specify output direction for a GPIO pin

.equ    GPIO_ISR_OFFSET, 0x74                           @ GPIO is int #13 (vector table entry 29)

@ Entry point to the ASM portion of the program
main_asm:
    bl      init_gpio_buttons                           @ Call subroutine to initialise GPIO buttons
    bl      install_irq_gpio                            @ Call subroutine to install GPIO interrupt handler
main_loop:
    b       main_loop                                   @ Infinite loop

init_gpio_buttons:
    push    {r0, r1, lr}                                @ Preserve registers
    movs    r0, #GPIO_BTN_PIN                           @ This value is the GPIO button 21 pin on the PI PICO board
    bl      asm_gpio_init                               @ Call the subroutine to initialise the GPIO pin specified by r0
    movs    r0, #GPIO_BTN_PIN                           @ This value is the GPIO button 21 pin on the PI PICO board
    movs    r1, #GPIO_DIR_IN                            @ We want this GPIO pin to be setup as an input pin
    bl      asm_gpio_set_dir                            @ Call the subroutine to set the GPIO pin specified by r0 to state specified by r1
    movs    r0, #GPIO_BTN_PIN                           @ This value is the GPIO button 21 pin on the PI PICO board
    bl      asm_gpio_set_irq                            @ Call subroutine to enable falling-edge interrupt for GPIO 20
    pop     {r0, r1, pc}                                @ Restore registers

@ Subroutine to install an interrupt handler for GPIO
install_irq_gpio:
    push    {r4, r5, lr}                                @ Preserve registers (incl. LR)
    ldr     r4, =(PPB_BASE + M0PLUS_VTOR_OFFSET)        @ Load the address of the VTOR
    ldr     r4, [r4]                                    @ Load the VTOR
    ldr     r5, =gpio_isr                               @ Load the address of the ISR handler
    str     r5, [r4, GPIO_ISR_OFFSET]                   @ Store the handler into the GPIO ISR
    ldr     r0, =0x2000                                 @ Load appropriate value to write IO_IRQ_BANK0 bit in NVIC
    bl      enable_interrupt                            @ Call subroutine to enable interrupt specified by r0
    pop     {r4, r5, pc}                                @ Restore registers and exit subroutine

@ Helper subroutine to enable an interrupt
@ Params:
@   r0: The interrupt you wish to enable in 32-bit vector form
enable_interrupt:
    push    {r4, lr}                                    @ Preserve registers (incl. LR)
    ldr     r4, =(PPB_BASE + M0PLUS_NVIC_ICPR_OFFSET)   @ Load the address of the Interrupt Clear Pending Register
    str     r0, [r4]                                    @ Clear pending interrupts (by writing the TIMER_IRQ_0 bit)
    ldr     r4, =(PPB_BASE + M0PLUS_NVIC_ISER_OFFSET)   @ Load the address of the Interrupt Set Enable Register
    str     r0, [r4]                                    @ Enable the interrupt (by writing the TIMER_IRQ_0 bit)
    pop     {r4, pc}                                    @ Restore registers and exit subroutine

@ GPIO interrupt service handler routine
.thumb_func
gpio_isr:
    push    {lr}
    add     r3, r3, r4
    ldr     r4, =(IO_BANK0_BASE + IO_BANK0_INTR2_OFFSET) @ Load INTR2 register address
    ldr     r5, =GPIO_BTN_MSK_FALL                       @ Load value to write to GPIO 21 bit 
    ldr     r6, [r4]
    ands    r6, r5, r6
    lsrs    r6, #22
    cmp     r6, #1
    bne     rise_edge
    @ It's a falling-edge event, just store the current time
    ldr     r7, =GPIO_BTN_MSK_FALL_CLEAR                @ Load value to write to GPIO 21 bit 
    b       gpio_isr_clear
rise_edge:
    ldr     r7, =GPIO_BTN_MSK_RISE_CLEAR                @ Load value to write to GPIO 21 bit 
gpio_isr_clear:
    ldr     r4, =ltime                                  @ Load the address of the timer var
    ldr     r4, [r4]                                    @ Load the timer var
    ldr     r5, =(TIMER_BASE + TIMER_TIMELR_OFFSET)     @ Load the address of the timer (lwr 32 bits) register
    ldr     r5, [r5]                                    @ Load the lwr 32 bits of the time
    ldr     r6, =ltime                                  @ Load the address of the timer var
    str     r5, [r6]                                    @ Store the current time in the var
    subs    r0, r5, r4                                  @ diff = currClickTime - lastClickTime
    ldr     r4, =(IO_BANK0_BASE + IO_BANK0_INTR2_OFFSET) @ Load INTR2 register address
    str     r7, [r4]                                    @ Clear pending GPIO 21 request
    pop     {pc}

@ Set data alignment
.align 4

.data
ltime: .word    DFLT_TIME
