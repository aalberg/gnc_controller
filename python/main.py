#import time
import controller_driver
import seq_decode
import cmd_generator

def main():
  driver = controller_driver.ControllerDriver("COM6", True)
  decoder = seq_decode.CommandSequenceDecoder()
  driver.Append(decoder.DecodeSequence(["20", "multishine", "multishine", \
                                    "multishine", "multishine", "multishine"]))
  generator = cmd_generator.CommandGenerator(filename = "tas/testing.txt", \
                                             decoder = decoder, loop = False)
  while driver.HasCommand():
    driver.PopAndSend()

  while 1:
    run = False
    input = raw_input(">>")
    if input == "exit":
      driver.Close()
      break
    elif input[0:4] == "read":
      with open(input[5:]) as f:
        for line in f:
          sline = line.strip()
          if len(sline) > 0 and not sline[0] == '#' and not sline == "loop":
            driver.Append(decoder.DecodeSequence(sline))
      run = True
    elif input[0:4] == "loop":
      generator = cmd_generator.CommandGenerator(filename = input[5:], \
                                                 decoder = decoder, loop = True)
    elif input == "start":
      driver.StartPeriodicSend(generator)
    elif input == "s":
      driver.StopPeriodicSend()
    else:
      driver.Append(decoder.DecodeSequence(input))
      run = True
    
    if run:
      while driver.HasCommand():
        driver.PopAndSend()

if __name__ == "__main__":
  main()