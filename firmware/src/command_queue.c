#include <string.h>

#include "command_queue.h"

/*const ControllerState empty_state = {
  0xA5,
  0x80,
  0xA5,
  0xA5,
  0xA5,
  0xA5,
  0xA5,
  0xA5,
};*/
const ControllerState empty_state = {
  0x00,
  0x80,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
};

void InitCommandQueue(CommandQueue* queue) {
  memset(queue, 0, sizeof(CommandQueue));
}

int IsEmpty(CommandQueue* queue) {
  return queue->size == 0;
}

int IsFull(CommandQueue* queue) {
  return queue->size == QUEUE_SIZE;
}

int Pop(CommandQueue* queue, ControllerState* state) {
  if (IsEmpty(queue)) {
    return -1;
  }
  memcpy(state, &(queue->arr[queue->start_index]), sizeof(ControllerState));
  queue->start_index = (queue->start_index + 1) % QUEUE_SIZE;
  queue->size --;
  return 0;
}

int Push(CommandQueue* queue, ControllerState* state) {
  if (IsFull(queue)) {
    return -1;
  }
  queue->end_index = (queue->end_index + 1) % QUEUE_SIZE;
  memcpy(&(queue->arr[queue->end_index]), state, sizeof(ControllerState));
  queue->size ++;
  return 0;
}

int OverwriteState(CommandQueue* queue, ControllerState* state, int index) {
  if (index >= queue->size) {
    return -1;
  }
  memcpy(&(queue->arr[(queue->start_index + index) % QUEUE_SIZE]), state, sizeof(ControllerState));
  return 0;
}

void ClearQueue(CommandQueue* queue) {
  queue->start_index = 0;
  queue->end_index = 0;
  queue->size = 0;
}