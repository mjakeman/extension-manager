/*
 * exm-extension.c
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

#include "exm-extension.h"

#include "../exm-types.h"
#include "../exm-enums.h"

struct _ExmExtension
{
    GObject parent_instance;

    gchar *uuid;
    gchar *name;
    gchar *description;
    ExmExtensionState state;
    gboolean enabled;
    gchar *url;
    gchar *version;
    gchar *error;
    gboolean has_prefs;
    gboolean has_update;
    gboolean can_change;
    gboolean is_user;
    GPtrArray *session_modes;
    GHashTable *donations;
};

G_DEFINE_FINAL_TYPE (ExmExtension, exm_extension, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_UUID,
    PROP_NAME,
    PROP_DESCRIPTION,
    PROP_STATE,
    PROP_ENABLED,
    PROP_URL,
    PROP_VERSION,
    PROP_ERROR,
    PROP_HAS_PREFS,
    PROP_HAS_UPDATE,
    PROP_CAN_CHANGE,
    PROP_IS_USER,
    PROP_SESSION_MODES,
    PROP_DONATIONS,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmExtension *
exm_extension_new (const gchar *uuid)
{
    return g_object_new (EXM_TYPE_EXTENSION,
                         "uuid", uuid,
                         NULL);
}

static void
exm_extension_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
    ExmExtension *self = EXM_EXTENSION (object);

    switch (prop_id)
    {
    case PROP_UUID:
        g_value_set_string (value, self->uuid);
        break;
    case PROP_NAME:
        g_value_set_string (value, self->name);
        break;
    case PROP_DESCRIPTION:
        g_value_set_string (value, self->description);
        break;
    case PROP_STATE:
        g_value_set_enum (value, self->state);
        break;
    case PROP_ENABLED:
        g_value_set_boolean (value, self->enabled);
        break;
    case PROP_URL:
        g_value_set_string (value, self->url);
        break;
    case PROP_VERSION:
        g_value_set_string (value, self->version);
        break;
    case PROP_ERROR:
        g_value_set_string (value, self->error);
        break;
    case PROP_HAS_PREFS:
        g_value_set_boolean (value, self->has_prefs);
        break;
    case PROP_HAS_UPDATE:
        g_value_set_boolean (value, self->has_update);
        break;
    case PROP_CAN_CHANGE:
        g_value_set_boolean (value, self->can_change);
        break;
    case PROP_IS_USER:
        g_value_set_boolean (value, self->is_user);
        break;
    case PROP_SESSION_MODES:
        g_value_set_boxed (value, self->session_modes);
        break;
    case PROP_DONATIONS:
        g_value_set_pointer (value, self->donations);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_extension_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
    ExmExtension *self = EXM_EXTENSION (object);

    switch (prop_id)
    {
    case PROP_UUID:
        g_free (self->uuid);
        self->uuid = g_value_dup_string (value);
        break;
    case PROP_NAME:
        g_free (self->name);
        self->name = g_value_dup_string (value);
        break;
    case PROP_DESCRIPTION:
        g_free (self->description);
        self->description = g_value_dup_string (value);
        break;
    case PROP_STATE:
        self->state = g_value_get_enum (value);
        break;
    case PROP_ENABLED:
        self->enabled = g_value_get_boolean (value);
        break;
    case PROP_URL:
        g_free (self->url);
        self->url = g_value_dup_string (value);
        break;
    case PROP_VERSION:
        g_free (self->version);
        self->version = g_value_dup_string (value);
        break;
    case PROP_ERROR:
        g_free (self->error);
        self->error = g_value_dup_string (value);
        break;
    case PROP_HAS_PREFS:
        self->has_prefs = g_value_get_boolean (value);
        break;
    case PROP_HAS_UPDATE:
        self->has_update = g_value_get_boolean (value);
        break;
    case PROP_CAN_CHANGE:
        self->can_change = g_value_get_boolean (value);
        break;
    case PROP_IS_USER:
        self->is_user = g_value_get_boolean (value);
        break;
    case PROP_SESSION_MODES:
        if (self->session_modes)
            g_ptr_array_unref (self->session_modes);
        self->session_modes = g_value_get_boxed (value);
        break;
    case PROP_DONATIONS:
        if (self->donations)
            g_hash_table_unref (self->donations);
        self->donations = g_value_get_pointer (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_extension_class_init (ExmExtensionClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->get_property = exm_extension_get_property;
    object_class->set_property = exm_extension_set_property;

    properties [PROP_UUID] =
        g_param_spec_string ("uuid",
                             "UUID",
                             "UUID",
                             NULL,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_NAME] =
        g_param_spec_string ("name",
                             "Name",
                             "Name",
                             NULL,
                             G_PARAM_READWRITE);

    properties [PROP_DESCRIPTION] =
        g_param_spec_string ("description",
                             "Description",
                             "Description",
                             NULL,
                             G_PARAM_READWRITE);

    properties [PROP_STATE] =
        g_param_spec_enum ("state",
                           "State",
                           "State",
                           EXM_TYPE_EXTENSION_STATE,
                           EXM_EXTENSION_STATE_INITIALIZED,
                           G_PARAM_READWRITE);

    properties [PROP_ENABLED] =
        g_param_spec_boolean ("enabled",
                              "Enabled",
                              "Enabled",
                              FALSE,
                              G_PARAM_READWRITE);

    properties [PROP_URL] =
        g_param_spec_string ("url",
                             "URL",
                             "URL",
                             NULL,
                             G_PARAM_READWRITE);

    properties [PROP_VERSION] =
        g_param_spec_string ("version",
                             "Version",
                             "Version",
                             NULL,
                             G_PARAM_READWRITE);

    properties [PROP_ERROR] =
        g_param_spec_string ("error",
                             "Error",
                             "Error",
                             NULL,
                             G_PARAM_READWRITE);

    properties [PROP_HAS_PREFS] =
        g_param_spec_boolean ("has-prefs",
                              "Has Preferences",
                              "Has Preferences",
                              FALSE,
                              G_PARAM_READWRITE);

    properties [PROP_HAS_UPDATE] =
        g_param_spec_boolean ("has-update",
                              "Has Update",
                              "Has Update",
                              FALSE,
                              G_PARAM_READWRITE);

    properties [PROP_CAN_CHANGE] =
        g_param_spec_boolean ("can-change",
                              "Can Change",
                              "Can Change",
                              FALSE,
                              G_PARAM_READWRITE);

    properties [PROP_IS_USER] =
        g_param_spec_boolean ("is-user",
                              "Is User",
                              "Is User",
                              FALSE,
                              G_PARAM_READWRITE);

    properties [PROP_SESSION_MODES] =
        g_param_spec_boxed ("session-modes",
                            "Session Modes",
                            "Session Modes",
                            G_TYPE_PTR_ARRAY,
                            G_PARAM_READWRITE);

    properties [PROP_DONATIONS] =
        g_param_spec_pointer ("donations",
                              "Donations",
                              "Donations",
                              G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
exm_extension_init (ExmExtension *self G_GNUC_UNUSED)
{
}
