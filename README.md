# libsexy3 [![Build Status](https://travis-ci.org/TingPing/libsexy3.svg?branch=master)](https://travis-ci.org/TingPing/libsexy3)

This is a *"continuation"* of the libsexy library.
Currently only spell-entry has been ported, most of the
other widgets are no longer needed/used. More may be added
in the future.

## Docs

- [C](http://tingping.github.io/libsexy3/c)
- [Python](http://tingping.github.io/libsexy3/py)

## Changes

### Additions

- Glade catalog
- GIR files
- VAPI files
- Use gobject-properties
- Add underline-color style

### Differences

- Enchant is now a build time requirement
- ```sexy_spell_entry_is_checked``` --> ```sexy_spell_entry_get_checked```
- Stock icons have been removed from the menus

### Todo

- Add libsexy3mm files
- Add translations
- Automate updating docs

## Install

### Dependencies

- gtk3-devel
- enchant-devel
- iso-codes
- gobject-introspection
- gtk-doc

```sh
./autogen.sh
./configure --enable-gtk-doc
make
sudo make install
```
