exm_sources = [
  'main.c',
  'exm-window.c',
  'exm-application.c'
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
    'gtk/help-overlay.blp',
    'exm-window.blp'
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
