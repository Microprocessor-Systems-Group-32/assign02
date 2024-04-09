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
.equ    DFLT_TIME, 0x0                                  @ Specify the default time
.equ    DFLT_ALARM_RUN, 0                               @ Specify the default alarm run state
.equ    DFLT_ALARM_TIME, 1000000                        @ Specify the default alarm timeout

.equ    LONG_PRESS, 250000                              @ Specify the long press time
.equ    LONG_PRESS_SPACE, 1000000                       @ Specify the long press space time

.equ    GPIO_BTN_MSK_RISE, 0x00800000                   @ Bit-21 for falling-edge event on GP21
.equ    GPIO_BTN_MSK_FALL, 0x00400000                   @ Bit-20 for rising-edge event on GP21

.equ    GPIO_BTN_PIN,   21                              @ Specify pin for GPIO button 21
.equ    GPIO_DIR_IN,    0                               @ Specify input direction for a GPIO pin
.equ    GPIO_DIR_OUT,   1                               @ Specify output direction for a GPIO pin

.equ    GPIO_ISR_OFFSET, 0x74                           @ GPIO is int #13 (vector table entry 29)
.equ    ALRM_ISR_OFFSET, 0x40                           @ ALARM0 is int #0 (vector table entry 16)

@ Entry point to the ASM portion of the program
main_asm:
    push    {lr}
    bl      init_gpio_buttons                           @ Call subroutine to initialise GPIO buttons
    bl      install_irq_gpio                            @ Call subroutine to install GPIO interrupt handler
    bl      install_irq_0                               @ Call subroutine to install ALARM0 interrupt handler
    wfi
    bl      set_alarm_timing                            @ Call subroutine to set alarm delay
    pop     {pc}

@ Initialise the GPIO 21 pin
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

@ Subroutine to install an interrupt handler for Timer0
install_irq_0:
    push    {r4, r5, lr}                                @ Preserve registers (incl. LR)
    ldr     r4, =(PPB_BASE + M0PLUS_VTOR_OFFSET)        @ Load address of VTOR into r6
    ldr     r4, [r4]                                    @ Load VTOR
    ldr     r5, =irq_0_isr                              @ Load address of the ISR handler into r4
    str     r5, [r4, ALRM_ISR_OFFSET]                   @ Store the handler into the alarm ISR
    ldr     r0, =0x1                                    @ Load appropriate value to write TIMER_IRQ_0 bit in NVIC
    bl      enable_interrupt                            @ Call subroutine to enable alarm interrupt
    pop     {r4, r5, pc}                                @ Restore registers and exit subroutine

@ Helper Subroutine to set the ALARM0 interrupt delay
set_alarm_timing:
    push    {r4-r5, lr}                                 @ Preserve registers (incl. LR)
    ldr     r4, =(TIMER_BASE + TIMER_TIMELR_OFFSET)     @ Get lower 32-bits of timer register
    ldr     r4,[r4]                                     @ Load bits into r4
    ldr     r5, =ltimer                                 @ Load address of the timing delay in shared memory
    ldr     r5, [r5]                                    @ Load the timing delay
    add     r4, r4, r5                                  @ Add delays together
    ldr     r5, =(TIMER_BASE + TIMER_ALARM0_OFFSET)     @ Get ALARM0 control register    
    str     r4, [r5]                                    @ Store new delay
    ldr     r4, =0x1                                    @ Set appropriate value to enable timer (entry 0)
    ldr     r5, =(TIMER_BASE + TIMER_INTE_OFFSET)       @ Load INTE register address
    str     r4, [r5]                                    @ Enable timer
    pop     {r4-r5, pc}                                 @ Restore registers and exit subroutine

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

@ Timer interrupt service handler routine
.thumb_func
irq_0_isr:
    push    {r4-r6, lr}                                 @ Restore registers
    ldr     r4, =(TIMER_BASE + TIMER_INTR_OFFSET)       @ Load address of TIMER raw interrupts register
    ldr     r5, =0x1                                    @ Load appropriate value to write TIMER0 bit
    str     r5, [r4]                                    @ Acknowledge interrupt as handled (by writing the TIMER0 bit)
    ldr     r4, =lalarmRun                              @ Load alarm bool address
    ldr     r5, [r4]                                    @ Load alarm bool
    cmp     r5, #1                                      @ Check if alarm is running
    bne     irq_space                                   @ If not, return
    ldr     r5, =#0                                     @ Set alarm bool to false
    str     r5, [r4]                                    @ Store the alarm bool
    movs    r0, #3                                      @ Set param to End-Of-Input character
    movs    r1, #0                                      @ Set next char param to '\0'
    bl      add_input                                   @ Call C function to add char to input
    b       finish_irq0
