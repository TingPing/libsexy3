project('libsexy3', 'c',
  version: '1.2.0',
  license: 'LGPL2+',
  meson_version: '>= 0.37.0',
  default_options: [
    'c_std=gnu99',
  ]
)

gnome = import('gnome')

libglib_dep = dependency('glib-2.0', version: '>= 2.44')
libgtk_dep = dependency('gtk+-3.0')
libenchant_dep = dependency('enchant')
iso_codes = dependency('iso-codes')
iso_codes_prefix = iso_codes.get_pkgconfig_variable('prefix')
iso_code_domains = iso_codes.get_pkgconfig_variable('domains').split()
have_639 = false
have_3166 = false
foreach domain : iso_code_domains
  if domain.startswith('iso_639')
    have_639 = true
  elif domain.startswith('iso_3166')
    have_3166 = true
  endif
endforeach
if not have_639 or not have_3166
  error('iso-codes domains iso-638 or iso-3166 not found')
endif

config_h = configuration_data()
config_h.set_quoted('PACKAGE_NAME', meson.project_name())
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('GETTEXT_PACKAGE', meson.project_name())
config_h.set10('ENABLE_NLS', true)
config_h.set_quoted('LOCALEDIR', join_paths(get_option('prefix'),
                                 get_option('datadir'), 'locale'))
config_h.set('GLIB_VERSION_MIN_REQUIRED', 'GLIB_VERSION_2_44')
config_h.set('GLIB_VERSION_MAX_ALLOWED', 'GLIB_VERSION_2_44')
config_h.set_quoted('ISO_CODES_PREFIX', iso_codes_prefix)
config_h.set_quoted('ISO_CODES_LOCALEDIR',
                    join_paths(iso_codes_prefix, 'share/locale'))

if get_option('buildtype') == 'release'
  config_h.set('G_DISABLE_ASSERT', true)
  config_h.set('G_DISABLE_CHECKS', true)
  config_h.set('G_DISABLE_CAST_CHECKS', true)
elif get_option('buildtype').startswith('debug')
  config_h.set('G_ENABLE_DEBUG', true)
endif
configure_file(
  output: 'config.h',
  configuration: config_h,
)
config_h_include = include_directories('.')

subdir('libsexy')
subdir('data')
subdir('po')
if get_option('with-docs')
  subdir('docs/reference')
endif
