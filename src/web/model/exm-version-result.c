/*
 * exm-version-result.c
 *
 * Copyright 2022-2025 Matthew Jakeman <mjakeman26@outlook.co.nz>
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

#include "exm-version-result.h"

#include "exm-shell-versions.h"

#include <json-glib/json-glib.h>

struct _ExmVersionResult
{
    GObject parent_instance;

    gchar *extension;
    gint version;
    gchar *version_name;
    guint status;
    ExmShellVersions *shell_versions;
    gchar *created;
    gchar **session_modes;
};

G_DEFINE_FINAL_TYPE (ExmVersionResult, exm_version_result, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_EXTENSION,
    PROP_VERSION,
    PROP_VERSION_NAME,
    PROP_STATUS,
    PROP_SHELL_VERSIONS,
    PROP_CREATED,
    PROP_SESSION_MODES,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmVersionResult *
exm_version_result_new (void)
{
    return g_object_new (EXM_TYPE_VERSION_RESULT, NULL);
}

static void
exm_version_result_finalize (GObject *object)
{
    ExmVersionResult *self = (ExmVersionResult *)object;

    g_clear_pointer (&self->shell_versions, exm_shell_versions_unref);
    g_clear_pointer (&self->session_modes, g_strfreev);

    G_OBJECT_CLASS (exm_version_result_parent_class)->finalize (object);
}

static void
exm_version_result_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
    ExmVersionResult *self = EXM_VERSION_RESULT (object);

    switch (prop_id)
    {
    case PROP_EXTENSION:
        g_value_set_string (value, self->extension);
        break;
    case PROP_VERSION:
        g_value_set_int (value, self->version);
        break;
    case PROP_VERSION_NAME:
        g_value_set_string (value, self->version_name);
        break;
    case PROP_STATUS:
        g_value_set_uint (value, self->status);
        break;
    case PROP_SHELL_VERSIONS:
        g_value_set_boxed (value, self->shell_versions);
        break;
    case PROP_CREATED:
        g_value_set_string (value, self->created);
        break;
    case PROP_SESSION_MODES:
        g_value_set_boxed (value, self->session_modes);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_version_result_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
    ExmVersionResult *self = EXM_VERSION_RESULT (object);

    switch (prop_id)
    {
    case PROP_EXTENSION:
        g_free (self->extension);
        self->extension = g_value_dup_string (value);
        break;
    case PROP_VERSION:
        self->version = g_value_get_int (value);
        break;
    case PROP_VERSION_NAME:
        g_free (self->version_name);
        self->version_name = g_value_dup_string (value);
        break;
    case PROP_STATUS:
        self->status = g_value_get_uint (value);
        break;
    case PROP_SHELL_VERSIONS:
        if (self->shell_versions)
            g_clear_pointer (&self->shell_versions, exm_shell_versions_unref);

        self->shell_versions = exm_shell_versions_ref (g_value_get_boxed (value));
        break;
    case PROP_CREATED:
        g_free (self->created);
        self->created = g_value_dup_string (value);
        break;
    case PROP_SESSION_MODES:
        self->session_modes = g_value_dup_boxed (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

gboolean
exm_version_result_supports_shell_version (ExmVersionResult *self,
                                           const gchar      *shell_version)
{
    g_return_val_if_fail (self, FALSE);

    return exm_shell_versions_supports (self->shell_versions, shell_version);
}

static void
deserialize_version (JsonArray        *array G_GNUC_UNUSED,
                     guint             index G_GNUC_UNUSED,
                     JsonNode         *element_node,
                     ExmShellVersions *versions)
{
    JsonObject *version_obj;
    int major, minor, point;

    version_obj = json_node_get_object (element_node);

    major = json_object_get_int_member (version_obj, "major");
    minor = json_object_get_int_member (version_obj, "minor");
    point = json_object_get_int_member (version_obj, "point");

    exm_shell_versions_add (versions, major, minor, point);
}

static gpointer
deserialize_shell_versions (JsonNode* node)
{
    ExmShellVersions *versions;
    JsonArray *array;

    versions = exm_shell_versions_new ();

    g_assert (JSON_NODE_HOLDS_ARRAY (node));
    array = json_node_get_array (node);

    json_array_foreach_element (array, (JsonArrayForeach) deserialize_version, versions);

    return versions;
}

static void
exm_version_result_class_init (ExmVersionResultClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_version_result_finalize;
    object_class->get_property = exm_version_result_get_property;
    object_class->set_property = exm_version_result_set_property;

    properties [PROP_EXTENSION] =
        g_param_spec_string ("extension",
                             "Extension",
                             "Extension",
                             NULL,
                             G_PARAM_READWRITE);

    properties [PROP_VERSION] =
        g_param_spec_int ("version",
                          "Version",
                          "Version",
                          G_MININT, G_MAXINT, 0,
                          G_PARAM_READWRITE);

    properties [PROP_VERSION_NAME] =
        g_param_spec_string ("version_name",
                             "Version Name",
                             "Version Name",
                             NULL,
                             G_PARAM_READWRITE);

    properties [PROP_STATUS] =
        g_param_spec_uint ("status",
                           "Status",
                           "Status",
                           0, G_MAXUINT, 0,
                           G_PARAM_READWRITE);

    properties [PROP_SHELL_VERSIONS] =
        g_param_spec_boxed ("shell_versions",
                            "Shell Versions",
                            "Shell Versions",
                            EXM_TYPE_SHELL_VERSIONS,
                            G_PARAM_READWRITE);

    properties [PROP_CREATED] =
        g_param_spec_string ("created",
                             "Created",
                             "Created",
                             NULL,
                             G_PARAM_READWRITE);

    properties[PROP_SESSION_MODES] =
        g_param_spec_boxed ("session_modes",
                            "Session Modes",
                            "Session Modes",
                            G_TYPE_STRV,
                            G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    json_boxed_register_deserialize_func (EXM_TYPE_SHELL_VERSIONS,
                                          JSON_NODE_ARRAY,
                                          deserialize_shell_versions);
}

static void
exm_version_result_init (ExmVersionResult *self G_GNUC_UNUSED)
{
}
