/* exm-unified-data.c
 *
 * Copyright 2022 Matthew Jakeman <mjakeman26@outlook.co.nz>
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

#include "exm-unified-data.h"

struct _ExmUnifiedData
{
    GObject parent_instance;

    ExmSearchResult *web_data;
    ExmExtension *local_data;
};

G_DEFINE_FINAL_TYPE (ExmUnifiedData, exm_unified_data, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_NAME,
    PROP_CREATOR,
    PROP_UUID,
    PROP_DESCRIPTION,
    PROP_SCREENSHOT_URI,
    PROP_LINK,
    PROP_HOMEPAGE,
    PROP_PK,
    PROP_SHELL_VERSION_MAP,
    PROP_DOWNLOADS,
    PROP_ICON_URI,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmUnifiedData *
exm_unified_data_new (void)
{
    return g_object_new (EXM_TYPE_UNIFIED_DATA, NULL);
}

static void
exm_unified_data_finalize (GObject *object)
{
    ExmUnifiedData *self = (ExmUnifiedData *)object;

    if (self->web_data)
        g_object_unref (self->web_data);

    if (self->local_data)
        g_object_unref (self->local_data);

    G_OBJECT_CLASS (exm_unified_data_parent_class)->finalize (object);
}

static void
exm_unified_data_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
    ExmUnifiedData *self = EXM_UNIFIED_DATA (object);

    switch (prop_id)
    {
    case PROP_NAME:
        g_value_set_string (value, exm_unified_data_get_name (self));
        break;
    case PROP_CREATOR:
        g_value_set_string (value, exm_unified_data_get_creator (self));
        break;
    case PROP_UUID:
        g_value_set_string (value, exm_unified_data_get_uuid (self));
        break;
    case PROP_DESCRIPTION:
        g_value_set_string (value, exm_unified_data_get_description (self));
        break;
    case PROP_SCREENSHOT_URI:
        {
            char *uri = NULL;
            exm_unified_data_get_screenshot_uri (self, &uri);
            g_value_set_string (value, uri);
        }
        break;
    case PROP_LINK:
        {
            char *uri = NULL;
            exm_unified_data_get_link (self, &uri);
            g_value_set_string (value, uri);
        }
        break;
    case PROP_HOMEPAGE:
        {
            char *uri = NULL;
            exm_unified_data_get_homepage (self, &uri);
            g_value_set_string (value, uri);
        }
        break;
    case PROP_PK:
        {
            int pk = 0;
            exm_unified_data_get_pk (self, &pk);
            g_value_set_int (value, pk);
        }
        break;
    case PROP_SHELL_VERSION_MAP:
        {
            ExmShellVersionMap *map;
            map = exm_unified_data_get_shell_version_map (self);
            g_value_set_object (value, map);
        }
        break;
    case PROP_DOWNLOADS:
        {
            int downloads = 0;
            exm_unified_data_get_downloads (self, &downloads);
            g_value_set_int (value, downloads);
        }
        break;
    case PROP_ICON_URI:
        {
            char *uri = NULL;
            exm_unified_data_get_icon_uri (self, &uri);
            g_value_set_string (value, uri);
        }
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_unified_data_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
    ExmUnifiedData *self = EXM_UNIFIED_DATA (object);

    switch (prop_id)
      {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      }
}

ExmExtension *
exm_unified_data_get_local_data (ExmUnifiedData *self)
{
    return self->local_data;
}

static void
notify_properties (ExmUnifiedData *self)
{
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_NAME]);
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_CREATOR]);
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_UUID]);
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_DESCRIPTION]);
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_SCREENSHOT_URI]);
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_LINK]);
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_HOMEPAGE]);
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_PK]);
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_SHELL_VERSION_MAP]);
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_DOWNLOADS]);
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_ICON_URI]);
}

void
exm_unified_data_set_local_data (ExmUnifiedData *self,
                                 ExmExtension   *extension)
{
    self->local_data = g_object_ref (extension);
    notify_properties (self);
}

ExmSearchResult *
exm_unified_data_get_web_data (ExmUnifiedData *self)
{
    return self->web_data;
}

void
exm_unified_data_set_web_data (ExmUnifiedData  *self,
                               ExmSearchResult *extension)
{
    self->web_data = g_object_ref (extension);
    notify_properties (self);
}

const char *
exm_unified_data_get_name (ExmUnifiedData *self)
{
    const char *name;

    if (self->web_data)
    {
        g_object_get (self->web_data, "name", &name, NULL);
        return name;
    }

    if (self->local_data)
    {
        g_object_get (self->local_data, "display-name", &name, NULL);
        return name;
    }

    return NULL;
}

const char *
exm_unified_data_get_creator (ExmUnifiedData *self)
{
    const char *creator;

    if (self->web_data)
    {
        g_object_get (self->web_data, "creator", &creator, NULL);
        return creator;
    }

    return NULL;
}

const char *
exm_unified_data_get_uuid (ExmUnifiedData *self)
{
    const char *uuid;

    if (self->web_data)
    {
        g_object_get (self->web_data, "uuid", &uuid, NULL);
        return uuid;
    }

    if (self->local_data)
    {
        g_object_get (self->local_data, "uuid", &uuid, NULL);
        return uuid;
    }

    return NULL;
}

const char *
exm_unified_data_get_description (ExmUnifiedData *self)
{
    const char *description;

    if (self->web_data)
    {
        g_object_get (self->web_data, "description", &description, NULL);
        return description;
    }

    if (self->local_data)
    {
        g_object_get (self->local_data, "description", &description, NULL);
        return description;
    }

    return NULL;
}

gboolean
exm_unified_data_get_screenshot_uri (ExmUnifiedData  *self,
                                     char           **uri)
{
    g_return_val_if_fail (uri != NULL, FALSE);

    *uri = NULL;

    if (self->web_data)
    {
        const char *tmp;

        g_object_get (self->web_data, "screenshot", &tmp, NULL);
        *uri = g_strdup (tmp);

        return TRUE;
    }

    return FALSE;
}

gboolean
exm_unified_data_get_link (ExmUnifiedData  *self,
                           char           **link)
{
    g_return_val_if_fail (link != NULL, FALSE);

    *link = NULL;

    if (self->web_data)
    {
        const char *tmp;

        g_object_get (self->web_data, "link", &tmp, NULL);
        *link = g_strdup (tmp);

        return TRUE;
    }

    return FALSE;
}

gboolean
exm_unified_data_get_homepage (ExmUnifiedData  *self,
                               char           **homepage)
{
    g_return_val_if_fail (homepage != NULL, FALSE);

    *homepage = NULL;

    if (self->web_data)
    {
        const char *tmp;

        g_object_get (self->web_data, "url", &tmp, NULL);
        *homepage = g_strdup (tmp);

        return TRUE;
    }

    return FALSE;
}

gboolean
exm_unified_data_get_pk (ExmUnifiedData *self,
                         int            *pk)
{
    g_return_val_if_fail (pk != NULL, FALSE);

    *pk = 0;

    if (self->web_data)
    {
        g_object_get (self->web_data, "pk", pk, NULL);
        return TRUE;
    }

    return FALSE;
}

gboolean
exm_unified_data_is_empty (ExmUnifiedData *self)
{
    return !(self->local_data || self->web_data);
}

ExmShellVersionMap *
exm_unified_data_get_shell_version_map (ExmUnifiedData *self)
{
    ExmShellVersionMap *version_map;

    if (self->web_data)
    {
        g_object_get (self->web_data, "shell_version_map", &version_map, NULL);
        return version_map;
    }

    return NULL;
}

gboolean
exm_unified_data_get_downloads (ExmUnifiedData *self,
                                int            *downloads)
{
    g_return_val_if_fail (downloads != NULL, FALSE);

    *downloads = 0;

    if (self->web_data)
    {
        g_object_get (self->web_data, "downloads", downloads, NULL);
        return TRUE;
    }

    return FALSE;
}

gboolean
exm_unified_data_get_icon_uri (ExmUnifiedData  *self,
                               char           **uri)
{
    g_return_val_if_fail (uri != NULL, FALSE);

    *uri = NULL;

    if (self->web_data)
    {
        const char *tmp;

        g_object_get (self->web_data, "icon", &tmp, NULL);
        *uri = g_strdup (tmp);

        return TRUE;
    }

    return FALSE;
}

static void
exm_unified_data_class_init (ExmUnifiedDataClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_unified_data_finalize;
    object_class->get_property = exm_unified_data_get_property;
    object_class->set_property = exm_unified_data_set_property;

    properties [PROP_NAME] =
        g_param_spec_string ("name", "Name", "Name", NULL, G_PARAM_READABLE);

    properties [PROP_CREATOR] =
        g_param_spec_string ("creator", "Creator", "Creator", NULL, G_PARAM_READABLE);

    properties [PROP_UUID] =
        g_param_spec_string ("uuid", "UUID", "UUID", NULL, G_PARAM_READABLE);

    properties [PROP_DESCRIPTION] =
        g_param_spec_string ("description", "Description", "Description", NULL, G_PARAM_READABLE);

    properties [PROP_SCREENSHOT_URI] =
        g_param_spec_string ("screenshot-uri", "Screenshot URI", "Screenshot URI", NULL, G_PARAM_READABLE);

    properties [PROP_LINK] =
        g_param_spec_string ("link", "Link", "Link", NULL, G_PARAM_READABLE);

    properties [PROP_HOMEPAGE] =
        g_param_spec_string ("homepage", "Homepage", "Homepage", NULL, G_PARAM_READABLE);

    properties [PROP_PK] =
        g_param_spec_int ("pk", "Pk", "Pk", 0, G_MAXINT32, 0, G_PARAM_READABLE);

    properties [PROP_SHELL_VERSION_MAP] =
        g_param_spec_boxed ("shell-version-map", "Shell Version Map", "Shell Version Map", EXM_TYPE_SHELL_VERSION_MAP, G_PARAM_READABLE);

    properties [PROP_DOWNLOADS] =
        g_param_spec_int ("downloads", "Downloads", "Downloads", 0, G_MAXINT32, 0, G_PARAM_READABLE);

    properties [PROP_ICON_URI] =
        g_param_spec_string ("icon-uri", "Icon URI", "Icon URI", NULL, G_PARAM_READABLE);

    g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
exm_unified_data_init (ExmUnifiedData *self)
{
    self->local_data = NULL;
    self->web_data = NULL;
}

