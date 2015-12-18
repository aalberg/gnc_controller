#include <string.h>

#include "arm_cm4.h"

#include "command_queue.h"
#include "controller_driver.h"
#include "led.h"
#include "uart.h"

#define NOP10() asm("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop")
#define NOP80() NOP10();NOP10();NOP10();NOP10();NOP10();NOP10();NOP10();NOP10()
#define NOP1U0() NOP10();NOP10();NOP10()
#define NOP3U0() NOP80();NOP80();NOP80();NOP80()
#define NOP1U1() NOP80();NOP10();NOP10()
#define NOP3U1() NOP80();NOP80();NOP80();NOP10()

static CommandQueue* command_queue_ptr;
enum ControllerDriverState controller_driver_state = WAITING;
enum ControllerLineBitPart controller_line_bit_part = NONE;

static ControllerState state_to_transimit;
static uint8_t* state_ptr = (uint8_t*)&state_to_transimit;

static unsigned int timer_us_1 = 0;
//static unsigned int timer_us_2 = 0;
static unsigned int timer_us_3 = 0;
static unsigned int timer_us_4 = 0;
//static unsigned int timer_us_8 = 0;

void InitControllerDriver(CommandQueue* queue_ptr) {
  // Precalculate timer counts for 1-4,8us.
  timer_us_1 = 1;//mcg_clk_hz / 1000000;
  //timer_us_2 = mcg_clk_hz * 2 / 1000000;
  timer_us_3 = 80;//mcg_clk_hz * 3 / 1000000;
  timer_us_4 = mcg_clk_hz * 4 / 1000000;
  //timer_us_8 = mcg_clk_hz * 8 / 1000000;
  
  // Init state.
  command_queue_ptr = queue_ptr;
  controller_driver_state = WAITING;
  memcpy(&state_to_transimit, &empty_state, sizeof(ControllerState));
  
  // Set pin C6 to input and enable the port C falling edge interrupt on pin C6.
  GPIOC_PDDR &= ~(1 << 6);
  PORTC_PCR6 = 0;
  PORTC_PCR6 |= PORT_PCR_MUX(0x1);
  PORTC_PCR6 |= PORT_PCR_IRQC(0xA);
  enable_irq(IRQ(INT_PORTC));
  //NVIC_ENABLE_IRQ(INT_PORTC);
  PORTC_ISFR = 1 << 6;
  
  // Enable PIT
  PIT_MCR = 0x00;
  PIT_TCTRL2 = PIT_TCTRL_TIE_MASK;  // Enable Timer 2 interrupts.
  enable_irq(IRQ(INT_PIT2));
  
  const char startup_msg[] = "\x98\x76\x54";//"Controller Driver Initialized\n";
  UARTWrite(startup_msg, strlen(startup_msg));
}

void EnableControllerDriver() {
  controller_driver_state = WAITING;
  GPIOC_PDDR &= ~(1 << 6);
  PORTC_PCR6 = 0;
  PORTC_PCR6 |= PORT_PCR_MUX(0x1);
  PORTC_PCR6 |= PORT_PCR_IRQC(0xA);
  PORTC_ISFR = 1 << 6;
}

void DisableControllerDriver() {
  controller_driver_state = DISABLED;
  PORTC_PCR6 = 0;
  PIT_TCTRL2 &= ~PIT_TCTRL_TEN_MASK;
}

void StartControllerCommandSend() {
  if (controller_driver_state == DISABLED) {
    EnableControllerDriver();
  }
  controller_driver_state = RECEIVE_CODEWORD;
  PIT_TCTRL2 = 0;
  PIT_LDVAL2 = timer_us_4;
  PIT_TCTRL2 = 3;
}

