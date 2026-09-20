/*
 * exm-user-profile.c
 *
 * Copyright 2022-2026 Matthew Jakeman <mjakeman26@outlook.co.nz>
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

#include "exm-user-profile.h"

struct _ExmUserProfile
{
    GObject parent_instance;

    guint id;
    gchar *username;
    gchar *display_name;
    gchar *avatar;
};

G_DEFINE_FINAL_TYPE (ExmUserProfile, exm_user_profile, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_ID,
    PROP_USERNAME,
    PROP_DISPLAY_NAME,
    PROP_AVATAR,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmUserProfile *
exm_user_profile_new (void)
{
    return g_object_new (EXM_TYPE_USER_PROFILE, NULL);
}

static void
exm_user_profile_finalize (GObject *object)
{
    ExmUserProfile *self = (ExmUserProfile *)object;

    g_clear_pointer (&self->username, g_free);
    g_clear_pointer (&self->display_name, g_free);
    g_clear_pointer (&self->avatar, g_free);

    G_OBJECT_CLASS (exm_user_profile_parent_class)->finalize (object);
}

static void
exm_user_profile_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
    ExmUserProfile *self = EXM_USER_PROFILE (object);

    switch (prop_id)
    {
    case PROP_ID:
        g_value_set_uint (value, self->id);
        break;
    case PROP_USERNAME:
        g_value_set_string (value, self->username);
        break;
    case PROP_DISPLAY_NAME:
        g_value_set_string (value, self->display_name);
        break;
    case PROP_AVATAR:
        g_value_set_string (value, self->avatar);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_user_profile_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
    ExmUserProfile *self = EXM_USER_PROFILE (object);

    switch (prop_id)
    {
    case PROP_ID:
        self->id = g_value_get_uint (value);
        break;
    case PROP_USERNAME:
        g_free (self->username);
        self->username = g_value_dup_string (value);
        break;
    case PROP_DISPLAY_NAME:
        g_free (self->display_name);
        self->display_name = g_value_dup_string (value);
        break;
    case PROP_AVATAR:
        g_free (self->avatar);
        self->avatar = g_value_dup_string (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_user_profile_class_init (ExmUserProfileClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize     = exm_user_profile_finalize;
    object_class->get_property = exm_user_profile_get_property;
    object_class->set_property = exm_user_profile_set_property;

    properties [PROP_ID] =
        g_param_spec_uint ("id",
                           "ID",
                           "ID",
                           0, G_MAXUINT, 0,
                           G_PARAM_READWRITE);

    properties [PROP_USERNAME] =
        g_param_spec_string ("username",
                             "Username",
                             "Username",
                             NULL,
                             G_PARAM_READWRITE);

    properties [PROP_DISPLAY_NAME] =
        g_param_spec_string ("display_name",
                             "Display Name",
                             "Display Name",
                             NULL,
                             G_PARAM_READWRITE);

    properties [PROP_AVATAR] =
        g_param_spec_string ("avatar",
                             "Avatar",
                             "Avatar",
                             NULL,
                             G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
exm_user_profile_init (ExmUserProfile *self G_GNUC_UNUSED)
{
}
