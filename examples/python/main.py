#!/usr/bin/env python3

import os
from os import path
from gi.repository import Gtk, Sexy, GObject

BASE_DIR = path.split(path.abspath(__file__))[0]
CSS_FILE = path.join(BASE_DIR, 'style.css')
UI_FILE = path.join(BASE_DIR, 'main.ui')

def main():
	GObject.type_register(Sexy.SpellEntry)
	builder = Gtk.Builder()
	builder.add_from_file(UI_FILE)

	win = builder.get_object('window')
	win.connect('delete-event', Gtk.main_quit)

	entry = Sexy.SpellEntry()
	entry.set_text('I was nto!')
	box = builder.get_object('box')
	box.add(entry)

	provider = Gtk.CssProvider()
	provider.load_from_path(CSS_FILE)
	context = entry.get_style_context()
	context.add_provider(provider, 800)

	label = builder.get_object('label')
	label_str = '<b>Available Languages</b>:\n{}\n'.format('\n'.join(entry.get_languages()))
	label_str += '<b>Enabled Languages</b>:\n{}'.format('\n'.join(entry.get_active_languages()))
	label.set_markup(label_str)

	win.show_all()
	Gtk.main()

if __name__ == '__main__':
	main()
