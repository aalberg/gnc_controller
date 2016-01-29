grammar = {
  # Nops.
  "": [],
  "nop": ["00"],
  "nop2": ["00", "00"],
  "nop3": ["00", "00", "00"],
  "nop4": ["nop2", "nop2"],
  "nop5": ["nop4", "00"],
  "nop6": ["nop3", "nop3"],
  "nop7": ["nop6", "00"],
  "nop8": ["nop4", "nop4"],
  "nop9": ["nop4", "nop5"],
  "nop10": ["nop4", "nop4", "nop2"],
  "nop11": ["nop4", "nop4", "nop3"],
  "nop12": ["nop8", "nop4"],
  "nop13": ["nop8", "nop5"],
  "nop14": ["nop7", "nop7"],
  "nop15": ["nop8", "nop7"],
  "nop16": ["nop8", "nop8"],

  # Basic movement.
  "dash l": ["00 jx-80"],
  "dash r": ["00 jx80"],
  "walk l": ["00 jx-63"],
  "walk r": ["00 jx63"],
  "jump": ["00 x"],
  "sh": ["00 x"],
  "sh l": ["00 jx-80", "sh"],
  "sh tl": ["dash l", "sh l"],
  "sh r": ["00 jx80", "sh"],
  "sh tr": ["dash r", "sh r"],
  "fh": ["00 x", "00 x", "00 x"],
  "fh l": ["00 jx-80", "fh"],
  "fh tl": ["dash l", "fh l"],
  "fh r": ["00 jx80", "fh"],
  "fh tr": ["dash r", "fh r"],
  "down": ["00 jy-80"],
  "drop": ["down", "down"],

  "b": ["00 b"],
  "down b": ["00 b jy-80"],
  "up b": ["00 b jy80"],
  "side b l": ["00 b jx-80"],
  "side b r": ["00 b jx80"],
  
  "roll l": ["00 r jx-80", "00 r jx-80"],
  "roll r": ["00 r jx80", "00 r jx80"],

  # Advanced techs.
  "p airdodge l": ["00 r jx-76 jy-23"],
  "p airdodge r": ["00 r jx76 jy-23"],
  
  "p wavedash l": ["jump", "nop2", "00 r jx-76 jy-23"],
  "p wavedash r": ["jump", "nop2", "00 r jx76 jy-23"],
  # Multishines are fun.
  "wavedash d": ["jump", "nop2", "00 r jy-80"],
  "jc shine": ["down b", "nop2", "jump"],
  "multishine": ["down b", "nop4", "jump", "nop2"],
  "p waveshine l": ["down b", "nop2", "p wavedash l"],
  "p waveshine r": ["down b", "nop2", "p wavedash r"],

  "dashdance cycle": ["dash r", "dash r", "nop16", "dash l", "dash l", "nop16"],
}

class CommandSequenceDecoder:
  def __init__(self, grammar = grammar):
    self.grammar = grammar
    self.echo_commands = False

  def ExpandCommand(self, command_seq):
    expansion_made = True
    i = 0
    cur_seq = command_seq
    while expansion_made and i < 100:
      expansion_made = False
      expanded = []
      for command in cur_seq:
        if command in self.grammar:
          expansion_made = True
          expanded.extend(self.grammar[command])
        else:
          expanded.append(command)
      cur_seq = expanded
    return cur_seq
    
  def DecodeSequence(self, seq):
    seq_split = []
    if isinstance(seq, str):
      seq_split = seq.split(';')
    elif isinstance(seq, list):
      seq_split = seq

    seq_split = self.ExpandCommand(seq_split)
    for command in seq_split:
      if self.echo_commands:
        print command
    return seq_split