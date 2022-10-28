import os
from queue import Queue
import struct
from threading import Condition, Event, Thread
import tkinter as tk
from turtle import circle
import serial


class App(tk.Tk):

    class State:
        CONNECTED = 0
        NOT_CONNECTED = 1

    class Action:
        FORWARD = 0
        RIGHT = 1
        LEFT = 2
        BACK = 3
        IDLE = 4

    MAX_THRUST = 4082
    MIN_THRUST = 0
    MIDDLE_THRUST = MAX_THRUST // 2

    def __init__(self) -> None:
        super().__init__()

        self._serial: serial.Serial = None
        self._con = Event()
        self._tx = Queue()
        self.state_update = Queue()
        self.draw = Queue()
        self.cmd = Queue()

        pad = {'padx': 7, 'pady': 7}
        style_frames =  {'background': 'white'}
        style = style_frames.copy()
        style.update({'font': ("Arial", 25)})

        self.keymap = {
            App.Action.FORWARD: False,
            App.Action.RIGHT: False,
            App.Action.LEFT: False,
            App.Action.BACK: False,
        }

        # -- Settings -- #
        self.frame_settings = tk.LabelFrame(self, text='Serial settings', **style_frames)

        self.status = tk.Label(self.frame_settings, **style)
        self.status.grid(row=0, columnspan=2, **pad)

        self.port = tk.Entry(self.frame_settings, **style)
        tk.Label(self.frame_settings, text='Port', **style).grid(row=1, column=0, **pad)
        self.port.grid(row=1, column=1, **pad)

        self.baud = tk.Entry(self.frame_settings, **style)
        tk.Label(self.frame_settings, text='Baudrate', **style).grid(row=2, column=0, **pad)
        self.baud.grid(row=2, column=1, **pad)

        self.btn_connect = tk.Button(self.frame_settings, text='Connect', command=self._connect, **style)
        self.btn_connect.grid(row=3, column=0, **pad)

        self.btn_disconnect = tk.Button(self.frame_settings, text='Disonnect', command=self._disconnect, **style)
        self.btn_disconnect.grid(row=3, column=1, **pad)

        self.baud.insert(tk.END, '115200')
        self.port.insert(tk.END, '/dev/ttyACM0')

        # -- Thrust -- #
        self.frame_thrust = tk.Frame(self, **style_frames)

        self.thrust = tk.Scale(self.frame_thrust, from_=self.MIN_THRUST, to_=self.MAX_THRUST, **style)
        tk.Label(self.frame_thrust, text='Thrust', **style).grid(row=0, column=0, **pad)
        self.thrust.grid(row=1, column=0, **pad)
        tk.Button(self.frame_thrust, text='Reset', command=self._reset_thrust, **style).grid(row=2, column=0, **pad)

        self.steer = tk.Scale(self.frame_thrust, from_=self.MIN_THRUST, to_=self.MAX_THRUST, **style)
        tk.Label(self.frame_thrust, text='Steer', **style).grid(row=0, column=1, **pad)
        self.steer.grid(row=1, column=1, **pad)
        tk.Button(self.frame_thrust, text='Reset', command=self._reset_steer, **style).grid(row=2, column=1, **pad)

        self.thrust.set(self.MIDDLE_THRUST)
        self.steer.set(self.MIDDLE_THRUST)

        self.canvas = tk.Canvas(self.frame_thrust)
        self.canvas.grid(row=0, column=2, rowspan=2, columnspan=2)

        self.thrust.bind("<ButtonRelease-1>", lambda *a: self._run())
        self.steer.bind("<ButtonRelease-1>", lambda *a: self._run())

        self.btn_run = tk.Button(self.frame_thrust, text='Run', command=self._run, **style)
        self.btn_run.grid(row=2, column=3)

        self.bind('<KeyPress>', self._keypress)
        self.bind('<KeyRelease>', self._keyrelease)

        # Done, let's setup final GUI stuff
        self._setup()

        # Pack frames
        self.frame_settings.pack(fill=tk.BOTH, expand=True)
        self.frame_thrust.pack(fill=tk.BOTH, expand=True)

        self.StatusThread(self).start()
        self.ReaderThread(self).start()
        self.WriterThread(self).start()
        self.CanvasAnimator(self).start()
        self.CommanderThread(self).start()

        self.state_update.put(self.State.NOT_CONNECTED)

    def _connect(self) -> None:
        if self._serial is not None:
            print('Already connected!')
            return

        port = self.port.get()
        baudrate = int(self.baud.get())
        try:
            self._serial = serial.Serial(port, baudrate=baudrate)
        except serial.SerialException:
            print('Failed to connect!')
            return

        print(f'Connected to serial on port {port} with baudrate {baudrate}')
        self._con.set()
        self.state_update.put(self.State.CONNECTED)

    def _disconnect(self) -> None:
        if self._serial is None:
            print('Not connected!')
            return

        self._serial.close()
        self._con.clear()
        self.state_update.put(self.State.NOT_CONNECTED)
        self._serial = None

    def _reset_steer(self) -> None:
        self.steer.set(2041)
        self._run()

    def _reset_thrust(self) -> None:
        self.thrust.set(2041)
        self._run()

    def _run(self) -> None:
        if self._serial is None:
            print('Not connected!')
            return

        self._write(struct.pack('II', int(self.thrust.get()), int(self.steer.get())))

    def _write(self, data: bytes) -> None:
        self._tx.put(data)

    def _setup(self) -> None:
        self.title('BeeNrf Control')
        self.config(background='white')

    # Callbacks
    def _keypress(self, keypress) -> None:
        char = keypress.char.upper()
        if char == 'W':
            self.keymap[App.Action.FORWARD] = True
        elif char == 'A':
            self.keymap[App.Action.LEFT] = True
        elif char == 'D':
            self.keymap[App.Action.RIGHT] = True
        elif char == 'S':
            self.keymap[App.Action.BACK] = True

        self.draw.put(1)
        self.cmd.put(1)

    def _keyrelease(self, keyrelease) -> None:
        char = keyrelease.keysym.upper()
        if char == 'W':
            self.keymap[App.Action.FORWARD] = False
        elif char == 'A':
            self.keymap[App.Action.LEFT] = False
        elif char == 'D':
            self.keymap[App.Action.RIGHT] = False
        elif char == 'S':
            self.keymap[App.Action.BACK] = False

        self.draw.put(1)
        self.cmd.put(1)

    # -- Background threads -- #
    class BackgroundThread(Thread):

        def __init__(self, app: 'App') -> None:
            super().__init__(daemon=True, name=self.__class__.__name__)
            self.app = app

    class ReaderThread(BackgroundThread):

        def run(self) -> None:
            print(f'{self.name} Started')
            while True:
                self.app._con.wait()
                rx = self.app._serial.readline()
                print(f'RX: {rx}')

    class WriterThread(BackgroundThread):

        def run(self) -> None:
            print(f'{self.name} Started')
            while True:
                self.app._con.wait()
                tx = self.app._tx.get()
                self.app._serial.write(tx)
                self.app._serial.flushOutput()
                print(f'TX: {tx}')


    class StatusThread(BackgroundThread):

        def run(self) -> None:
            print(f'{self.name} Started')

            while True:
                new_state = self.app.state_update.get()
                if new_state == App.State.CONNECTED:
                    fg = 'green'
                    text = 'Connected'
                elif new_state == App.State.NOT_CONNECTED:
                    fg = 'red'
                    text='Not connected'

                self.app.status.config(fg=fg, text=text)

    class CommanderThread(BackgroundThread):

        def run(self) -> None:
            print(f'{self.name} Started')

            while True:
                self.app._con.wait()
                self.app.cmd.get()

                if self.app.keymap[App.Action.FORWARD]:
                    thrust = App.MAX_THRUST
                elif self.app.keymap[App.Action.BACK]:
                    thrust = App.MIN_THRUST
                else:
                    thrust = App.MIDDLE_THRUST

                if self.app.keymap[App.Action.RIGHT]:
                    steer = App.MAX_THRUST
                elif self.app.keymap[App.Action.LEFT]:
                    steer = App.MIN_THRUST
                else:
                    steer = App.MIDDLE_THRUST

    class CanvasAnimator(BackgroundThread):

        def run(self) -> None:
            print(f'{self.name} Started')
            c = self.app.canvas
            c.update()
            w = c.winfo_width()
            h = c.winfo_height()

            r = 25
            cx = w / 2
            cy = h / 2

            bg = 'gray'
            fill = 'red'

            circles = {
                App.Action.FORWARD: c.create_oval(cx-r, cy-100-r, cx+r, cy-100+r, fill=bg, outline=bg),
                App.Action.RIGHT:   c.create_oval(cx+100-r, cy-r, cx+100+r, cy+r, fill=bg, outline=bg),
                App.Action.LEFT:    c.create_oval(cx-100-r, cy-r, cx-100+r, cy+r, fill=bg, outline=bg),
                App.Action.BACK:    c.create_oval(cx-r, cy+100-r, cx+r, cy+100+r, fill=bg, outline=bg),
            }

            c.config(background=bg)

            while True:
                action = self.app.draw.get()

                for action, circle in circles.items():
                    is_pressed = self.app.keymap[action]
                    filling = fill if is_pressed else bg
                    c.itemconfig(circle, fill=filling)



if __name__ == '__main__':
    os.system("xset r off")      # This is to turn off key-press getting stuck on linux.
    try:
        app = App()
        app.mainloop()
    finally:
        os.system("xset r on")
