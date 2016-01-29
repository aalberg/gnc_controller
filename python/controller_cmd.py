button_table = {
  "a": (0, 0),
  "b": (0, 1),
  "x": (0, 2),
  "y": (0, 3),
  "s": (0, 4),
  "dl": (1, 0),
  "dr": (1, 1),
  "dd": (1, 2),
  "du": (1, 3),
  "z": (1, 4),
  "r": (1, 5),
  "l": (1, 6),
}

analog_table = {
  "jx": 2,
  "jy": 3,
  "cx": 4,
  "cy": 5,
  "lt": 6,
  "rt": 7,
}

class ControllerCommand:
  def __init__(self, cmd_str = None, cmd_type = -1, cmd_param = -1, body = None):
    self.header = bytearray([0] * 2)
    self.valid = True
    self.has_data = False
    if not cmd_str == None:
      self.header, self.data, self.has_data = ControllerCommand.ParseFromString(cmd_str)
    elif cmd_type >= 0 and cmd_param >= 0:
      self.header = ControllerCommand.GetHeader(cmd_type, cmd_param)
      self.data = body
      if not body == None:
        self.has_data = True
    else:
      self.valid = False
      
    if self.has_data:
      self.data[1] |= (1 << 7)

  @staticmethod
  def GetHeader(cmd_type = 0, cmd_param = 0, header_str = None):
    if not header_str == None:
      return bytearray([int(header_str[0], 16) << 4, int(header_str[1:], 16)])
    cmd_type = (cmd_type << 4) & 0xF0
    return bytearray([cmd_type, cmd_param & 0xFF])

  @staticmethod
  def ParseFromString(cmd_str):
    encoding = cmd_str[0]
    data = None
    has_data = False
    
    cmd_split = cmd_str.split(' ')
    header = ControllerCommand.GetHeader(header_str = cmd_split[0])
    
    # Commands that send controller state
    if encoding == '0' or encoding == '1' or encoding == '4' or encoding == '5':
      has_data = True
      data = bytearray([0x00, 0x80, 0x80, 0x7F, 0x80, 0x7F, 0x00, 0x00])
      for cmd in cmd_split:
        if cmd in button_table:
          data_index = button_table[cmd]
          data[data_index[0]] |= (1 << data_index[1])
        elif cmd[0:2] in analog_table:
          data_index = analog_table[cmd[0:2]]
          if cmd[1] == 'x':
            data[data_index] = (int(cmd[2:]) + 128) & 0xFF
          elif cmd[1] == 'y':
            data[data_index] = (int(cmd[2:]) + 127) & 0xFF
          else:
            data[data_index] = int(cmd[2:]) & 0xFF

    return header, data, has_data
  
  def Serialize(self, sequence):
    self.header[0] |= sequence & 0x0F
    packet = bytearray()
    packet.extend(self.header)
    if not self.data == None:
      packet.extend(self.data)
    return packet
    
  def SerializedString(self, sequence):
    return ' '.join(format(n,'02X') for n in self.Serialize(sequence))
    
  @staticmethod
  def SerializeString(str):
    return ' '.join(format(n,'02X') for n in str)