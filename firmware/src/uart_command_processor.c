#include <string.h>

#include "command_queue.h"
#include "led.h"
#include "uart.h"
#include "uart_command_processor.h"

static CommandQueue* command_queue_ptr;
enum CommandProcessorState processor_state = ACTIVE;

static char header[2];
static char cmd;
static char seq;
static char expected_seq = 0;
static ControllerState state = EMPTY_STATE;
static int ret = 0;
static int i = 0;

// Initialize and active the command processor.
void InitUartCommandProcessor(CommandQueue* queue_ptr) {
  command_queue_ptr = queue_ptr;
  
  //const char startup_msg[] = "\xFE\xDC\xBA";//"Uart Initialized\n";
  UARTInit(0, 115200);
  //UARTWrite(startup_msg, strlen(startup_msg));
}

// Poll the UART for a command.
int CommandProcessorPoll() {
  if (UARTAvail() != 0) {
    return ReceiveData();
  }
  return -1;
}

// Recieve and process data from the uart.
int ReceiveData() {
  LedOn(2);
  UARTRead(header, 2);
  
  cmd = header[0] & 0xF0;
  seq = header[0] & 0x0F;
  
  if (seq != expected_seq && cmd != 0xF0) {
    header[0] = 0xF0 | expected_seq;
    ret = 0xFF;
    UARTFlushRead();
  } else {
    header[0] = 0;
    switch(cmd) {
      // Add to queue.
      case 0x00:
        UARTRead((char *)&state, 8);
        ret = Push(command_queue_ptr, &state);
        expected_seq  = (expected_seq + 1) & 0x0F;
        break;
      // Overwrite queue at position.
      case 0x10:
        UARTRead((char *)&state, 8);
        ret = OverwriteState(command_queue_ptr, &state, header[1]);
        expected_seq  = (expected_seq + 1) & 0x0F;
        break;
      // Clear queue.
      case 0x20:
        ret = ClearQueue(command_queue_ptr);
        expected_seq  = (expected_seq + 1) & 0x0F;
        break;
      // Get number of elements in queue.
      case 0x30:
        ret = command_queue_ptr->size;
        expected_seq  = (expected_seq + 1) & 0x0F;
        break;
      // Overwrite last queue element.
      case 0x40:
        UARTRead((char *)&state, 8);
        ret = OverwriteState(command_queue_ptr, &state, 0);
        expected_seq  = (expected_seq + 1) & 0x0F;
        break;
      // Overwrite queue element and delete everything after it.
      case 0x50:
        UARTRead((char *)&state, 8);
        ret = OverwriteState(command_queue_ptr, &state, header[1]);
        if (ret != -1) {
          ret = DeleteAfter(command_queue_ptr, command_queue_ptr->size - header[1] - 1);
        }
        expected_seq  = (expected_seq + 1) & 0x0F;
        break;
      // Advance queue buffer position.
      case 0xA0:
        if (!IsEmpty(command_queue_ptr)) {
          command_queue_ptr->start_index = (command_queue_ptr->start_index + 1) % QUEUE_SIZE;
          command_queue_ptr->size --;
          ret = 0x00;
        } else {
          ret = 0xFF;
        }
        expected_seq  = (expected_seq + 1) & 0x0F;
        break;
      // Toggle Pop removing queue items.
      case 0xB0:
        command_queue_ptr->pop_removes_items = !command_queue_ptr->pop_removes_items;
        ret = command_queue_ptr->pop_removes_items;
        expected_seq  = (expected_seq + 1) & 0x0F;
        break;
      // Pop queue element (Special case, debug only, send two acks).
      case 0xC0:
        header[0] = 0xA0 | expected_seq;
        header[1] = command_queue_ptr->size;
        UARTWrite((const char *)&header, 2);
        if (Pop(command_queue_ptr, &state) >= 0) {
          UARTWrite((const char *)&state, 8);
          ret = command_queue_ptr->size;
        } else {
          ret = 0xFF;
        }
        expected_seq  = (expected_seq + 1) & 0x0F;
        break;
      // Dump queue contents (Special case, debug only, send two acks).
      case 0xD0:
        header[0] = 0xA0 | expected_seq;
        header[1] = command_queue_ptr->size;
        UARTWrite((const char *)&header, 2);
        for (i = command_queue_ptr->start_index; i != command_queue_ptr->end_index; i = (i + 1) % QUEUE_SIZE) {
          UARTWrite((const char *)&(command_queue_ptr->arr[i]), 8);
        }
        ret = command_queue_ptr->size;
        expected_seq  = (expected_seq + 1) & 0x0F;
        break;
      // Get queue size.
      case 0xE0:
        ret = QUEUE_SIZE;
        expected_seq  = (expected_seq + 1) & 0x0F;
        break;
      // Reset connection.
      case 0xF0:
        ret = 0xF0;
        expected_seq = 0;
        ClearQueue(command_queue_ptr);
        break;
      default:
        header[0] = 0xF0 | expected_seq;
        header[1] = 0xFF;
        UARTFlushRead();
        break;
    }
    header[0] = 0xA0 | seq;
  }
  header[1] = ret & 0xFF;
  UARTWrite((const char *)&header, 2);
  LedToggle(1);
  LedOff(2);
  return ret;
}