project('extension-manager', 'c',
          version: '0.5.1',
    meson_version: '>= 0.59.0',
  default_options: [ 'warning_level=2',
                     'c_std=gnu11',
                   ],
)

i18n = import('i18n')

if get_option('development')
  app_id = 'com.mattjakeman.ExtensionManager.Devel'
  vcs_tag = run_command('git', 'rev-parse', '--short', 'HEAD', check: false).stdout().strip()
  if vcs_tag == ''
    version_suffix = '-devel'
  else
    version_suffix = '-@0@'.format(vcs_tag)
  endif
else
  app_id = 'com.mattjakeman.ExtensionManager'
  version_suffix = ''
endif

config_h = configuration_data()
config_h.set_quoted('APP_VERSION', meson.project_version() + version_suffix)
config_h.set_quoted('GETTEXT_PACKAGE', 'extension-manager')
config_h.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
config_h.set_quoted('APP_ID', app_id)
config_h.set_quoted('PKG_NAME', get_option('package'))
config_h.set_quoted('PKG_DISTRIBUTOR', get_option('distributor'))
config_h.set10('IS_OFFICIAL', get_option('official'))
config_h.set10('WITH_BACKTRACE', get_option('backtrace'))
configure_file(
  output: 'exm-config.h',
  configuration: config_h,
)
add_project_arguments([
  '-I' + meson.project_build_root(),
  '-Werror=implicit-function-declaration',
], language: 'c')


subdir('data')
subdir('src')
subdir('po')

gnome.post_install(
  glib_compile_schemas: true,
  gtk_update_icon_cache: true,
  update_desktop_database: true
)
