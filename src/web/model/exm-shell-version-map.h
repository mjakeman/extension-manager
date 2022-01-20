#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define EXM_TYPE_SHELL_VERSION_MAP (exm_shell_version_map_get_type())

G_DECLARE_FINAL_TYPE (ExmShellVersionMap, exm_shell_version_map, EXM, SHELL_VERSION_MAP, GObject)

ExmShellVersionMap *exm_shell_version_map_new (void);

G_END_DECLS
