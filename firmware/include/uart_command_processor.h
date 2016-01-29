#ifndef UART_COMMAND_PROCESSOR_H_
#define UART_COMMAND_PROCESSOR_H_

#include "command_queue.h"

enum CommandProcessorState {
  HANDSHAKE,
  ACTIVE,
};

extern enum CommandProcessorState processor_state;

void InitUartCommandProcessor(CommandQueue* queue);
int CommandProcessorPoll();
int ReceiveData();

#endif  // UART_COMMAND_PROCESSOR_H_