exm_sources = [
  'main.c',
  'exm-window.c',
  'exm-application.c',
  'exm-detail-view.c',
  'exm-screenshot.c',

  # Installed Page
  'exm-installed-page.c',
  'exm-extension-row.c',

  # Browse Page
  'exm-browse-page.c',
  'exm-search-row.c'
]

exm_deps = [
  dependency('gtk4'),
  dependency('libadwaita-1'),
  dependency('json-glib-1.0'),
  dependency('libsoup-3.0')
]

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
    'exm-screenshot.blp'
  ),
  output: '.',
  command: [find_program('blueprint-compiler'), 'batch-compile', '@OUTPUT@', '@CURRENT_SOURCE_DIR@', '@INPUT@'],
)

exm_sources += gnome.compile_resources('exm-resources',
  'exm.gresource.xml',
  dependencies: blueprints,
  c_name: 'exm'
)

executable('extension-manager', exm_sources,
  dependencies: exm_deps,
  install: true,
)
