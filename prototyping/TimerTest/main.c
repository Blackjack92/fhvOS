/*
 *         Application Use Case:
 *             The application demonstrates DMTimer in Autoreload mode
 *             of operation. In the example for every overflow of DMTimer
 *             the counter register is reloaded with contents of overflow
 *             register. This sequence is continued 10 times and at each
 *             overflow a decrementing value is printed on the serial
 *             console showcasing the DMTimer as a down counter.
 *
 *         Running the example:
 *             On execution, the example will count down from 9 - 0 and stop.
 *             The time interval between each count is approximate to 700ms.
 */

/*	Timer Notes
 *
 * 	DMTIMER0: 	start address: 		0x44E0_5000
 * 			 	end address:		0x44E0_5FFF
 * 				clock domain:		PD_WKUP_TIMER0_GCLK
 * 				reset signal:		WKUP_DOM_RST_N
 *
 *
 * 	Offset:		Registers:			Full Name:
 *
 *	00h			TIDR				Timer Identification Register
 * 	10h 		TIOCP_CFG			Timer OCP Configuration Register
 * 	3Ch			TCRR				Timer Counter Register
 * 	40h			TLDR				Timer Load Register
 * 	44h			TTGR				Timer Trigger Register
 * 	38h			TCLR				Timer Control Register
 * 	48h			TWPS				Timer Write Posting Bits Register
 * 	28h			IRQSTATUS			Timer Status Register
 * 	2Ch			IRQENABLE_SET		Timer Interrupt Enable Set Register
 * 	30h			IRQENABLE_CLR		Timer Interrupt Enable Clear Register
 * 	50h 		TCAR1 				Timer Capture Register 1
 * 	58h 		TCAR2 				Timer Capture Register 2
 *
 *
 * 	Timer Mode Functionality:
 *
 *	upward counter
 *	TCLR (ST bit) start/stops at any time
 *	TCRR load 1.) directly or 2.) over TLDR by TTGR
 *	On Shote Mode: 		(TCLR AR bit = 0)
 *	Auto-Reload Mode: 	(TCLR AR bit = 1), TCRR reloaded with TLDR
*/

#include "soc_AM335x.h"
#include "beaglebone.h"
#include "interrupt.h"
#include "error.h"
#include "uartStdio.h"
#include "uart_irda_cir.h"
#include "misc.h"
#include "dmtimer.h"
#include "consoleUtils.h"
#include "hw_cm_per.h"


// timer defines
#define TIMER_INITIAL_COUNT             (0xFF000000u)
#define TIMER_RLD_COUNT                 (0xFF000000u)
#define CM_DPLL_CLKSEL_TIMER2_CLK_CLKSEL_CLK_M_OSC   (0x1u)
#define CM_DPLL_CLKSEL_TIMER2_CLK_CLKSEL   (0x00000003u)
#define CM_DPLL_CLKSEL_TIMER2_CLK   (0x8)
#define DMTimerWaitForWrite(reg, baseAdd)   \
            if(HWREG(baseAdd + DMTIMER_TSICR) & DMTIMER_TSICR_POSTED)\
            while((reg & DMTimerWritePostedStatusGet(baseAdd)));



// registers
#define INTC_MIR0						(0x84)
#define INTC_MIR_CLEAR0					(0x88)
#define INTC_MIR_SET0					(0x8C)
#define INTC_ISR_SET0					(0x90)
#define INTC_ISR_CLEAR0 				(0x94)
#define INTC_PENDING_IRQ0				(0x98)
#define INTC_PENDING_FIQ0				(0x9C)
#define INTC_ITR1						(0xA0)
#define INTC_MIR1						(0xA4)
#define INTC_MIR_CLEAR1					(0xA8)
#define INTC_MIR_SET1					(0xAC)
#define INTC_ISR_SET1					(0xB0)
#define INTC_ISR_CLEAR1 				(0xB4)
#define INTC_PENDING_IRQ1				(0xB8)
#define INTC_PENDING_FIQ1				(0xBC)
#define INT_ITR2						(0xC0)
#define INTC_MIR2						(0xC4)
#define INTC_MIR_CLEAR2					(0xC8)
#define INTC_MIR_SET2					(0xCC)
#define INTC_ISR_SET2					(0xD0)
#define INTC_ISR_CLEAR2 				(0xD4)
#define INTC_PENDING_IRQ2				(0xD8)
#define INTC_PENDING_FIQ2				(0xDC)
#define INTC_ITR3						(0xE0)
#define INTC_MIR3						(0xE4)
#define INTC_MIR_CLEAR3					(0xE8)
#define INTC_MIR_SET3					(0xEC)
#define INTC_ISR_SET3					(0xF0)
#define INTC_ISR_CLEAR3 				(0xF4)
#define INTC_PENDING_IRQ3				(0xF8)
#define INTC_PENDING_FIQ3				(0xFC)
#define INTC_PROTECTION					(0x4C)

