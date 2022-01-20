#include "exm-shell-version-map.h"

typedef struct
{
    gchar *shell_major_version;
    gchar *shell_minor_version;
    int extension_package;
    double extension_version;
} MapEntry;

G_DEFINE_BOXED_TYPE (ExmShellVersionMap, exm_shell_version_map, exm_shell_version_map_ref, exm_shell_version_map_unref)

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
    self->ref_count = 1;

    return self;
}

static void
free_entry (MapEntry *entry)
{
    g_free (entry->shell_major_version);

    if (entry->shell_minor_version)
        g_free (entry->shell_minor_version);

    g_slice_free (MapEntry, entry);
}

static void
exm_shell_version_map_free (ExmShellVersionMap *self)
{
    g_assert (self);
    g_assert_cmpint (self->ref_count, ==, 0);

    g_list_free_full (g_steal_pointer (&self->map), (GDestroyNotify) free_entry);

    g_slice_free (ExmShellVersionMap, self);
}

/**
 * exm_shell_version_map_ref:
 * @self: A #ExmShellVersionMap
 *
 * Increments the reference count of @self by one.
 *
 * Returns: (transfer full): @self
 */
ExmShellVersionMap *
exm_shell_version_map_ref (ExmShellVersionMap *self)
{
    g_return_val_if_fail (self, NULL);
    g_return_val_if_fail (self->ref_count, NULL);

    g_atomic_int_inc (&self->ref_count);

    return self;
}

/**
 * exm_shell_version_map_unref:
 * @self: A #ExmShellVersionMap
 *
 * Decrements the reference count of @self by one, freeing the structure when
 * the reference count reaches zero.
 */
void
exm_shell_version_map_unref (ExmShellVersionMap *self)
{
    g_return_if_fail (self);
    g_return_if_fail (self->ref_count);

    if (g_atomic_int_dec_and_test (&self->ref_count))
        exm_shell_version_map_free (self);
}

void
exm_shell_version_map_add (ExmShellVersionMap *self,
                           const gchar        *shell_version,
                           int                 ext_package,
                           double              ext_version)
{
    gchar **strarr;
    const gchar *major;
    const gchar *minor;

    strarr = g_strsplit (shell_version, ".", 2);

    major = strarr[0];
    minor = strarr[1];

    g_debug ("Parsed Version: %s as %s.%s\n", shell_version, major, minor);

    MapEntry *entry = g_slice_new0 (MapEntry);
    entry->shell_major_version = g_strdup (major);
    entry->shell_minor_version = g_strdup (minor);
    entry->extension_version = ext_version;
    entry->extension_package = ext_package;

    self->map = g_list_append (self->map, entry);
}

gboolean
exm_shell_version_map_supports (ExmShellVersionMap *self,
                                const gchar        *shell_version)
{
    // The shell_version string can be either in the form 3.32, 3.36,
    // 3.38, etc or it can be 40, 41, etc. As a rule, we return true
    // if the provided `shell_version` is equal or more specific to
    // the version string stored in the version map.

    gchar **strarr;
    GList *element;
    const gchar *major;
    const gchar *minor;

    g_return_val_if_fail (self->map, FALSE);

    strarr = g_strsplit (shell_version, ".", 2);

    major = strarr[0];
    minor = strarr[1];

    for (element = self->map;
         element != NULL;
         element = element->next)
    {
        MapEntry *entry = element->data;
        if (!g_str_equal (major, entry->shell_major_version))
            continue;

        if (!entry->shell_minor_version ||
            g_str_equal (entry->shell_minor_version, minor))
            return TRUE;
    }

    return FALSE;
}
