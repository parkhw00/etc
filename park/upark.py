#!/usr/bin/env python

import gi

gi.require_version("Gtk", "3.0")
from gi.repository import Gtk,GObject

class ParkWindow(Gtk.Window):
    def __init__(self):
        Gtk.Window.__init__(self, title="Sensor Settings")
        self.set_border_width(3)
        self.set_size_request(400, 400)

        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=3)
        self.add(vbox)

        # a row, input labels
        hbox = Gtk.Box(spacing=3)
        hbox.add(Gtk.Label(label="address", xalign=0.0))
        self.entry_addr = Gtk.Entry(text="1")
        hbox.add(self.entry_addr)

        hbox.add(Gtk.VSeparator())
        hbox.add(Gtk.Label(label="serial", xalign=0.0))
        self.entry_serial = Gtk.Entry(text="/dev/ttyUSB0")
        hbox.add(self.entry_serial)

        hbox.add(Gtk.VSeparator())
        hbox.add(Gtk.Label(label="height(cm)", xalign=0.0))
        self.entry_height = Gtk.Entry(text="")
        hbox.add(self.entry_height)
        vbox.add(hbox)

        hbox.add(Gtk.VSeparator())
        self.button_reset = Gtk.Button(label="restart")
        self.button_reset.connect("clicked", self.on_restart)
        hbox.add(self.button_reset)

        vbox.add(Gtk.HSeparator())

        # a row, log message box
        vbox.add(Gtk.HSeparator())

        self.button_startstop = Gtk.Button(label="start")
        self.button_startstop.connect("clicked", self.on_startstop)
        vbox.add(self.button_startstop)
        self.log = Gtk.TextView(monospace=True, editable=True)
        scr = Gtk.ScrolledWindow(vscrollbar_policy=Gtk.PolicyType.AUTOMATIC)
        scr.set_size_request(-1, 200)
        scr.add(self.log)
        vbox.add(scr)

        self.connect("destroy", Gtk.main_quit)
        self.show_all()

    def logaddl(self, str):
        print(str)
        #buffer = self.log.get_buffer()
        #buffer.insert(buffer.get_end_iter(), str+'\n')

    def on_restart(self, button):
        self.logaddl('restart..')

    def statuschack(self):
        self.logaddl('chuck..')

    def on_start(self):
        self.logaddl('start..')
        GObject.timeout_add()

    def on_stop(self):
        self.logaddl('stop..')

    def on_startstop(self, button):
        if button.get_label() == 'start':
            button.set_label('stop')
            self.on_start()
        else:
            button.set_label('start')
            self.on_stop()


win = ParkWindow()
Gtk.main()

