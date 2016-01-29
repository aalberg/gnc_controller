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
const ControllerState empty_state = EMPTY_STATE;

void InitCommandQueue(CommandQueue* queue) {
  memset(queue, 0, sizeof(CommandQueue));
  queue->pop_removes_items = 1;
  queue->pop_parity = 0;
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
  if (queue->pop_removes_items) {
    if (queue->pop_parity == 0) {
      queue->pop_parity = 1;
    } else {
      queue->pop_parity = 0;
      queue->start_index = (queue->start_index + 1) % QUEUE_SIZE;
      queue->size --;
    }
  }
  return queue->size;
}

int Push(CommandQueue* queue, ControllerState* state) {
  if (IsFull(queue)) {
    return -1;
  }
  memcpy(&(queue->arr[queue->end_index]), state, sizeof(ControllerState));
  queue->end_index = (queue->end_index + 1) % QUEUE_SIZE;
  queue->size ++;
  return queue->size;
}

int OverwriteState(CommandQueue* queue, ControllerState* state, int index) {
  if (index >= queue->size) {
    return -1;
  }
  memcpy(&(queue->arr[(queue->end_index - index - 1) % QUEUE_SIZE]), state, sizeof(ControllerState));
  return queue->size;
}

int DeleteAfter(CommandQueue* queue, int index) {
  if (index >= queue->size) {
    return -1;
  }
  queue->end_index = (queue->start_index + index + 1) % QUEUE_SIZE;
  queue->size = index + 1;
  return queue->size;
}

int ClearQueue(CommandQueue* queue) {
  queue->start_index = 0;
  queue->end_index = 0;
  queue->size = 0;
  queue->pop_parity = 0;
  return 0;
}