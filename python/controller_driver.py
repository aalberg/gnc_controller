import serial
import Queue
import time
import threading
from controller_cmd import ControllerCommand

class ControllerDriver:
  def __init__(self, port = '', echo_serial = False):
    self.queue = Queue.Queue()
    self.unacked_packets = [0] * 16
    self.last_packet_sent = -1
    self.seq_num = 0
    self.uc_buffer_size = -1
    self.uc_cur_buffer = -1
    self.echo_serial = echo_serial
    self.ser = None
    self.Open(port)

  def Open(self, port = ''):
    if self.ser == None or not self.ser.isOpen():
      try:
        self.ser = serial.Serial(
          port=port,
          baudrate=115200,
          parity=serial.PARITY_NONE,
          stopbits=serial.STOPBITS_ONE,
          bytesize=serial.EIGHTBITS,
          timeout=1
        )
        self.SendCommand(ControllerCommand(cmd_type = 0xF, cmd_param = 0))
        self.SendCommand(ControllerCommand(cmd_type = 0xE, cmd_param = 0))
        print "Serial port opened"
        return 0
      except serial.SerialException as e:
        print str(e), self.ser
        return -1
    else:
      print "Serial port is already open"
      return -1

  def Close(self):
    self.ser.close()
    return 0

  def Read(self, num_bytes):
    response = bytearray(self.ser.read(num_bytes))
    if self.echo_serial:
      print 'R', ControllerCommand.SerializeString(response)
    return response

  def Write(self, buf):
    if self.echo_serial:
      print 'S', ControllerCommand.SerializeString(buf)
    return self.ser.write(buf)

  def SendCommand(self, command):
    if not command.valid:
      return None

    if self.ser == None:
      print 'S', ControllerCommand.SerializeString(command.Serialize(self.seq_num))
      return

    # Don't send a command with data if there is too much in the uC buffer.
    if command.has_data and self.uc_buffer_size / 2 < self.uc_cur_buffer:
      with self.queue.mutex:
        self.queue.queue.appendleft(command)
      self.SendCommand(ControllerCommand("30"))
      return None

    # Try to send once.
    serialized_packet = command.Serialize(self.seq_num)
    self.Write(command.Serialize(self.seq_num))
    response = self.Read(2)
    ack_byte = response[0] & 0xF0
    self.seq_num = response[0] & 0x0F

    # On nack, resend.
    while not ack_byte == 0xA0:
      print "Nack", ControllerCommand.SerializeString(response)
      self.Write(command.Serialize(self.seq_num))
      response = self.Read(2)
      ack_byte = response[0] & 0xF0
      self.seq_num = response[0] & 0x0F
      print "seq", self.seq_num

    # On ack, update state and process the return value.
    print "Ack", ControllerCommand.SerializeString(response)
    command_type = command.header[0] & 0xF0
    data_byte = response[1]
    advance_seq_num = True
    if command_type == 0x00 or command_type == 0x10 or command_type == 0x20 or \
       command_type == 0x30 or command_type == 0x40 or command_type == 0x50:
      self.uc_cur_buffer = data_byte
      if data_byte == 0xFF:
        print "uC queue full", ControllerCommand.SerializeString(response)
        with self.queue.mutex:
          self.queue.queue.appendleft(command)
    elif command_type == 0xA0:
      if data_byte == 0x00:
        print "Buffer advanced", ControllerCommand.SerializeString(response)
      else:
        print "Buffer advance failed", ControllerCommand.SerializeString(response)
    elif command_type == 0xB0:
      if data_byte == 0x00:
        print "Queue pop disabled", ControllerCommand.SerializeString(response)
      elif data_byte == 0x01:
        print "Queue pop enabled", ControllerCommand.SerializeString(response)
      else:
        print "Toggle queue pop failed", ControllerCommand.SerializeString(response)
        with self.queue.mutex:
          self.queue.queue.appendleft(command)
    elif command_type == 0xC0:
      self.uc_cur_buffer = data_byte
      response_packet = self.Read(8)
      print ControllerCommand.SerializeString(response_packet)

      # Second ack doesn't matter.
      response = self.Read(2)
    elif command_type == 0xD0:
      self.uc_cur_buffer = data_byte
      for i in xrange(0, data_byte):
        response_packet = self.Read(8)
        print ControllerCommand.SerializeString(response_packet)

      # Second ack doesn't matter.
      response = self.Read(2)
    elif command_type == 0xE0:
      self.uc_buffer_size = data_byte
    elif command_type == 0xF0:
      self.uc_cur_buffer = 0
      advance_seq_num = False
      if not data_byte == 0xF0:
        print "Sync failed", ControllerCommand.SerializeString(response)
        with self.queue.mutex:
          self.queue.queue.appendleft(command)

    if advance_seq_num:
      self.seq_num = (self.seq_num + 1) % 16

    return response

  def PopAndSend(self):
    if self.queue.qsize() > 0:
      self.SendCommand(self.Pop())
    else:
      print "No command to send"

  def StartPeriodicSend(self, generator):
    interval = .013
    self.timer = threading.Timer(interval, ControllerDriver.PeriodicSend, args = (self, generator, interval))
    self.timer.start()

  def StopPeriodicSend(self):
    self.timer.cancel()

  def PeriodicSend(self, generator, interval):
    self.timer = threading.Timer(interval, ControllerDriver.PeriodicSend, (self, generator, interval))
    self.timer.start()

    self.PopAndSend()
    stop_when_empty = False
    while self.queue.qsize() < 10:
      try:
        self.Append(generator.next())
      except StopIteration:
        stop_when_empty == True
    if stop_when_empty and self.queue.qsize() == 0:
      self.StopPeriodicSend()

  def Append(self, command):
    if isinstance(command, ControllerCommand):
      self.queue.put(command)
    elif isinstance(command, str):
      self.queue.put(ControllerCommand(command))
    elif isinstance(command, list):
      for c in command:
        self.Append(c)
    else:
      print "Invalid type:", type(command)

  def Pop(self):
    return self.queue.get()

  def Clear(self):
    with self.queue.mutex:
      self.queue.queue.clear()

  def HasCommand(self):
    return self.queue.qsize() > 0