// commands
#define INTC_MIR_CLEAR_BITMASK			(0x0)
#define INTC_MIR_SET_BITMASK			(0x1)
#define INT_MODE_IRQ					(0x0)
#define INT_MODE_FIQ					(0x1)
#define INTC_HIGHEST_PRIORITY			(0x0)
#define INTC_LOWEST_PRIORITY			(0x7F)
#define INTC_PRIORITY_DISABLED			(0xFF)
#define INTC_IDLE_SYNCFREE				(0x0)
#define INTC_IDLE_SYNCAUTO				(0x1)
#define INTC_IDLE_FUNCAUTO				(0x0)
#define INTC_IDLE_FUNCFREE				(0x1)
#define INTC_IDLE_SUNCAUTO_FUNCFREE		(0x3)

void clearInterruptMask(unsigned int intcMirClearRegister);
void setInterruptMask(unsigned int intcMirSetRegister);
void setIntPriorityAndMode(int intcILR, unsigned int intPriority, unsigned int intMode);
void intcIdleSettings(unsigned int idleMode);
void setIntControllerAutoIdle();
void setIdleSyncMode(unsigned int syncMode);
void setIdleFuncMode(unsigned int funcMode);

static void DMTimerAintcConfigure(void);
static void DMTimerSetUp(void);
static void DMTimerIsr(void);
static volatile unsigned int cntValue = 10;
static volatile unsigned int flagIsr = 0;

void globalInterruptEnable(void);


int main(void) {

	/* This function will enable clocks for the DMTimer2 instance */
	    DMTimer2ModuleClkConfig();

	    /* Initialize the UART console */
	    ConsoleUtilsInit();

	    /* Select the console type based on compile time check */
	    ConsoleUtilsSetType(CONSOLE_UART);

	    /* Enable IRQ in CPSR */
	    IntMasterIRQEnable();

	    /* Register DMTimer2 interrupts on to AINTC */
	    DMTimerAintcConfigure();

	    /* Perform the necessary configurations for DMTimer */
	    DMTimerSetUp();

	    /* Enable the DMTimer interrupts */
	    DMTimerIntEnable(SOC_DMTIMER_2_REGS, DMTIMER_INT_OVF_EN_FLAG);

	    ConsoleUtilsPrintf("Tencounter: ");

	    /* Start the DMTimer */
	    DMTimerEnable(SOC_DMTIMER_2_REGS);

	    while(cntValue)
	    {
	        if(flagIsr == 1)
	        {
	            ConsoleUtilsPrintf("\b%d",(cntValue - 1));
	            cntValue--;
	            flagIsr = 0;
	        }
	    }

	    /* Stop the DMTimer */
	    DMTimerDisable(SOC_DMTIMER_2_REGS);

	    PRINT_STATUS(S_PASS);

	    /* Halt the program */
	    while(1);
}





void setIntControllerAutoIdle()
{
	HWREG(SOC_AINTC_REGS + INTC_SYSCONFIG) = INTC_SYSCONFIG_AUTOIDLE;

	while (!(HWREG(SOC_AINTC_REGS + INTC_SYSCONFIG) & INTC_SYSCONFIG_AUTOIDLE))
			;
}

void setIntPriorityAndMode(int intControllerILR, unsigned int intPriority, unsigned int intMode)
{

	HWREG(SOC_AINTC_REGS + intControllerILR) = (intPriority << 2) + intMode;

	while(!(HWREG(SOC_AINTC_REGS + intControllerILR) & ((intPriority << 2) + intMode)))
		;
}

void clearInterruptMask(unsigned int intcMirClearRegister)
{
	HWREG(SOC_AINTC_REGS + intcMirClearRegister) = INTC_MIR_CLEAR_BITMASK;

	while(!(HWREG(SOC_AINTC_REGS + intcMirClearRegister) & INTC_MIR_CLEAR_BITMASK))
		;
}

void setInterruptMask(unsigned int intcMirSetRegister)
{
	HWREG(SOC_AINTC_REGS + intcMirSetRegister) = INTC_MIR_SET_BITMASK;

	while(!(HWREG(SOC_AINTC_REGS + intcMirSetRegister) & INTC_MIR_SET_BITMASK))
		;
}

