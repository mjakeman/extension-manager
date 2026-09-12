exm_sources = [
  'main.c',
  'exm-window.c',
  'exm-application.c',
  'exm-gnome-shell-search-provider.c',

  # Detail View
  'exm-detail-view.c',
  'exm-screenshot.c',
  'exm-zoom-picture.c',
  'exm-animated-paintable.c',
  'exm-comment-tile.c',
  'exm-comment-dialog.c',
  'exm-rating.c',
  'exm-versions-dialog.c',

  # Screenshot View
  'exm-screenshot-view.c',

  # Author Page
  'exm-author-page.c',

  # Installed Page
  'exm-installed-page.c',
  'exm-extension-row.c',
  'exm-extension-toggle.c',

  # Browse Page
  'exm-browse-page.c',
  'exm-search-row.c',

  # Misc
  'exm-error-dialog.c',
  'exm-extension-error-dialog.c',
  'exm-upgrade-assistant.c',
  'exm-upgrade-result.c',
  'exm-unified-data.c',
  'exm-install-button.c',
  'exm-backtrace.c',
  'exm-utils.c'
]

cc = meson.get_compiler('c')
libbacktrace_dep = cc.find_library('backtrace', required: get_option('backtrace'))

exm_deps = [
  dependency('gtk4'),
  dependency('libadwaita-1', version: '>= 1.8.alpha'),
  dependency('gio-unix-2.0'),
  dependency('json-glib-1.0'),
  dependency('libsoup-3.0'),
  dependency('libxml-2.0'),
  dependency('glycin-2'),
  dependency('glycin-gtk4-2'),
]

if libbacktrace_dep.found()
  exm_deps += libbacktrace_dep
endif

gnome = import('gnome')

exm_sources += gnome.gdbus_codegen('exm-shell-search-provider-generated',
  'org.gnome.Shell.SearchProvider2.xml',
  interface_prefix: 'org.gnome.',
  namespace: 'Exm',
)

subdir('local')
subdir('web')

blueprints = custom_target('blueprints',
  input: files(
    'exm-author-page.blp',
    'exm-browse-page.blp',
    'exm-comment-dialog.blp',
    'exm-comment-tile.blp',
    'exm-detail-view.blp',
    'exm-error-dialog.blp',
    'exm-extension-error-dialog.blp',
    'exm-extension-row.blp',
    'exm-installed-page.blp',
    'exm-install-button.blp',
    'exm-rating.blp',
    'exm-screenshot-view.blp',
    'exm-screenshot.blp',
    'exm-search-row.blp',
    'exm-shortcuts-dialog.blp',
    'exm-upgrade-assistant.blp',
    'exm-versions-dialog.blp',
    'exm-window.blp'
  ),
  output: '.',
  command: [find_program('blueprint-compiler'), 'batch-compile', '@OUTPUT@', '@CURRENT_SOURCE_DIR@', '@INPUT@'],
)

exm_sources += gnome.mkenums_simple('exm-enums',
  sources: 'exm-types.h'
)

gresourceconf = configuration_data()
gresourceconf.set('appstream_file', '@0@.metainfo.xml'.format(app_id))
gresourceconf.set('resource_path', '/' + app_id.replace('.', '/'))

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