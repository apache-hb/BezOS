import can

if __name__ == "__main__":
    with can.Bus(interface='serial', channel='/tmp/canbus.pty', baudrate=9600, timeout=100000.0) as bus:
        for msg in bus:
            print(msg.data)
