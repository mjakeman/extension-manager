exm_sources = [
  'main.c',
  'exm-window.c',
    'exm-application.c',
  ]

exm_deps = [
  dependency('gtk4'),
  dependency('libadwaita-1'),
]

blueprints = custom_target('blueprints',
  input: files(
    'gtk/help-overlay.blp',
    'exm-window.blp'
  ),
  output: '.',
  command: [find_program('blueprint-compiler'), 'batch-compile', '@OUTPUT@', '@CURRENT_SOURCE_DIR@', '@INPUT@'],
)

gnome = import('gnome')

exm_sources += gnome.compile_resources('exm-resources',
  'exm.gresource.xml',
  dependencies: blueprints,
  c_name: 'exm'
)

executable('extension-manager', exm_sources,
  dependencies: exm_deps,
  install: true,
)
