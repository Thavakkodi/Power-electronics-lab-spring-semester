//#############################################################################
//
// FILE:   adc_ex1_soc_epwm.c
//
// TITLE:  ADC ePWM Triggering
//
//! \addtogroup bitfield_example_list
//! <h1>ADC ePWM Triggering</h1>
//!
//! This example sets up ePWM1 to periodically trigger a conversion on ADCA.
//!
//! \b External \b Connections \n
//!  - A1 should be connected to a signal to convert
//!
//! \b Watch \b Variables \n
//! - \b adcAResults - A sequence of analog-to-digital conversion samples from
//!   pin A1. The time between samples is determined based on the period
//!   of the ePWM timer.
//!
//
//#############################################################################
//
//
// 
// C2000Ware v6.00.01.00
//
// Copyright (C) 2024 Texas Instruments Incorporated - http://www.ti.com
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions 
// are met:
// 
//   Redistributions of source code must retain the above copyright 
//   notice, this list of conditions and the following disclaimer.
// 
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the 
//   documentation and/or other materials provided with the   
//   distribution.
// 
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// $
//#############################################################################

//
// Included Files
//
#include "f28x_project.h"

//
// Defines
//
#define RESULTS_BUFFER_SIZE     256

//
// Globals
//
uint16_t adcAResults[RESULTS_BUFFER_SIZE];   // Buffer for results
uint16_t index;                              // Index into result buffer
volatile uint16_t bufferFull;                // Flag to indicate buffer is full

// my globals
float32_t duty_cycle = 0.5;
const float32_t pwm_freq = 200e3;
const uint16_t waveform_type = TB_COUNT_UPDOWN;
volatile float32_t Vout;
volatile float32_t Vin;
volatile float32_t IL;
// ADC voltage divider values
const float32_t R1 = 150000;
const float32_t R2 = 100000;
const float32_t Vfs = 3.3;
const uint32_t n_ADC = 12;


//
// Function Prototypes
//
void initADC(void);
void initEPWM(void);
void initADCSOC(void);
__interrupt void adcA1ISR(void);

//
// Main
//
void main(void)
{
    //
    // Initialize device clock and peripherals
    //
    InitSysCtrl();

    //
    // Initialize GPIO
    //
    InitGpio();

    //
    // Disable CPU interrupts
    //
    DINT;

    //
    // Initialize the PIE control registers to their default state.
    // The default state is all PIE interrupts disabled and flags
    // are cleared.
    //
    InitPieCtrl();

    //
    // Disable CPU interrupts and clear all CPU interrupt flags:
    //
    IER = 0x0000;
    IFR = 0x0000;

    //
    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    //
    InitPieVectTable();

    //
    // Map ISR functions
    //
    EALLOW;
    PieVectTable.ADCA1_INT = &adcA1ISR;     // Function for ADCA interrupt 1
    EDIS;

    //
    // Configure the ADC and power it up
    //
    initADC();

    //
    // Configure the ePWM
    //
    initEPWM();

    //
    // Setup the ADC for ePWM triggered conversions on channel 1
    //
    initADCSOC();

    //
    // Enable global Interrupts and higher priority real-time debug events:
    //
    IER |= M_INT1;  // Enable group 1 interrupts

    EINT;           // Enable Global interrupt INTM
    ERTM;           // Enable Global realtime interrupt DBGM

    //
    // Initialize results buffer
    //
    for(index = 0; index < RESULTS_BUFFER_SIZE; index++)
    {
        adcAResults[index] = 0;
    }

    index = 0;
    bufferFull = 0;

    //
    // Enable PIE interrupt
    //
    PieCtrlRegs.PIEIER1.bit.INTx1 = 1;

    //
    // Sync ePWM
    //
    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1;

    //
    // Take conversions indefinitely in loop
    //
    while(1)
    {

    }
}

//
// initADC - Function to configure and power up ADCA.
//
void initADC(void)
{
    //
    // Setup VREF as internal
    //
    SetVREF(ADC_ADCA, ADC_INTERNAL, ADC_VREF3P3);

    EALLOW;

    //
    // Set ADCCLK divider to /4
    //
    AdcaRegs.ADCCTL2.bit.PRESCALE = 6;

    //
    // Set pulse positions to late
    //
    AdcaRegs.ADCCTL1.bit.INTPULSEPOS = 1;

    //
    // Power up the ADC and then delay for 1 ms
    //
    AdcaRegs.ADCCTL1.bit.ADCPWDNZ = 1;
    EDIS;

    DELAY_US(1000);
}

