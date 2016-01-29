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
static int edges_since_send = 0;
static int short_command_num = 0;
static int bytes_to_send = 0;

static ControllerState state_to_transimit;
static uint8_t calibration_data[10] = {0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x02, 0x02};
static uint8_t* state_ptr = (uint8_t*)&state_to_transimit;

static unsigned int timer_bit = 0;

void InitControllerDriver(CommandQueue* queue_ptr) {
  // Precalculate timer counts for end of command word detection.
  timer_bit = mcg_clk_hz * 3 / 1250000;
  
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
  PORTC_ISFR = 1 << 6;
  
  // Enable PIT
  PIT_MCR = 0x00;
  // Enable Timer 2 interrupts.
  PIT_TCTRL2 = PIT_TCTRL_TIE_MASK;
  enable_irq(IRQ(INT_PIT2));
  
  //const char startup_msg[] = "\x98\x76\x54";//"Controller Driver Initialized\n";
  //UARTWrite(startup_msg, strlen(startup_msg));
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
  PIT_LDVAL2 = timer_bit;
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

  // Small optimization to check this first.
  if (controller_driver_state == RECEIVE_CODEWORD) {
    // Prepare to start sending.
    byte_index = 0;
    bit_index = 0;
    
    // Messy way to keep track of what we're supposed to be sending back since
    // we don't actually read what the wii sends us.
    if (edges_since_send < 16) {
      // Request controller ID.
      if (short_command_num == 0) {
        state_ptr = (uint8_t*)&state_to_transimit;
        state_ptr[0] = 0x09;
        state_ptr[1] = 0x00;
        state_ptr[2] = 0x00;
        bytes_to_send = 3;
        short_command_num = 1;
      // Get calibration data.
      } else {
        state_ptr = calibration_data;
        bytes_to_send = 10;
        short_command_num = 0;
      }
    // Poll controller status.
    } else {
      // If no state to send, send empty state.
      state_ptr = (uint8_t*)&state_to_transimit;
      if (Pop(command_queue_ptr, &state_to_transimit) == -1) {
        memcpy(&state_to_transimit, &empty_state, sizeof(ControllerState));
      }
      bytes_to_send = 8;
    }
    
    // Load a 0 on pin C6.
    GPIOC_PDOR &= ~(1 << 6);
    
    // Start sending immediately.
    while (1) {
      // Get value of bit being sent.
      if (byte_index == bytes_to_send) {
        bit_value = 1;
      } else {
        bit_value = (*(state_ptr + byte_index)) & (1 << (7 - bit_index));
      }
      
      // Set line value.
      GPIOC_PDDR |= (1 << 6);
      if (bit_value) {
        NOP1U1();
      } else {
        NOP3U0();
      }

      GPIOC_PDDR &= ~(1 << 6);
      if (bit_value) {
        NOP3U1();
      } else {
        NOP1U0();
      }
      
      // Increment bit index.
      bit_index ++;
      if (bit_index > 7) {
        byte_index ++;
        bit_index = 0;
      }
      
      // Check if done sending.
      if (byte_index == bytes_to_send && bit_index == 1) {
        controller_driver_state = WAITING;
        GPIOC_PDDR &= ~(1 << 6);
        PORTC_PCR6 = (PORTC_PCR6 & ~PORT_PCR_IRQC(0xF)) | PORT_PCR_IRQC(0xA);
        PIT_TFLG2 |= PIT_TFLG_TIF_MASK;
        edges_since_send = 0;
        return;
      }
    }
  } else if (controller_driver_state == WAITING || controller_driver_state == DISABLED) {
    // Go back to waiting for the PORTC interrupt.
    GPIOC_PDDR &= ~(1 << 6);
    PORTC_PCR6 = (PORTC_PCR6 & ~PORT_PCR_IRQC(0xF)) | PORT_PCR_IRQC(0xA);
    PIT_TFLG2 |= PIT_TFLG_TIF_MASK;
    return;
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
  
  edges_since_send ++;
  LedToggle(1);
  LedOn(2);
  controller_driver_state = RECEIVE_CODEWORD;
  // Keep reseting the timer until the codeword is done.
  PIT_TCTRL2 &= ~PIT_TCTRL_TEN_MASK;  // Disable timer 2.
  PIT_LDVAL2 = timer_bit;  // Setup timer 2 ~1 bit length.
  PIT_TCTRL2 |= PIT_TCTRL_TEN_MASK;  // Start Timer 2.
  
  // Clear interrupt flag.
  PORTC_ISFR = 1 << 6;
}