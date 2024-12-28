/*
 * exm-versions-result.c
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

#include "exm-versions-result.h"

#include "exm-shell-versions.h"

#include <json-glib/json-glib.h>

struct _ExmVersionsResult
{
    GObject parent_instance;

    gchar            *extension;
    int               version;
    int               status;
    ExmShellVersions  shell_versions;
    gchar            *created;
};

enum {
    PROP_0,
    PROP_EXTENSION,
    PROP_VERSION,
    PROP_STATUS,
    PROP_SHELL_VERSIONS,
    PROP_CREATED,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmVersionsResult *
exm_versions_result_new (void)
{
    return g_object_new (EXM_TYPE_VERSIONS_RESULT, NULL);
}

static void
exm_versions_result_finalize (GObject *object)
{
    ExmVersionsResult *self = (ExmVersionsResult *)object;

    g_clear_pointer (&self->shell_versions, exm_shell_versions_unref);

    G_OBJECT_CLASS (exm_versions_result_parent_class)->finalize (object);
}

static void
exm_versions_result_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
    ExmVersionsResult *self = EXM_VERSIONS_RESULT (object);

    switch (prop_id)
    {
    case PROP_EXTENSION:
        g_value_set_string (value, self->extension);
        break;
    case PROP_VERSION:
        g_value_set_int (value, self->version);
        break;
    case PROP_STATUS:
        g_value_set_int (value, self->status);
        break;
    case PROP_SHELL_VERSIONS:
        g_value_set_boxed (value, self->shell_versions);
        break;
    case PROP_CREATED:
        g_value_set_string (value, self->created);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_versions_result_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
    ExmVersionsResult *self = EXM_VERSIONS_RESULT (object);

    switch (prop_id)
    {
    case PROP_EXTENSION:
        self->extension = g_value_dup_string (value);
        break;
    case PROP_VERSION:
        self->version = g_value_get_int (value);
        break;
    case PROP_STATUS:
        self->status = g_value_get_int (value);
        break;
    case PROP_SHELL_VERSIONS:
        if (self->shell_versions)
            g_clear_pointer (&self->shell_versions, exm_shell_versions_unref);

        self->shell_versions = exm_shell_versions_ref (g_value_get_boxed (value));
        break;
    case PROP_CREATED:
        self->created = g_value_dup_string (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

gboolean
exm_versions_result_supports_shell_version (ExmVersionsResult *self,
                                            const gchar       *shell_version)
{
    g_return_val_if_fail (shell_version, FALSE);

    return exm_shell_versions_supports (self->shell_versions,
                                        shell_version);
}

static void
deserialize_version (JsonObject         *object,
                     const gchar        *shell_version,
                     JsonNode           *member_node,
                     ExmShellVersions   *versions)
{
    int package;
    double version;
    JsonObject *version_obj;

    version_obj = json_node_get_object (member_node);

    package = json_object_get_int_member (version_obj, "id");
    version = json_object_get_double_member (version_obj, "version");

    exm_shell_versions_add (versions, shell_version, package, version);
}

static gpointer
deserialize_shell_versions (JsonNode* node)
{
    ExmShellVersions *versions;
    JsonObject *object;

    versions = exm_shell_versions_new ();

    g_assert (JSON_NODE_HOLDS_OBJECT (node));
    object = json_node_get_object (node);

    json_object_foreach_member (object, (JsonObjectForeach) deserialize_version, versions);

    return versions;
}

static void
exm_versions_result_class_init (ExmVersionsResultClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_versions_result_finalize;
    object_class->get_property = exm_versions_result_get_property;
    object_class->set_property = exm_versions_result_set_property;

    properties [PROP_EXTENSION] =
        g_param_spec_string ("extension",
                             "Extension UUID",
                             "Extension UUID",
                             NULL,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_VERSION] =
        g_param_spec_int ("version",
                          "Version",
                          "Version",
                          0, G_MAXINT, 0,
                          G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_STATUS] =
        g_param_spec_int ("status",
                          "Status",
                          "Status",
                          0, G_MAXINT, 0,
                          G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_SHELL_VERSIONS] =
        g_param_spec_boxed ("shell_versions",
                             "Shell Versions",
                             "Shell Versions",
                             EXM_TYPE_SHELL_VERSIONS,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_CREATED] =
        g_param_spec_string ("created",
                             "Created",
                             "Created",
                             NULL,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    json_boxed_register_deserialize_func (EXM_TYPE_SHELL_VERSIONS,
                                          JSON_NODE_OBJECT,
                                          deserialize_shell_versions);
}

static void
exm_versions_result_init (ExmVersionsResult *self)
{
}