void intcIdleSettings(unsigned int idleMode)
{
	HWREG(SOC_AINTC_REGS + INTC_IDLE) = idleMode;

	while(!(HWREG(SOC_AINTC_REGS + INTC_IDLE) & idleMode))
			;
}








void globalInterruptEnable(void)
{
	__asm("    mrs     r0, CPSR\n\t"
			"    bic     r0, r0, #0x80\n\t"
			"    msr     CPSR_c, r0");
}

/*
** Do the necessary DMTimer configurations on to AINTC.
*/
static void DMTimerAintcConfigure(void)
{
    /* Initialize the ARM interrupt control */
    IntAINTCInit();

    /* Registering DMTimerIsr */
    IntRegister(SYS_INT_TINT2, DMTimerIsr);

    /* Set the priority */
    IntPrioritySet(SYS_INT_TINT2, 0, AINTC_HOSTINT_ROUTE_IRQ);

    /* Enable the system interrupt */
    IntSystemEnable(SYS_INT_TINT2);
}

/*
** Setup the timer for one-shot and compare mode.
*/
static void DMTimerSetUp(void)
{
    /* Load the counter with the initial count value */
    DMTimerCounterSet(SOC_DMTIMER_2_REGS, TIMER_INITIAL_COUNT);

    /* Load the load register with the reload count value */
    DMTimerReloadSet(SOC_DMTIMER_2_REGS, TIMER_RLD_COUNT);

    /* Configure the DMTimer for Auto-reload and compare mode */
    DMTimerModeConfigure(SOC_DMTIMER_2_REGS, DMTIMER_AUTORLD_NOCMP_ENABLE);
}

/*
** DMTimer interrupt service routine. This will send a character to serial
** console.
*/
#pragma INTERRUPT(DMTimerIsr, UDEF)
static void DMTimerIsr(void)
{
    /* Disable the DMTimer interrupts */
    DMTimerIntDisable(SOC_DMTIMER_2_REGS, DMTIMER_INT_OVF_EN_FLAG);

    /* Clear the status of the interrupt flags */
    DMTimerIntStatusClear(SOC_DMTIMER_2_REGS, DMTIMER_INT_OVF_IT_FLAG);

    flagIsr = 1;

    /* Enable the DMTimer interrupts */
    DMTimerIntEnable(SOC_DMTIMER_2_REGS, DMTIMER_INT_OVF_EN_FLAG);
}

/*
#pragma INTERRUPT(udef_handler, UDEF)
interrupt void udef_handler() {
	printf("udef interrupt\n");
}

#pragma INTERRUPT(fiq_handler, FIQ)
interrupt void fiq_handler() {
	printf("fiq interrupt\n");
}


// Is called on any prefetch abort.

#pragma INTERRUPT(pabt_handler, PABT)
interrupt void pabt_handler() {
	printf("pabt interrupt\n");
}
*/

