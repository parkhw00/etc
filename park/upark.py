#!/usr/bin/env python

import gi
import subprocess

gi.require_version("Gtk", "3.0")
from gi.repository import Gtk,GLib

class ParkWindow(Gtk.Window):

    SYNC_SETHEIGHT = '0xb1'
    SYNC_DEFAULT = '0xa3'

    CMD_RESTART     = '0xa2'
    CMD_PARKSTATUS  = '0xa5'
    CMD_STATUS      = '0xa6'

    def __init__(self):
        Gtk.Window.__init__(self, title="Sensor Settings")
        self.set_border_width(3)
        #self.set_size_request(400, 400)

        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=3)
        self.add(vbox)

        # a row, input labels
        hbox = Gtk.Box(spacing=3)
        hbox.pack_start(Gtk.Label(label="address", xalign=1.0), True, True, 0)
        self.entry_addr = Gtk.Entry(text="1")
        hbox.pack_start(self.entry_addr, True, True, 0)

        hbox.pack_start(Gtk.VSeparator(), False, False, 0)
        hbox.pack_start(Gtk.Label(label="serial", xalign=1.0), True, True, 0)
        self.entry_serial = Gtk.Entry(text="/dev/ttyUSB0")
        hbox.pack_start(self.entry_serial, True, True, 0)

        hbox.pack_start(Gtk.VSeparator(), False, False, 0)
        hbox.pack_start(Gtk.Label(label="height(cm)", xalign=1.0), True, True, 0)
        self.entry_height = Gtk.Entry(text="200")
        hbox.pack_start(self.entry_height, True, True, 0)

        vbox.pack_start(hbox, False, False, 0)

        # a row, second input
        hbox = Gtk.Box(spacing=3)

        hbox.pack_start(Gtk.Label(label="led(occupied)", xalign=1.0), True, True, 0)
        self.sel_led_occupied = Gtk.ComboBoxText()
        self.sel_led_occupied.append('0', "red:occupied, green:vacant")
        self.sel_led_occupied.append('1', "red:off, green:off")
        self.sel_led_occupied.append('2', "red:off, green:on")
        self.sel_led_occupied.append('3', "red:on, green:off")
        self.sel_led_occupied.append('4', "red:on, green:on")
        self.sel_led_occupied.append('5', "red:off, green:blinking")
        self.sel_led_occupied.append('6', "red:blinking, green:off")
        self.sel_led_occupied.append('7', "red:blinking, green:blinking")
        hbox.pack_start(self.sel_led_occupied, True, True, 0)

        hbox.pack_start(Gtk.VSeparator(), False, False, 0)
        hbox.pack_start(Gtk.Label(label="led(vacant)", xalign=1.0), True, True, 0)
        self.sel_led_vacant = Gtk.ComboBoxText()
        self.sel_led_vacant.append('0', "red:occupied, green:vacant")
        self.sel_led_vacant.append('1', "red:off, green:off")
        self.sel_led_vacant.append('2', "red:off, green:on")
        self.sel_led_vacant.append('3', "red:on, green:off")
        self.sel_led_vacant.append('4', "red:on, green:on")
        self.sel_led_vacant.append('5', "red:off, green:blinking")
        self.sel_led_vacant.append('6', "red:blinking, green:off")
        self.sel_led_vacant.append('7', "red:blinking, green:blinking")
        hbox.pack_start(self.sel_led_vacant, True, True, 0)

        hbox.pack_start(Gtk.VSeparator(), False, False, 0)
        self.button_reset = Gtk.Button(label="set&restart")
        self.button_reset.connect("clicked", self.on_restart)
        hbox.pack_start(self.button_reset, True, True, 0)

        vbox.pack_start(hbox, False, False, 0)

        # a row, log message box
        vbox.pack_start(Gtk.HSeparator(), False, False, 0)

        self.button_startstop = Gtk.Button(label="start")
        self.button_startstop.connect("clicked", self.on_startstop)
        vbox.pack_start(self.button_startstop, False, False, 0)
        self.log = Gtk.TextView(monospace=True, editable=True)
        scr = Gtk.ScrolledWindow(vscrollbar_policy=Gtk.PolicyType.AUTOMATIC)
        #scr.set_size_request(-1, 200)
        scr.add(self.log)
        vbox.pack_start(scr, True, True, 0)

        self.connect("destroy", Gtk.main_quit)
        self.show_all()

    def logl(self, str):
        print(str)
        return True

    def setstatus(self, str):
        buffer = self.log.get_buffer()
        buffer.set_text(str)

    def runcmd(self, order=CMD_STATUS, sync=SYNC_DEFAULT):
        r = subprocess.run(['./park',
            '-s', sync,
            '-d', self.entry_serial.get_text(),
            '-a', self.entry_addr.get_text(),
            '-o', order],
                stdout=subprocess.PIPE)
        return r.stdout.decode('utf-8')

    def on_restart(self, button):
        led_occ = self.sel_led_occupied.get_active_id()
        if led_occ != None:
            self.logl('led occ {}'.format(led_occ))
            self.logl('set led occ\n' + self.runcmd(order=str(0xd0+int(led_occ))))

        led_vac = self.sel_led_vacant.get_active_id()
        if led_vac != None:
            self.logl('led vac {}'.format(led_vac))
            self.logl('set led vac\n' + self.runcmd(order=str(0xd8+int(led_vac))))

        try:
            height = int(self.entry_height.get_text())
            if height < 200:
                print('Too small height')
                raise Exception('Too small height')
        except:
            height = 200
            self.entry_height.set_text(str(height))

        self.logl('set height {}cm. restart..'.format(height) )
        self.logl('set height\n' + self.runcmd(order=str(height-200), sync=self.SYNC_SETHEIGHT))
        self.logl('restart..\n' + self.runcmd(self.CMD_RESTART))

    def statuscheck(self):
        self.logl('check..')
        self.setstatus(self.runcmd() + '\n' + self.runcmd(order=self.CMD_PARKSTATUS))
        #self.logl('check.. done.')

        return True

    def on_start(self):
        self.logl('start..')
        self.statuscheck()
        self.tid = GLib.timeout_add(2000, self.statuscheck)

    def on_stop(self):
        self.logl('stop..')
        GLib.source_remove(self.tid)

    def on_startstop(self, button):
        if button.get_label() == 'start':
            button.set_label('stop')
            self.on_start()
        else:
            button.set_label('start')
            self.on_stop()


win = ParkWindow()
Gtk.main()

