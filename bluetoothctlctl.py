# auto pair gamepads

import os
import signal
import subprocess
import time

controller_names_to_pair = ["Wireless Controller", "Pro Controller"]

def call_and_interrupt(command, seconds=0.5):
    process = subprocess.Popen(command, stdout=subprocess.PIPE, 
                       shell=True, preexec_fn=os.setsid) 
    print("Starting process ", process.pid)
    time.sleep(seconds) 
    os.killpg(os.getpgid(process.pid), signal.SIGTERM)  # Send the signal to all the process groups
    return process

pair_addrs = []
pair_pro = call_and_interrupt("bluetoothctl paired-devices")
for line in pair_pro.stdout:
    words = line.decode().split()
    pair_addrs.append(words[1])

all_device_addrs = []
device_pro = call_and_interrupt("bluetoothctl devices")
for line in device_pro.stdout:
    words = line.decode().split()
    all_device_addrs.append(words[1])

for addr in all_device_addrs:
    if addr not in pair_addrs:
        call_and_interrupt("bluetoothctl remove " + addr)

pro = call_and_interrupt("bluetoothctl scan on", 20 )

output = ""
bt_addrs = []
for line in pro.stdout: 
    txt = line.decode()
    for name in controller_names_to_pair:
        if name in txt:
            words = txt.split()
            bt_addrs.append(words[2])
            print(txt)

    output += line.decode()

for address in bt_addrs:
    con_pro = call_and_interrupt("bluetoothctl connect " + address, 4)
    
    for line in con_pro.stdout: 
        if "org.bluez.Error.Failed" in line.decode():
            dc_pro = call_and_interrupt("bluetoothctl remove " + address, 1)
