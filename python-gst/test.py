#!/usr/bin/env python

import os
os.environ["GDK_BACKEND"] = "x11"

import argparse
import sys
import gi
gi.require_versions({'Gst': '1.0', 'GLib': '2.0'})
from gi.repository import Gst, GObject, GLib

class PlayCheck(object):

    def __init__(self, loop, uri):
        self.status = 0;
        self.loop = loop

        self.player = Gst.ElementFactory.make("playbin", "player")
        self.sink = Gst.ElementFactory.make("fakesink", "sink")
        self.player.set_property("video-sink", self.sink)

        bus = self.player.get_bus()
        bus.add_signal_watch()
        bus.enable_sync_message_emission()
        bus.connect("message", self.on_message)

        print('start..', uri)
        self.player.set_property("uri", uri)
        self.player.set_state(Gst.State.PLAYING)

        self.last_pts = -1
        self.not_running = 0
        GLib.timeout_add(1000, self.check)

    def check(self):
        print("check..")
        sample=self.sink.get_property("last-sample")
        if sample != None:
            buf=sample.get_buffer()
            print("pts", buf.pts, "last_pts", self.last_pts)
            if self.last_pts != buf.pts:
                self.not_running = 0
                self.last_pts = buf.pts
            else:
                self.not_running = self.not_running + 1
                print("not running", self.not_running)
                if self.not_running > 60:
                    print("quit..")
                    self.status = 1
                    self.loop.quit()

        GLib.timeout_add(1000, self.check)

    def on_message(self, bus, message):
        t = message.type
        if t == Gst.MessageType.EOS:
            self.player.set_state(Gst.State.NULL)
            print ("EOS")
            self.loop.quit()
        elif t == Gst.MessageType.ERROR:
            self.player.set_state(Gst.State.NULL)
            err, debug = message.parse_error()
            print ("Error: %s" % err, debug)
            self.loop.quit()

    def get_status(self):
        return self.status

def main(args):
    parser = argparse.ArgumentParser(description='media play checker')
    parser.add_argument('-u', '--uri',
            default='rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov',
            type=str, help='media uri to check play')
    arg = parser.parse_args()
    print (arg)

    Gst.init(None)

    loop = GLib.MainLoop()
    p = PlayCheck(loop, arg.uri)

    loop.run()
    ret = p.get_status()
    print ("done.", ret)
    return ret

if __name__ == '__main__':
    sys.exit(main(sys.argv))


# GDK_BACKEND=x11 ./test.py

#win = Gtk.Window()
#win.connect("destroy", Gtk.main_quit)
#win.add (Gtk.Button (label="Test Button"))
#win.show_all()
#Gtk.main()
