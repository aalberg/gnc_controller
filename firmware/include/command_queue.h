#ifndef COMMAND_QUEUE_H_
#define COMMAND_QUEUE_H_

#define QUEUE_SIZE 64

#define EMPTY_STATE { \
  0x00, \
  0x80, \
  0x80, \
  0x7F, \
  0x80, \
  0x7F, \
  0x00, \
  0x00, \
}

#include "arm_cm4.h"

typedef struct {
  unsigned char byte0;
  unsigned char byte1;
  unsigned char joy_x;
  unsigned char joy_y;
  unsigned char c_x;
  unsigned char c_y;
  unsigned char l_trig;
  unsigned char r_trig;
} ControllerState;

extern const ControllerState empty_state;

typedef struct {
  int start_index;
  int end_index;
  int size;
  ControllerState arr[QUEUE_SIZE];
  uint8 pop_removes_items;
  uint8 pop_parity;
} CommandQueue;

void InitCommandQueue(CommandQueue* queue);
int IsEmpty(CommandQueue* queue);
int IsFull(CommandQueue* queue);
int Pop(CommandQueue* queue, ControllerState* state);
int Push(CommandQueue* queue, ControllerState* state);
int OverwriteState(CommandQueue* queue, ControllerState* state, int index);
int DeleteAfter(CommandQueue* queue, int index);
int ClearQueue(CommandQueue* queue);

#endif  // COMMAND_QUEUE_H_