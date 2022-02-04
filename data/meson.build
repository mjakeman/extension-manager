desktop_file_in_config = configuration_data()
desktop_file_in_config.set('app_id', app_id)
desktop_file_in_config.set('app_title', app_title)
desktop_file_in_config.set('app_version', meson.project_version())
desktop_file_in = configure_file(
          input: 'com.mattjakeman.ExtensionManager.desktop.in.in',
         output: '@0@.desktop.in'.format(app_id),
  configuration: desktop_file_in_config,
)

desktop_file = i18n.merge_file(
  input: desktop_file_in,
  output: '@0@.desktop'.format(app_id),
  type: 'desktop',
  po_dir: '../po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'applications')
)

desktop_utils = find_program('desktop-file-validate', required: false)
if desktop_utils.found()
  test('Validate desktop file', desktop_utils,
    args: [desktop_file]
  )
endif

appconf = configuration_data()
appconf.set('app_id', app_id)
appconf.set('app_title', app_title)
appstream_file_in = configure_file(
          input: 'com.mattjakeman.ExtensionManager.appdata.xml.in.in',
         output: '@0@.appdata.xml.in'.format(app_id),
  configuration: appconf,
)

appstream_file = i18n.merge_file(
  input: appstream_file_in,
  output: '@0@.appdata.xml'.format(app_id),
  po_dir: '../po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'appdata')
)

appstream_util = find_program('appstream-util', required: false)
if appstream_util.found()
  test('Validate appstream file', appstream_util,
    args: ['validate-relax', appstream_file]
  )
endif

schemaconf = configuration_data()
schemaconf.set('app_id', app_id)
schema_file_in = configure_file(
          input: 'com.mattjakeman.ExtensionManager.gschema.xml.in',
         output: '@0@.gschema.xml'.format(app_id),
  configuration: appconf,
)

install_data(schema_file_in,
  install_dir: join_paths(get_option('datadir'), 'glib-2.0/schemas')
)

compile_schemas = find_program('glib-compile-schemas', required: false)
if compile_schemas.found()
  test('Validate schema file', compile_schemas,
    args: ['--strict', '--dry-run', meson.current_source_dir()]
  )
endif

subdir('icons')
