#include "exm-shell-version-map.h"

typedef struct
{
    int package;
    double version;
} VersionTuple;

G_DEFINE_BOXED_TYPE (ExmShellVersionMap, exm_shell_version_map, exm_shell_version_map_copy, exm_shell_version_map_free)

struct _ExmShellVersionMap
{
    GHashTable *table;
};

/**
 * exm_shell_version_map_new:
 *
 * Creates a new #ExmShellVersionMap.
 *
 * Returns: (transfer full): A newly created #ExmShellVersionMap
 */
ExmShellVersionMap *
exm_shell_version_map_new (void)
{
    ExmShellVersionMap *self;

    self = g_slice_new0 (ExmShellVersionMap);

    self->table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

    return self;
}

/**
 * exm_shell_version_map_copy:
 * @self: a #ExmShellVersionMap
 *
 * Makes a deep copy of a #ExmShellVersionMap.
 *
 * Returns: (transfer full): A newly created #ExmShellVersionMap with the same
 *   contents as @self
 */
ExmShellVersionMap *
exm_shell_version_map_copy (ExmShellVersionMap *self)
{
    ExmShellVersionMap *copy;

    g_return_val_if_fail (self, NULL);

    copy = exm_shell_version_map_new ();

    return copy;
}

/**
 * exm_shell_version_map_free:
 * @self: a #ExmShellVersionMap
 *
 * Frees a #ExmShellVersionMap allocated using exm_shell_version_map_new()
 * or exm_shell_version_map_copy().
 */
void
exm_shell_version_map_free (ExmShellVersionMap *self)
{
    g_return_if_fail (self);

    g_slice_free (ExmShellVersionMap, self);
}

void
exm_shell_version_map_add (ExmShellVersionMap *self,
                           const gchar        *shell_version,
                           int                 ext_package,
                           double              ext_version)
{
    VersionTuple *tuple = g_new0 (VersionTuple, 1);
    tuple->package = ext_package;
    tuple->version = ext_version;

    g_hash_table_insert (self->table, g_strdup (shell_version), tuple);
}