void DMTimer2ModuleClkConfig(void)
{
    HWREG(SOC_CM_PER_REGS + CM_PER_L3S_CLKSTCTRL) =
                             CM_PER_L3S_CLKSTCTRL_CLKTRCTRL_SW_WKUP;

    while((HWREG(SOC_CM_PER_REGS + CM_PER_L3S_CLKSTCTRL) &
     CM_PER_L3S_CLKSTCTRL_CLKTRCTRL) != CM_PER_L3S_CLKSTCTRL_CLKTRCTRL_SW_WKUP);

    HWREG(SOC_CM_PER_REGS + CM_PER_L3_CLKSTCTRL) =
                             CM_PER_L3_CLKSTCTRL_CLKTRCTRL_SW_WKUP;

    while((HWREG(SOC_CM_PER_REGS + CM_PER_L3_CLKSTCTRL) &
     CM_PER_L3_CLKSTCTRL_CLKTRCTRL) != CM_PER_L3_CLKSTCTRL_CLKTRCTRL_SW_WKUP);

    HWREG(SOC_CM_PER_REGS + CM_PER_L3_INSTR_CLKCTRL) =
                             CM_PER_L3_INSTR_CLKCTRL_MODULEMODE_ENABLE;

    while((HWREG(SOC_CM_PER_REGS + CM_PER_L3_INSTR_CLKCTRL) &
                               CM_PER_L3_INSTR_CLKCTRL_MODULEMODE) !=
                                   CM_PER_L3_INSTR_CLKCTRL_MODULEMODE_ENABLE);

    HWREG(SOC_CM_PER_REGS + CM_PER_L3_CLKCTRL) =
                             CM_PER_L3_CLKCTRL_MODULEMODE_ENABLE;

    while((HWREG(SOC_CM_PER_REGS + CM_PER_L3_CLKCTRL) &
        CM_PER_L3_CLKCTRL_MODULEMODE) != CM_PER_L3_CLKCTRL_MODULEMODE_ENABLE);

    HWREG(SOC_CM_PER_REGS + CM_PER_OCPWP_L3_CLKSTCTRL) =
                             CM_PER_OCPWP_L3_CLKSTCTRL_CLKTRCTRL_SW_WKUP;

    while((HWREG(SOC_CM_PER_REGS + CM_PER_OCPWP_L3_CLKSTCTRL) &
                              CM_PER_OCPWP_L3_CLKSTCTRL_CLKTRCTRL) !=
                                CM_PER_OCPWP_L3_CLKSTCTRL_CLKTRCTRL_SW_WKUP);

    HWREG(SOC_CM_PER_REGS + CM_PER_L4LS_CLKSTCTRL) =
                             CM_PER_L4LS_CLKSTCTRL_CLKTRCTRL_SW_WKUP;

    while((HWREG(SOC_CM_PER_REGS + CM_PER_L4LS_CLKSTCTRL) &
                             CM_PER_L4LS_CLKSTCTRL_CLKTRCTRL) !=
                               CM_PER_L4LS_CLKSTCTRL_CLKTRCTRL_SW_WKUP);

    HWREG(SOC_CM_PER_REGS + CM_PER_L4LS_CLKCTRL) =
                             CM_PER_L4LS_CLKCTRL_MODULEMODE_ENABLE;

    while((HWREG(SOC_CM_PER_REGS + CM_PER_L4LS_CLKCTRL) &
      CM_PER_L4LS_CLKCTRL_MODULEMODE) != CM_PER_L4LS_CLKCTRL_MODULEMODE_ENABLE);

    /* Select the clock source for the Timer2 instance. */
    HWREG(SOC_CM_DPLL_REGS + CM_DPLL_CLKSEL_TIMER2_CLK) &=
          ~(CM_DPLL_CLKSEL_TIMER2_CLK_CLKSEL);

    HWREG(SOC_CM_DPLL_REGS + CM_DPLL_CLKSEL_TIMER2_CLK) |=
          CM_DPLL_CLKSEL_TIMER2_CLK_CLKSEL_CLK_M_OSC;

    while((HWREG(SOC_CM_DPLL_REGS + CM_DPLL_CLKSEL_TIMER2_CLK) &
           CM_DPLL_CLKSEL_TIMER2_CLK_CLKSEL) !=
           CM_DPLL_CLKSEL_TIMER2_CLK_CLKSEL_CLK_M_OSC);

    HWREG(SOC_CM_PER_REGS + CM_PER_TIMER2_CLKCTRL) |=
                             CM_PER_TIMER2_CLKCTRL_MODULEMODE_ENABLE;

    while((HWREG(SOC_CM_PER_REGS + CM_PER_TIMER2_CLKCTRL) &
    CM_PER_TIMER2_CLKCTRL_MODULEMODE) != CM_PER_TIMER2_CLKCTRL_MODULEMODE_ENABLE);

    while((HWREG(SOC_CM_PER_REGS + CM_PER_TIMER2_CLKCTRL) &
       CM_PER_TIMER2_CLKCTRL_IDLEST) != CM_PER_TIMER2_CLKCTRL_IDLEST_FUNC);

    while(!(HWREG(SOC_CM_PER_REGS + CM_PER_L3S_CLKSTCTRL) &
            CM_PER_L3S_CLKSTCTRL_CLKACTIVITY_L3S_GCLK));

    while(!(HWREG(SOC_CM_PER_REGS + CM_PER_L3_CLKSTCTRL) &
            CM_PER_L3_CLKSTCTRL_CLKACTIVITY_L3_GCLK));

    while(!(HWREG(SOC_CM_PER_REGS + CM_PER_OCPWP_L3_CLKSTCTRL) &
           (CM_PER_OCPWP_L3_CLKSTCTRL_CLKACTIVITY_OCPWP_L3_GCLK |
            CM_PER_OCPWP_L3_CLKSTCTRL_CLKACTIVITY_OCPWP_L4_GCLK)));

    while(!(HWREG(SOC_CM_PER_REGS + CM_PER_L4LS_CLKSTCTRL) &
           (CM_PER_L4LS_CLKSTCTRL_CLKACTIVITY_L4LS_GCLK |
            CM_PER_L4LS_CLKSTCTRL_CLKACTIVITY_TIMER2_GCLK)));

}
