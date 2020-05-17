#!/usr/bin/env python

import os
os.environ["GDK_BACKEND"] = "x11"

import gi
gi.require_versions({'Gst': '1.0','Gtk': '3.0','GdkX11': '3.0', 'Gdk': '3.0', 'GstVideo': '1.0', 'GLib': '2.0'})
from gi.repository import Gst, GObject, Gtk, GdkX11, Gdk, GstVideo, GLib

import xdot
import ch

class GTK_Main(object):

    demux_stream_num=dict((
        ('audio', 0),
        ('video', 0),
        ))
    stream_demux_pads=dict()
    elements=dict()

    playsink_pads=dict()

    playsink=None
    vsink=None

    chinfo=None

    def __init__(self):
        window = Gtk.Window(type=Gtk.WindowType.TOPLEVEL)
        window.set_title("Player")
        window.set_default_size(500, 400)
        window.connect("destroy", Gtk.main_quit, "WM destroy")

        vbox = Gtk.VBox()
        window.add(vbox)
        hbox = Gtk.HBox()
        vbox.pack_start(hbox, False, False, 0)

        self.button = Gtk.Button(label="Start")
        hbox.pack_start(self.button, False, False, 0)
        self.button.connect("clicked", self.start_stop)
        drawdot = Gtk.Button(label='draw dot')
        hbox.pack_start(drawdot, False, False, 0)
        drawdot.connect('clicked', self.draw_dot)

        self.pipeline = Gst.Pipeline.new("dtv-pipeline")

        self.src = Gst.ElementFactory.make("dvbsrc", "src")
        self.demux = Gst.ElementFactory.make("tsdemux", "demux")
        self.demux.connect("pad-added", self.demux_pad_added)
        self.demux.connect("pad-removed", self.demux_pad_removed)

        self.vsink = Gst.ElementFactory.make('gtksink')
        self.playsink = Gst.ElementFactory.make('playsink')
        self.playsink.set_property("video-sink", self.vsink)
        #self.playsink.set_property('flags', "audio+video+deinterlace")
        Gst.util_set_object_arg(self.playsink, 'flags', 'audio+video+deinterlace')
        vbox.add(self.vsink.props.widget)

        self.pipeline.add(self.src)
        self.pipeline.add(self.demux)
        self.pipeline.add(self.playsink)

        self.src.link(self.demux)

        bus = self.pipeline.get_bus()
        bus.add_signal_watch()
        bus.enable_sync_message_emission()
        bus.connect("message", self.on_message)

        window.connect('key-press-event', self.on_key_press)
        window.show_all()

    def draw_dot(self, w):
        print('draw dot..')
        dot_data = Gst.debug_bin_to_dot_data(self.pipeline, Gst.DebugGraphDetails.ALL)
        win = xdot.DotWindow()
        win.set_dotcode(bytes(dot_data, 'utf-8'))

    def start(self):
        print('start..')
        self.button.set_label("Stop")

        if not self.chinfo:
            self.chinfo = ch.next()
        print('channel {0} {1}'.format(self.chinfo['VCHANNEL'], self.chinfo['name']))

        self.src.set_property('adapter', 1)
        self.src.set_property('delsys', dict([
            ('ATSC', 'atsc'),
            ])[self.chinfo['DELIVERY_SYSTEM']])
        self.src.set_property('frequency', int(self.chinfo['FREQUENCY']))
        self.src.set_property('modulation', dict([
            ('VSB/8', '8vsb'),
            ])[self.chinfo['MODULATION']])
        self.demux.set_property('program-number', int(self.chinfo['SERVICE_ID']))

        self.pipeline.set_state(Gst.State.PLAYING)

        self.last_pts = -1
        self.not_running = 0

    def stop(self):
        print('stop..')
        self.button.set_label("Start")
        self.pipeline.set_state(Gst.State.NULL)

    def start_stop(self, w):
        print("start_stop.. label : ", self.button.get_label())
        if self.button.get_label() == "Start":
            self.start()
        elif self.button.get_label() == "Stop":
            self.stop()

    def on_message(self, bus, message):
        t = message.type
        if t == Gst.MessageType.EOS:
            self.pipeline.set_state(Gst.State.NULL)
            self.button.set_label("Start")
        elif t == Gst.MessageType.ERROR:
            self.pipeline.set_state(Gst.State.NULL)
            err, debug = message.parse_error()
            print ("Error: %s" % err, debug)
            self.button.set_label("Start")

    def demux_pad_added(self, element, pad):
        print('demux pad added. {0}.{1}'.format(element.name, pad.name))
        av = pad.name.split('_')[0]
        if av != 'audio' and av != 'video':
            print('not A/V. skip')
            return
        av_num = self.demux_stream_num[av]
        if av_num > 0:
            print('second or more. skip')
            return

        self.demux_stream_num[av] = av_num + 1
        e_name_base = element.name.split(':')[0]

        q=Gst.ElementFactory.make('queue')
        d=Gst.ElementFactory.make('decodebin', f'{av}_dec')
        d.connect("pad-added", self.dec_pad_added)
        d.connect("pad-removed", self.dec_pad_removed)

        self.pipeline.add(q)
        self.pipeline.add(d)
        q.sync_state_with_parent()
        d.sync_state_with_parent()

        pad.link(q.get_static_pad('sink'))
        q.link(d)

        self.stream_demux_pads[av] = pad.name
        self.elements[pad.name] = dict([('q', q), ('d', d)])

    def demux_pad_removed(self, element, pad):
        print('demux pad removed. {0}.{1}'.format(element.name, pad.name))
        if pad.name in self.elements:
            av = pad.name.split('_')[0]

            self.elements[pad.name]['q'].set_state(Gst.State.NULL)
            self.elements[pad.name]['d'].set_state(Gst.State.NULL)
            self.pipeline.remove(self.elements[pad.name]['q'])
            self.pipeline.remove(self.elements[pad.name]['d'])
            del self.elements[pad.name]['q']
            del self.elements[pad.name]['d']
            del self.elements[pad.name]
            del self.stream_demux_pads[av]

            self.demux_stream_num[av] = self.demux_stream_num[av] - 1
        else:
            print(f'no key. {pad.name}')

    def dec_pad_added(self, element, pad):
        print('dec pad added. {0}.{1}'.format(element.name, pad.name))
        if not pad.name.startswith('src'):
            return

        av = element.name.split('_')[0]
        dpad = self.stream_demux_pads[av]

        self.playsink_pads[av] = self.playsink.get_request_pad(f'{av}_sink')
        pad.link(self.playsink_pads[av])

    def dec_pad_removed(self, element, pad):
        print('dec pad removed. {0}.{1}'.format(element.name, pad.name))

        if not pad.name.startswith('src'):
            return

        av = element.name.split('_')[0]

        self.playsink.release_request_pad(self.playsink_pads[av])
        del self.playsink_pads[av]

    def on_key_press(self, widget, event):
        print(f'key press. {event.keyval}')
        if event.keyval == Gdk.KEY_Down:
            self.chinfo = ch.next(self.chinfo)
            self.stop()
            self.start()
        elif event.keyval == Gdk.KEY_Up:
            self.chinfo = ch.prev(self.chinfo)
            self.stop()
            self.start()


if __name__ == '__main__':
    import sys
    import argparse

    parser = argparse.ArgumentParser(description='TV Viewer')
    parser.add_argument('--conf',
            help='channel list. output of dvbv5-scan. format of DVBV5')
    parser.add_argument('--loglevel',
            choices=['debug', 'info', 'warning', 'error', 'critical'],
            help='log level')
    parser.add_argument('--watch',
            help='channel name or number to watch')
    parser.add_argument('--adapter', type=int,
            help='dvb adapter number')

    args = parser.parse_args()

    if args.conf:
        ch.load_from_file(args.conf)

    Gst.init(None)

    GTK_Main()
    Gtk.main()

# gst-launch-1.0 \
#        dvbsrc adapter=1 delsys=atsc frequency=285000000 modulation=8vsb ! \
#        tsdemux program-number=45 ! \
#        decodebin caps=video/x-raw name=vd \
#        playsink name=sink \
#        vd.src_0 ! sink.video_sink
