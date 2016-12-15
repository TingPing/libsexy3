install_data('sexy-catalog.xml',
  install_dir: join_paths(get_option('datadir'), 'glade/catalogs')
)

pkgconf = configuration_data()
prefix = get_option('prefix')
pkgconf.set('prefix', prefix)
pkgconf.set('libdir', join_paths(prefix, get_option('libdir')))
pkgconf.set('includedir', join_paths(prefix, get_option('includedir')))
pkgconf.set('VERSION', meson.project_version())

configure_file(
  input: 'libsexy3.pc.in',
  output: 'libsexy3.pc',
  configuration: pkgconf,
  install: true,
  install_dir: join_paths(get_option('libdir'), 'pkgconfig')
)
