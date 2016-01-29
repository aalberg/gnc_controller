// Common includes.
#include "common.h"
#include "arm_cm4.h"

// Module includes.
#include "command_queue.h"
#include "controller_driver.h"
#include "led.h"
#include "uart.h"
#include "uart_command_processor.h"

// Initialize everything that needs to be set up.
void Init(void) {
  // Enable the PIT clock.
  SIM_SCGC3 |= SIM_SCGC3_ADC1_MASK;
  SIM_SCGC6 |= SIM_SCGC6_PIT_MASK | SIM_SCGC6_ADC0_MASK;

  // Turn on PIT.
  PIT_MCR = 0x00;
  // Timer 1
  PIT_LDVAL1 = mcg_clk_hz /* 60;*/>> 2; // setup timer 1 for 4Hz.
  PIT_TCTRL1 = PIT_TCTRL_TIE_MASK; // enable Timer 1 interrupts
  PIT_TCTRL1 |= PIT_TCTRL_TEN_MASK; // start Timer 1

  // Enable interrupts.
  enable_irq(IRQ(INT_PIT1));
  EnableInterrupts
}

int main(void) {
  Init();
  InitLeds();
  CommandQueue queue;
  InitCommandQueue(&queue);
  InitUartCommandProcessor(&queue);
  InitControllerDriver(&queue);
  //char c;
  
  while(1) {
    /*if (UARTAvail() != 0) {
      UARTRead(&c, 1);
      UARTWrite(&c, 1);
    }*/
    CommandProcessorPoll();
    /*if (controller_driver_state == WAITING) {
      LedOn(3);
      LedOff(4);
    } else if (controller_driver_state == RECEIVE_CODEWORD) {
      LedOff(3);
      LedOn(4);
    } else {
      LedOff(3);
      LedOff(4);
    }
    if (GPIOC_PDIR & 1 << 6) {
      LedOn(5);
    } else {
      LedOff(5);
    }*/
  }
  return 0;
}

// Blink the onboard LED to indicate the teensy is running.
void PIT1_IRQHandler() {
  LedToggle(0);
  //StartControllerCommandSend();
  
  // Reset the interrupt flag.
  PIT_TFLG1 |= PIT_TFLG_TIF_MASK;
}
