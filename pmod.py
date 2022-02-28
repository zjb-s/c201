import serial
import sys
crow = serial.Serial(
    port='/dev/tty.usbmodem346F365435381',
    baudrate=115200
)
if crow.name == '/dev/tty.usbmodem346F365435381':
    crow_connected = True

def jf_note(pitch, velocity):
    pitch = int(pitch) / 12
    command = 'ii.jf.play_note(' + str(pitch) + ',' + str(velocity) + ')\n'
    crow.write(command.encode())  

if crow_connected == True:
    if sys.argv[1] == "note":
        jf_note(sys.argv[2], sys.argv[3])