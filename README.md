# keystrokes-recorder
Keystrokes recorder for Windows, written in plain WinAPI using `PKBDTABLES` returned by `KbdLayerDescriptor` exported by the loaded keyboard layout.

Features:
  - supports every keyboard layout
  - supports changing keyboard layout during typing
  - writes recorded keystrokes to a file, along with the window name