void PIT2_IRQHandler() {
  static int byte_index = 0;
  static int bit_index = 0;
  static int bit_value;
  
  // Disable edge trigger interrupt.
  PORTC_PCR6 &= ~PORT_PCR_IRQC(0xF);
  // Disable timer 2.
  PIT_TCTRL2 &= ~PIT_TCTRL_TEN_MASK;

  if (controller_driver_state == WAITING) {
    // This shoudn't happen, go back to waiting for the PORTC interrupt.
    GPIOC_PDDR &= ~(1 << 6);
    PORTC_PCR6 = (PORTC_PCR6 & ~PORT_PCR_IRQC(0xF)) | PORT_PCR_IRQC(0xA);
    PIT_TFLG2 |= PIT_TFLG_TIF_MASK;
    return;
  } else if (controller_driver_state == RECEIVE_CODEWORD) {
    // Prepare to start sending.
    controller_driver_state = SENDING;
    byte_index = 0;
    bit_index = 0;
    controller_line_bit_part = FIRST;
    
    // If no state to send, send empty state.
    if (Pop(command_queue_ptr, &state_to_transimit)) {
      memcpy(&state_to_transimit, &empty_state, sizeof(ControllerState));
    }
    
    // Load a 0 on pin C6
    GPIOC_PDOR &= ~(1 << 6);
    // 1us delay before starting to send the command.
    PIT_LDVAL2 = timer_us_1;
    //bit_value = 0;
  } else if (controller_driver_state == SENDING) {
    while (1) {
    //if (controller_line_bit_part == FIRST) {
      // Get value of bit being sent.
      if (byte_index == 9) {
        bit_value = 1;
      } else {
        bit_value = (*(state_ptr + byte_index)) & (1 << (7 - bit_index));
      }
      
      // Set line value;
      controller_line_bit_part = SECOND;
      GPIOC_PDDR |= (1 << 6);
      if (bit_value) {
        //PIT_LDVAL2 = timer_us_1;
        NOP1U1();
      } else {
        //PIT_LDVAL2 = timer_us_3;
        NOP3U0();
      }
    //} else if (controller_line_bit_part == SECOND) {
      // Set line value;
      controller_line_bit_part = FIRST;
      GPIOC_PDDR &= ~(1 << 6);
      if (bit_value) {
        //PIT_LDVAL2 = timer_us_3;
        NOP3U1();
      } else {
        //PIT_LDVAL2 = timer_us_1;
        NOP1U0();
      }
      
      // Increment bit index.
      bit_index ++;
      if (bit_index > 7) {
        byte_index ++;
        bit_index = 0;
      }
      
      // Check if done sending.
      if (byte_index == 8 && bit_index == 1) {
        controller_driver_state = WAITING;
        GPIOC_PDDR &= ~(1 << 6);
        PORTC_PCR6 = (PORTC_PCR6 & ~PORT_PCR_IRQC(0xF)) | PORT_PCR_IRQC(0xA);
        PIT_TFLG2 |= PIT_TFLG_TIF_MASK;
        return;
      }
    //}
    }
  }
  
  // Clear the interrupt flag.
  PIT_TFLG2 |= PIT_TFLG_TIF_MASK;
  // Restart Timer 2.
  PIT_TCTRL2 |= PIT_TCTRL_TEN_MASK;
}

// For testing command word detection.
/*void PIT2_IRQHandler() {
  LedOff(2);
  PIT_TCTRL2 &= ~PIT_TCTRL_TEN_MASK;
  PIT_TFLG2 |= PIT_TFLG_TIF_MASK;
}*/

void PORTC_IRQHandler() {
  if ((PORTC_ISFR & (1 << 6)) == 0) {
    return;
  }
  
  LedToggle(1);
  LedOn(2);
  controller_driver_state = RECEIVE_CODEWORD;
  // Keep reseting the timer until the codeword is done.
  PIT_TCTRL2 &= ~PIT_TCTRL_TEN_MASK;  // Disable timer 2.
  PIT_LDVAL2 = timer_us_4;  // Setup timer 2 for 8us.
  PIT_TCTRL2 |= PIT_TCTRL_TEN_MASK;  // Start Timer 2.
  
  // Clear interrupt flag.
  PORTC_ISFR = 1 << 6;
}