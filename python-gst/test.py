#!/usr/bin/env python

import os
os.environ["GDK_BACKEND"] = "x11"

import argparse
import sys
import gi
gi.require_versions({'Gst': '1.0','Gtk': '3.0','GdkX11': '3.0', 'GstVideo': '1.0', 'GLib': '2.0'})
from gi.repository import Gst, GObject, Gtk, GdkX11, GstVideo, GLib

class GTK_Main(object):

    def __init__(self):
        window = Gtk.Window(type=Gtk.WindowType.TOPLEVEL)
        window.set_title("Player")
        window.set_default_size(500, 400)
        window.connect("destroy", Gtk.main_quit, "WM destroy")

        vbox = Gtk.VBox()
        window.add(vbox)
        hbox = Gtk.HBox()
        vbox.pack_start(hbox, False, False, 0)
        self.entry = Gtk.Entry()
        self.entry.set_text('rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov')
        hbox.add(self.entry)

        self.button = Gtk.Button(label="Start")
        hbox.pack_start(self.button, False, False, 0)
        self.button.connect("clicked", self.start_stop)

        #self.button_check = Gtk.Button(label="Check")
        #self.button_check.connect("clicked", self.check)
        #hbox.pack_start(self.button_check, False, False, 0)

        self.player = Gst.ElementFactory.make("playbin", "player")
        if True:
            self.sink = Gst.ElementFactory.make("gtksink", "sink")
            vbox.add(self.sink.props.widget)
        else:
            self.sink = Gst.ElementFactory.make("fakesink", "sink")
        self.player.set_property("video-sink", self.sink)

        #self.player = Gst.Pipeline.new("player")
        #self.src = Gst.ElementFactory.make("videotestsrc", "src")
        #self.sink = Gst.ElementFactory.make("xvimagesink", "sink")
        #self.sink = Gst.ElementFactory.make("fakesink", "sink")
        #self.player.add(self.src)
        #self.player.add(self.sink)
        #self.src.link(self.sink)

        bus = self.player.get_bus()
        bus.add_signal_watch()
        bus.enable_sync_message_emission()
        bus.connect("message", self.on_message)

        window.show_all()

    def start_stop(self, w):
        print("start_stop.. label : ", self.button.get_label())
        if self.button.get_label() == "Start":
            uri = self.entry.get_text().strip()
            print('start.. uri', uri)
            self.button.set_label("Stop")
            self.player.set_property("uri", uri)
            self.player.set_state(Gst.State.PLAYING)

            self.last_pts = -1
            self.not_running = 0
            GLib.timeout_add(1000, self.timeout_check)

    def check(self, w):
        self.timeout_check()

    def timeout_check(self):
        print("check..")
        if self.button.get_label() == "Stop":
            sample=self.sink.get_property("last-sample")
            if sample != None:
                buf=sample.get_buffer()
                print("pts", buf.pts, "last_pts", self.last_pts)
                if self.last_pts != buf.pts:
                    self.not_running = 0
                    self.last_pts = buf.pts
                else:
                    self.not_running = self.not_running + 1
                    print("not running while seconds", self.not_running)
                    if self.not_running > 60:
                        print("quit..")
                        Gtk.main_quit()

            GLib.timeout_add(1000, self.timeout_check)

    def on_message(self, bus, message):
        t = message.type
        if t == Gst.MessageType.EOS:
            self.player.set_state(Gst.State.NULL)
            self.button.set_label("Start")
        elif t == Gst.MessageType.ERROR:
            self.player.set_state(Gst.State.NULL)
            err, debug = message.parse_error()
            print ("Error: %s" % err, debug)
            self.button.set_label("Start")

def main(args):
    Gst.init(None)

    GTK_Main()
    Gtk.main()

if __name__ == '__main__':
    sys.exit(main(sys.argv))


# GDK_BACKEND=x11 ./test.py

#win = Gtk.Window()
#win.connect("destroy", Gtk.main_quit)
#win.add (Gtk.Button (label="Test Button"))
#win.show_all()
#Gtk.main()
