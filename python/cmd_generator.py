import Queue
import controller_cmd

class CommandGenerator:
  def __init__(self, commands = None, filename = None, decoder = None, loop = False):
    self.queue = Queue.Queue()
    self.loop = loop
    if not commands == None:
      if isinstance(commands[0], controller_cmd.ControllerCommand):
        for command in commands:
          self.queue.put(command)
      elif not decoder == None and isinstance(commands[0], str):
        self.decoder = decoder
        for command in commands:
          command_list = self.decoder.DecodeSequence(file.readline().strip())
          for command in command_list:
            self.queue.put(command)
    elif not filename == None and not decoder == None:
      print "filename", filename, self.queue.qsize()
      self.file = open(filename, 'r')
      self.decoder = decoder
      self.RefillQueueFromFile()
      print self.queue.qsize()
  
  def RefillQueueFromFile(self):
    if not self.file == None:
      while self.queue.qsize() < 10:
        line = self.file.readline()
        print line
        if line == '' and self.loop:
          self.file.seek(0)
          continue#line = self.file.readline()
        elif not self.loop and not line == "loop\n":
          print "what?"
          break
        line = line.strip()
        if line == "loop":
          self.loop = True
        else:
          print line
          for command in self.decoder.DecodeSequence(line):
            self.queue.put(command)
  
  def __iter__(self):
    return self
  
  def __next__(self):
    return self.next()
    
  def next(self):
    if self.queue == None or self.queue.qsize() == 0:
      raise StopIteration
    ret = self.queue.get()
    if not self.file == None:
      self.RefillQueueFromFile()
    elif self.loop:
      self.queue.put(ret)
    return ret