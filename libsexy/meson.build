libsexy_sources = [
  'gtkspell-iso-codes.c',
  'sexy-spell-entry.c',
]

libsexy_public_headers = [
  'sexy.h',
  'sexy-spell-entry.h'
]

install_headers(libsexy_public_headers, subdir: 'libsexy3')

libsexy_deps = [
  libglib_dep,
  libgtk_dep,
  libenchant_dep
]

libsexy_includes = [
  config_h_include
]

libsexy_c_args = [
  '-DHAVE_CONFIG_H',
  '-Wl,--version-script,' + join_paths(meson.current_source_dir(), 'sexy.map'),
]

libsexy = library('sexy3',
  sources: libsexy_sources,
  include_directories: libsexy_includes,
  c_args: libsexy_c_args,
  link_depends: 'sexy.map',
  version: '1.2.0',
  dependencies: libsexy_deps,
  pic: true,
  install: true
)

libsexy_dep = declare_dependency(
  link_with: libsexy,
  include_directories: include_directories('.'),
)

libsexy_gir = gnome.generate_gir(
  libsexy,
  sources: libsexy_sources + libsexy_public_headers,
  nsversion: '3.0',
  namespace: 'Sexy',
  symbol_prefix: 'sexy',
  identifier_prefix: 'Sexy',
  install: true,
  includes: 'Gtk-3.0',
  extra_args: [
    '--c-include=sexy.h',
  ],
)

if get_option('with-vapi')

  gnome.generate_vapi('libsexy3',
    sources: libsexy_gir[0],
    packages: 'gtk+-3.0',
    install: true
  )

endif
