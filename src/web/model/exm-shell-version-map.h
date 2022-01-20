#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define EXM_TYPE_SHELL_VERSION_MAP (exm_shell_version_map_get_type ())

typedef struct _ExmShellVersionMap ExmShellVersionMap;

GType                   exm_shell_version_map_get_type (void) G_GNUC_CONST;
ExmShellVersionMap     *exm_shell_version_map_new      (void);
ExmShellVersionMap     *exm_shell_version_map_copy     (ExmShellVersionMap *self);
void                    exm_shell_version_map_free     (ExmShellVersionMap *self);
void                    exm_shell_version_map_add      (ExmShellVersionMap *self,
                                                        const gchar        *shell_version,
                                                        int                 ext_package,
                                                        double              ext_version);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ExmShellVersionMap, exm_shell_version_map_free)

G_END_DECLS
