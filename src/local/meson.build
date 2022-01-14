exm_sources += files([
  # Extension Management
  'exm-manager.c',
  'exm-extension.c'
])

exm_sources += gnome.gdbus_codegen('shell-dbus-interface',
  sources: 'org.gnome.Shell.Extensions.xml',
  interface_prefix: 'org.gnome.Shell.',
  namespace: 'Shell',
  object_manager: true
)
