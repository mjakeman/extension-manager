#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define EXM_TYPE_SHELL_VERSION_MAP (exm_shell_version_map_get_type ())

typedef struct _ExmShellVersionMap ExmShellVersionMap;

struct _ExmShellVersionMap
{
    /*< private >*/
    guint ref_count;
    GList *map;

};

GType                   exm_shell_version_map_get_type (void) G_GNUC_CONST;
ExmShellVersionMap     *exm_shell_version_map_new      (void);
ExmShellVersionMap     *exm_shell_version_map_ref      (ExmShellVersionMap *self);
void                    exm_shell_version_map_unref    (ExmShellVersionMap *self);
void                    exm_shell_version_map_add      (ExmShellVersionMap *self,
                                                        const gchar        *shell_version,
                                                        int                 ext_package,
                                                        double              ext_version);
gboolean                exm_shell_version_map_supports (ExmShellVersionMap *self,
                                                        const gchar        *shell_version);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ExmShellVersionMap, exm_shell_version_map_unref)

G_END_DECLS