irq_space:
    ldr     r5, =#1                                     @ Set alarm bool to true
    str     r5, [r4]                                    @ Store the alarm bool
    movs    r0, #2                                      @ Set param to End-Of-Input character
    movs    r1, #0                                      @ Set next char param to '\0'
    bl      add_input                                   @ Call C function to add char to input
    bl      set_alarm_timing                            @ Enable the timer interrupt again
finish_irq0:
    pop     {r4-r6, pc}                                @ Restore registers

@ GPIO interrupt service handler routine
.thumb_func
gpio_isr:
    push    {lr}                                        @ Preserve LR
    bl      arm_watchdog_update                         @ Call subroutine to update the watchdog
    ldr     r4, =(IO_BANK0_BASE + IO_BANK0_INTR2_OFFSET) @ Load INTR2 register address
    ldr     r5, =GPIO_BTN_MSK_FALL                      @ Load value to write to GPIO 21 bit 
    ldr     r6, [r4]                                    @ Load the INTR2 register
    ands    r6, r5, r6                                  @ Mask the falling-edge bit
    lsrs    r6, #22                                     @ Shift right
    cmp     r6, #1                                      @ If falling-edge
    bne     rise_edge                                   @ Else, it's rising edge
fall_edge:
    @ It's a falling-edge event
    ldr     r7, =GPIO_BTN_MSK_FALL                      @ Load value to write to GPIO 21 bit
    ldr     r5, =(TIMER_BASE + TIMER_TIMELR_OFFSET)     @ Load the address of the timer (lwr 32 bits) register
    ldr     r5, [r5]                                    @ Load the lwr 32 bits of the time
    ldr     r6, =ltimeDiff                              @ Load the address of the timer var
    str     r5, [r6]                                    @ Store the current time in the var
    b       gpio_isr_clear                              @ Continue  
rise_edge:
    @ It's a rising-edge event
    ldr     r7, =GPIO_BTN_MSK_RISE                      @ Load value to write to GPIO 21 bit
    movs    r1, #1                                      @ Set that it's "time button IS pressed"
    movs    r1, #0                                      @ Set that it's "time button NOT pressed"
    ldr     r4, =ltimeDiff                              @ Load the address of the timer var
    ldr     r4, [r4]                                    @ Load the timer var
    ldr     r5, =(TIMER_BASE + TIMER_TIMELR_OFFSET)     @ Load the address of the timer (lwr 32 bits) register
    ldr     r5, [r5]                                    @ Load the lwr 32 bits of the time
    ldr     r6, =ltimeDiff                              @ Load the address of the timer var
    str     r5, [r6]                                    @ Store the current time in the var
    subs    r0, r5, r4                                  @ diff = currClickTime - lastClickTime
    ldr     r1, =LONG_PRESS                             @ Load the long press time
    cmp     r0, r1                                      @ Compare the difference with the long press time
    blt     dot                                         @ If less than long press time, it's a dot   
    movs    r0, #1                                      @ Set param to End-Of-Input character 
    movs    r1, #0                                      @ Set next char param to '\0'
    bl      add_input                                   @ Call C function to add char to input
    b       gpio_isr_clear                              @ Continue
dot:
    movs    r0, #0                                      @ Set param to '.' character 
    movs    r1, #0                                      @ Set next char param to '\0'
    bl      add_input                                   @ Call C function to add char to input
gpio_isr_clear:
    ldr     r5, =lalarmRun                              @ Load the address of the alarm bool
    ldr     r6, =#0                                     @ Load the value to write to the alarm bool
    str     r6, [r5]                                    @ Clear the alarm bool
    bl      set_alarm_timing                            @ Call subroutine to set the alarm timing
    ldr     r4, =(IO_BANK0_BASE + IO_BANK0_INTR2_OFFSET) @ Load INTR2 register address
    str     r7, [r4]                                    @ Clear pending GPIO 21 request
    pop     {pc}                                        @ Restore register

@ Set data alignment
.align 4

.data
ltimeDiff: .word    DFLT_TIME
ltimer:    .word    DFLT_ALARM_TIME
lalarmRun: .word    DFLT_ALARM_RUN