//
// initEPWM - Function to configure ePWM1 to generate the SOC.
//
void initEPWM(void)
{
    EALLOW;

    // Lab 1: generate complementary PWM waveforms on ePWM7A and ePWM7b

    EPwm7Regs.TBCTL.bit.CLKDIV = TB_CLOCK_DIV1;
    EPwm7Regs.TBCTL.bit.HSPCLKDIV = TB_HSDIV1;

    // EPwm7Regs.TBPRD = 375; 
    if (waveform_type == TB_COUNT_UPDOWN) 
    {
        EPwm7Regs.TBPRD = (int) ((150e6 / pwm_freq) / 2);
    }
    else {EPwm7Regs.TBPRD = 150e6 / pwm_freq;} // Assuming 150MHz clk

    EPwm7Regs.CMPA.bit.CMPA = (int) (duty_cycle * EPwm7Regs.TBPRD);   // Set ePWM7A duty count

    EPwm7Regs.CMPB.bit.CMPB = (int) (duty_cycle * EPwm7Regs.TBPRD);   // Set ePWM7B duty count


    EPwm7Regs.TBCTL.bit.CTRMODE = waveform_type;

    switch (waveform_type) {
        case TB_COUNT_UP: // maybe
            EPwm7Regs.AQCTLA.bit.CAD = AQ_NO_ACTION;
            EPwm7Regs.AQCTLA.bit.CAU = AQ_CLEAR;
            EPwm7Regs.AQCTLA.bit.ZRO = AQ_NO_ACTION;
            EPwm7Regs.AQCTLA.bit.PRD = AQ_SET;

            EPwm7Regs.AQCTLB.bit.CAD = AQ_NO_ACTION;
            EPwm7Regs.AQCTLB.bit.CAU = AQ_SET;
            EPwm7Regs.AQCTLB.bit.ZRO = AQ_NO_ACTION;
            EPwm7Regs.AQCTLB.bit.PRD = AQ_CLEAR;
            break;
        case TB_COUNT_DOWN: // maybe
            EPwm7Regs.AQCTLA.bit.CAD = AQ_SET;
            EPwm7Regs.AQCTLA.bit.CAU = AQ_NO_ACTION;
            EPwm7Regs.AQCTLA.bit.ZRO = AQ_CLEAR;
            EPwm7Regs.AQCTLA.bit.PRD = AQ_NO_ACTION;

            EPwm7Regs.AQCTLB.bit.CAD = AQ_CLEAR;
            EPwm7Regs.AQCTLB.bit.CAU = AQ_NO_ACTION;
            EPwm7Regs.AQCTLB.bit.ZRO = AQ_SET;
            EPwm7Regs.AQCTLB.bit.PRD = AQ_NO_ACTION;
            break;
        case TB_COUNT_UPDOWN:
            EPwm7Regs.AQCTLA.bit.CAD = AQ_SET;
            EPwm7Regs.AQCTLA.bit.CAU = AQ_CLEAR;
            EPwm7Regs.AQCTLA.bit.ZRO = AQ_NO_ACTION;
            EPwm7Regs.AQCTLA.bit.PRD = AQ_NO_ACTION;

            EPwm7Regs.AQCTLB.bit.CAD = AQ_CLEAR;
            EPwm7Regs.AQCTLB.bit.CAU = AQ_SET;
            EPwm7Regs.AQCTLB.bit.ZRO = AQ_NO_ACTION;
            EPwm7Regs.AQCTLB.bit.PRD = AQ_NO_ACTION;
            break;
        case TB_FREEZE:
            EPwm7Regs.AQCTLA.bit.CAD = AQ_NO_ACTION;
            EPwm7Regs.AQCTLA.bit.CAU = AQ_NO_ACTION;
            EPwm7Regs.AQCTLA.bit.ZRO = AQ_NO_ACTION;
            EPwm7Regs.AQCTLA.bit.PRD = AQ_NO_ACTION;

            EPwm7Regs.AQCTLB.bit.CAD = AQ_NO_ACTION;
            EPwm7Regs.AQCTLB.bit.CAU = AQ_NO_ACTION;
            EPwm7Regs.AQCTLB.bit.ZRO = AQ_NO_ACTION;
            EPwm7Regs.AQCTLB.bit.PRD = AQ_NO_ACTION;
            break;

    }

        // Adding deadtime:
        EPwm7Regs.DBCTL.bit.IN_MODE = 0b00; // use PWMxA
        EPwm7Regs.DBCTL.bit.POLSEL = 0b10; // invert PWMxB
        EPwm7Regs.DBCTL.bit.OUT_MODE = 0b11; // both falling and rising edge delay

        EPwm7Regs.DBFED.bit.DBFED = 15; // assuming 150MHz clk, 100ns deadtime
        EPwm7Regs.DBRED.bit.DBRED = 15;

    // init GPIO for ePWM7A and B
    // GPAGMUX1 is upper 2 bits, GPAMUX1 is lower 2, 0001 is ePWM
    GpioCtrlRegs.GPAGMUX1.bit.GPIO12 = 0b00;
    GpioCtrlRegs.GPAMUX1.bit.GPIO12 = 0b01;

    GpioCtrlRegs.GPAGMUX1.bit.GPIO13 = 0b00;
    GpioCtrlRegs.GPAMUX1.bit.GPIO13 = 0b01;

    // trigger ADC SOCs
    EPwm7Regs.ETSEL.bit.SOCAEN = 0b1;
    EPwm7Regs.ETSEL.bit.SOCASEL = 0b011;

    EPwm7Regs.ETPS.bit.SOCAPRD = ET_1ST;
    EPwm7Regs.ETPS.bit.INTPRD = ET_1ST;

    EDIS;
}

