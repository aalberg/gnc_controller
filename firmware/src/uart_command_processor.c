#include <string.h>

#include "command_queue.h"
#include "uart.h"
#include "uart_command_processor.h"

static CommandQueue* command_queue_ptr;

void InitUartCommandProcessor(CommandQueue* queue_ptr) {
  command_queue_ptr = queue_ptr;
  
  const char startup_msg[] = "\xFE\xDC\xBA";//"Uart Initialized\n";
  UARTInit(0, 115200);
  UARTWrite(startup_msg, strlen(startup_msg));
}

int CommandProcessorPoll() {
  if (UARTAvail() != 0) {
    return ReceiveData();
  }
  return -1;
}

// Recieve and process data from the uart.
int ReceiveData() {
  ControllerState state;
  char c;
  int ret = 0;
  UARTRead(&c, 1);
  
  switch(c & 0xF0) {
    case 0x00:
      UARTRead((void *)&state, 8);
      ret = Push(command_queue_ptr, &state);
    case 0x10:
      UARTRead((void *)&state, 8);
      ret = OverwriteState(command_queue_ptr, &state, c & 0x0F);
    case 0x20:
      ClearQueue(command_queue_ptr);
      ret = 1;
  }
  c = ret & 0xFF;
  UARTWrite(&c, 1);
  return ret;
}