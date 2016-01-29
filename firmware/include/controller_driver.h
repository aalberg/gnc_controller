#ifndef CONTROLLER_DRIVER_H_
#define CONTROLLER_DRIVER_H_

#include "command_queue.h"

enum ControllerDriverState {
  DISABLED,
  WAITING,
  RECEIVE_CODEWORD,
};

extern enum ControllerDriverState controller_driver_state;
extern enum ControllerLineBitPart controller_line_bit_part;

void InitControllerDriver(CommandQueue* queue);
void EnableControllerDriver();
void DisableControllerDriver();
void StartControllerCommandSend();


#endif  // CONTROLLER_DRIVER_H_