//
// initADCSOC - Function to configure ADCA's SOC0 to be triggered by ePWM1.
//
void initADCSOC(void)
{
    //
    // Select the channels to convert and the end of conversion flag
    //
    EALLOW;

    AdcaRegs.ADCSOC0CTL.bit.CHSEL = 1;     // SOC0 will convert pin A1
                                           // 0:A0  1:A1  2:A2  3:A3
                                           // 4:A4   5:A5   6:A6   7:A7
                                           // 8:A8   9:A9   A:A10  B:A11
                                           // C:A12  D:A13  E:A14  F:A15
    AdcaRegs.ADCSOC0CTL.bit.ACQPS = 15;     // Sample window is 100ns (assuming 150MHz)
    AdcaRegs.ADCSOC0CTL.bit.TRIGSEL = 0x11;   // Trigger on ePWM7 SOCA

    AdcaRegs.ADCSOC1CTL.bit.CHSEL = 4;     // SOC01 will convert pin A4
    AdcaRegs.ADCSOC1CTL.bit.ACQPS = 15;     // Sample window is 100ns (assuming 150MHz)
    AdcaRegs.ADCSOC1CTL.bit.TRIGSEL = 0x11;   // Trigger on ePWM7 SOCA

    AdcaRegs.ADCSOC2CTL.bit.CHSEL = 0xC;     // SOC01 will convert pin A12
    AdcaRegs.ADCSOC2CTL.bit.ACQPS = 15;     // Sample window is 100ns (assuming 150MHz)
    AdcaRegs.ADCSOC2CTL.bit.TRIGSEL = 0x11;   // Trigger on ePWM7 SOCA

    AdcaRegs.ADCINTSEL1N2.bit.INT1SEL = 2; // End of SOC2 will set INT1 flag
    AdcaRegs.ADCINTSEL1N2.bit.INT1E = 1;   // Enable INT1 flag
    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; // Make sure INT1 flag is cleared

    EDIS;
}

//
// adcA1ISR - ADC A Interrupt 1 ISR
//
__interrupt void adcA1ISR(void)
{
    // Clear the interrupt flag
    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;

    // back-calculate Vcc using ADC unisgned int
    Vout = AdcaResultRegs.ADCRESULT0 * (Vfs/(4096-1)) * ((R1+R2)/R1);
    Vin = AdcaResultRegs.ADCRESULT1 * (Vfs/(4096-1)) * ((R1+R2)/R1);
    IL = AdcaResultRegs.ADCRESULT2 * (Vfs/(4096-1));

    // Acknowledge the interrupt
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

//
// End of File
//
