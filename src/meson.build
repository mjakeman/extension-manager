exm_sources = [
  'main.c',
  'exm-window.c',
  'exm-application.c',

  # Detail View
  'exm-detail-view.c',
  'exm-screenshot.c',
  'exm-zoom-picture.c',
  'exm-comment-tile.c',
  'exm-comment-dialog.c',
  'exm-rating.c',

  # Installed Page
  'exm-installed-page.c',
  'exm-extension-row.c',

  # Browse Page
  'exm-browse-page.c',
  'exm-search-row.c',

  # Misc
  'exm-error-dialog.c',
  'exm-upgrade-assistant.c',
  'exm-upgrade-result.c',
  'exm-install-button.c',
  'exm-backtrace.c',
  'exm-utils.c'
]

cc = meson.get_compiler('c')
libbacktrace_dep = cc.find_library('backtrace', required: get_option('backtrace'))

exm_deps = [
  dependency('gtk4'),
  dependency('libadwaita-1'),
  dependency('gio-unix-2.0'),
  dependency('json-glib-1.0'),
  dependency('libsoup-3.0'),
  dependency('text-engine-0.1')
]

if libbacktrace_dep.found()
  exm_deps += libbacktrace_dep
endif

gnome = import('gnome')

subdir('local')
subdir('web')

blueprints = custom_target('blueprints',
  input: files(
    'exm-window.blp',
    'exm-installed-page.blp',
    'exm-browse-page.blp',
    'exm-extension-row.blp',
    'exm-search-row.blp',
    'exm-detail-view.blp',
    'exm-screenshot.blp',
    'exm-comment-tile.blp',
    'exm-comment-dialog.blp',
    'exm-rating.blp',
    'exm-upgrade-assistant.blp',
    'exm-error-dialog.blp'
  ),
  output: '.',
  command: [find_program('blueprint-compiler'), 'batch-compile', '@OUTPUT@', '@CURRENT_SOURCE_DIR@', '@INPUT@'],
)

exm_sources += gnome.mkenums_simple('exm-enums',
  sources: 'exm-types.h'
)

gresourceconf = configuration_data()
gresourceconf.set('appstream_file', '@0@.metainfo.xml'.format(app_id))

gresource_file = configure_file(
  input: 'exm.gresource.xml.in',
  output: 'exm.gresource.xml',
  configuration: gresourceconf,
)

exm_sources += gnome.compile_resources('exm-resources',
  gresource_file,
  dependencies: [blueprints, appstream_file],
  c_name: 'exm'
)

executable('extension-manager', exm_sources,
  dependencies: exm_deps,
  install: true,
)