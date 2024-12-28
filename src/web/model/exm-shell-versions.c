/*
 * exm-shell-versions.c
 *
 * Copyright 2022-2024 Matthew Jakeman <mjakeman26@outlook.co.nz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "exm-shell-versions.h"

G_DEFINE_BOXED_TYPE (ExmShellVersions, exm_shell_versions, exm_shell_versions_ref, exm_shell_versions_unref)

/**
 * exm_shell_versions_new:
 *
 * Creates a new #ExmShellVersions.
 *
 * Returns: (transfer full): A newly created #ExmShellVersions
 */
ExmShellVersions *
exm_shell_versions_new (void)
{
    ExmShellVersions *self;

    self = g_slice_new0 (ExmShellVersions);
    self->ref_count = 1;

    return self;
}

static void
free_entry (VersionsEntry *entry)
{
    g_free (entry->shell_major_version);

    if (entry->shell_minor_version)
        g_free (entry->shell_minor_version);

    g_slice_free (VersionsEntry, entry);
}

static void
exm_shell_versions_free (ExmShellVersions *self)
{
    g_assert (self);
    g_assert_cmpint (self->ref_count, ==, 0);

    g_list_free_full (g_steal_pointer (&self->versions), (GDestroyNotify) free_entry);

    g_slice_free (ExmShellVersions, self);
}

/**
 * exm_shell_versions_ref:
 * @self: A #ExmShellVersions
 *
 * Increments the reference count of @self by one.
 *
 * Returns: (transfer full): @self
 */
ExmShellVersions *
exm_shell_versions_ref (ExmShellVersions *self)
{
    g_return_val_if_fail (self, NULL);
    g_return_val_if_fail (self->ref_count, NULL);

    g_atomic_int_inc (&self->ref_count);

    return self;
}

/**
 * exm_shell_versions_unref:
 * @self: A #ExmShellVersions
 *
 * Decrements the reference count of @self by one, freeing the structure when
 * the reference count reaches zero.
 */
void
exm_shell_versions_unref (ExmShellVersions *self)
{
    g_return_if_fail (self);
    g_return_if_fail (self->ref_count);

    if (g_atomic_int_dec_and_test (&self->ref_count))
        exm_shell_versions_free (self);
}

void
exm_shell_versions_add (ExmShellVersions *self,
                        const gchar      *shell_version,
                        int               ext_package,
                        double            ext_version)
{
    gchar **strarr;
    const gchar *major;
    const gchar *minor;

    strarr = g_strsplit (shell_version, ".", 2);

    major = strarr[0];
    minor = strarr[1];

    g_debug ("Parsed Version: %s as %s.%s\n", shell_version, major, minor);

    VersionsEntry *entry = g_slice_new0 (VersionsEntry);
    entry->shell_major_version = g_strdup (major);
    entry->shell_minor_version = g_strdup (minor);
    entry->extension_version = ext_version;
    entry->extension_package = ext_package;

    self->versions = g_list_append (self->versions, entry);
}

gboolean
exm_shell_versions_supports (ExmShellVersions *self,
                             const gchar      *shell_version)
{
    // The shell_version string can be either in the form 3.32, 3.36,
    // 3.38, etc or it can be 40, 41, etc. As a rule, we return true
    // if the minor version of `shell_version` is equal or more specific
    // to the minor version string stored in the versions.

    gchar **strarr;
    GList *element;
    const gchar *major;
    const gchar *minor;

    // Some entries on the website do not define a shell_versions. Assume
    // these extensions have been retired and ignore.
    if (self->versions == NULL)
        return FALSE;

    strarr = g_strsplit (shell_version, ".", 2);

    major = strarr[0];
    minor = strarr[1];

    for (element = self->versions;
         element != NULL;
         element = element->next)
    {
        VersionsEntry *entry = element->data;

        if (!g_str_equal (major, entry->shell_major_version))
            continue;

        if (!entry->shell_minor_version ||
            g_str_has_prefix (minor, entry->shell_minor_version))
            return TRUE;
    }

    return FALSE;
}
