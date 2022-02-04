project('extension-manager', 'c',
          version: '0.2.3',
    meson_version: '>= 0.50.0',
  default_options: [ 'warning_level=2',
                     'c_std=gnu11',
                   ],
)

i18n = import('i18n')

if get_option('development')
  app_title = 'Extension Manager (Development)'
  app_id = 'com.mattjakeman.ExtensionManager.Devel'
else
  app_title = 'Extension Manager'
  app_id = 'com.mattjakeman.ExtensionManager'
endif

config_h = configuration_data()
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('GETTEXT_PACKAGE', 'extension-manager')
config_h.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
config_h.set_quoted('APP_ID', app_id)
config_h.set10('IS_DEVEL', get_option('development'))
configure_file(
  output: 'exm-config.h',
  configuration: config_h,
)
add_project_arguments([
  '-I' + meson.build_root(),
], language: 'c')


subdir('data')
subdir('src')
subdir('po')

meson.add_install_script('build-aux/meson/postinstall.py')
