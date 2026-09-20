/*
 * exm-unified-data.c
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
    ExmExtension    *local_data;
};

G_DEFINE_FINAL_TYPE (ExmUnifiedData, exm_unified_data, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_NAME,
    PROP_CREATOR,
    PROP_CREATOR_USERNAME,
    PROP_CREATOR_ID,
    PROP_UUID,
    PROP_DESCRIPTION,
    PROP_SCREENSHOT_URI,
    PROP_LINK,
    PROP_HOMEPAGE,
    PROP_ID,
    PROP_DOWNLOADS,
    PROP_ICON_URI,
    PROP_DONATION_URLS,
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

    g_clear_object (&self->web_data);
    g_clear_object (&self->local_data);

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
        {
            char *name = NULL;
            if (self->web_data)
                g_object_get (self->web_data, "name", &name, NULL);
            else if (self->local_data)
                g_object_get (self->local_data, "name", &name, NULL);
            g_value_take_string (value, name);
        }
        break;
    case PROP_CREATOR:
        {
            char *creator = NULL;
            if (self->web_data)
                g_object_get (self->web_data, "creator", &creator, NULL);
            g_value_take_string (value, creator);
        }
        break;
    case PROP_CREATOR_USERNAME:
        {
            char *username = NULL;
            if (self->web_data)
                g_object_get (self->web_data, "creator-username", &username, NULL);
            g_value_take_string (value, username);
        }
        break;
    case PROP_CREATOR_ID:
        {
            guint creator_id = 0;
            exm_unified_data_get_creator_id (self, &creator_id);
            g_value_set_uint (value, creator_id);
        }
        break;
    case PROP_UUID:
        {
            char *uuid = NULL;
            if (self->web_data)
                g_object_get (self->web_data, "uuid", &uuid, NULL);
            else if (self->local_data)
                g_object_get (self->local_data, "uuid", &uuid, NULL);
            g_value_take_string (value, uuid);
        }
        break;
    case PROP_DESCRIPTION:
        {
            char *description = NULL;
            if (self->web_data)
                g_object_get (self->web_data, "description", &description, NULL);
            else if (self->local_data)
                g_object_get (self->local_data, "description", &description, NULL);
            g_value_take_string (value, description);
        }
        break;
    case PROP_SCREENSHOT_URI:
        {
            char *uri = NULL;
            exm_unified_data_get_screenshot_uri (self, &uri);
            g_value_take_string (value, uri);
        }
        break;
    case PROP_LINK:
        {
            char *link = NULL;
            exm_unified_data_get_link (self, &link);
            g_value_take_string (value, link);
        }
        break;
    case PROP_HOMEPAGE:
        {
            char *homepage = NULL;
            exm_unified_data_get_homepage (self, &homepage);
            g_value_take_string (value, homepage);
        }
        break;
    case PROP_ID:
        {
            guint id = 0;
            exm_unified_data_get_id (self, &id);
            g_value_set_uint (value, id);
        }
        break;
    case PROP_DOWNLOADS:
        {
            guint downloads = 0;
            exm_unified_data_get_downloads (self, &downloads);
            g_value_set_uint (value, downloads);
        }
        break;
    case PROP_ICON_URI:
        {
            char *uri = NULL;
            exm_unified_data_get_icon_uri (self, &uri);
            g_value_take_string (value, uri);
        }
        break;
    case PROP_DONATION_URLS:
        {
            char **urls = NULL;
            exm_unified_data_get_donation_urls (self, &urls);
            g_value_take_boxed (value, urls);
        }
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_unified_data_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value G_GNUC_UNUSED,
                               GParamSpec   *pspec)
{
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
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
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_CREATOR_USERNAME]);
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_CREATOR_ID]);
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_UUID]);
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_DESCRIPTION]);
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_SCREENSHOT_URI]);
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_LINK]);
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_HOMEPAGE]);
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_ID]);
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_DOWNLOADS]);
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_ICON_URI]);
    g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_DONATION_URLS]);
}

void
exm_unified_data_set_local_data (ExmUnifiedData *self,
                                 ExmExtension   *extension)
{
    g_set_object (&self->local_data, extension);
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
    g_set_object (&self->web_data, extension);
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
        g_object_get (self->local_data, "name", &name, NULL);
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
exm_unified_data_get_creator_username (ExmUnifiedData *self)
{
    const char *username;

    if (self->web_data)
    {
        g_object_get (self->web_data, "creator-username", &username, NULL);
        return username;
    }

    return NULL;
}

gboolean
exm_unified_data_get_creator_id (ExmUnifiedData *self,
                                 guint          *creator_id)
{
    g_return_val_if_fail (creator_id != NULL, FALSE);

    *creator_id = 0;

    if (self->web_data)
    {
        g_object_get (self->web_data, "creator-id", creator_id, NULL);
        return TRUE;
    }

    return FALSE;
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

    if (self->local_data)
    {
        char *tmp = NULL;
        g_object_get (self->local_data, "url", &tmp, NULL);

        if (tmp != NULL && tmp[0] != '\0')
        {
            *homepage = tmp;
            return TRUE;
        }

        g_free (tmp);
    }

    return FALSE;
}

gboolean
exm_unified_data_get_id (ExmUnifiedData *self,
                         guint          *id)
{
    g_return_val_if_fail (id != NULL, FALSE);

    *id = 0;

    if (self->web_data)
    {
        g_object_get (self->web_data, "id", id, NULL);
        return TRUE;
    }

    return FALSE;
}

gboolean
exm_unified_data_is_empty (ExmUnifiedData *self)
{
    return !(self->local_data || self->web_data);
}

gboolean
exm_unified_data_get_downloads (ExmUnifiedData *self,
                                guint          *downloads)
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
add_local_donation_url (GPtrArray   *urls,
                        const char  *platform,
                        const char  *handle)
{
    static const struct { const char *key; const char *url_format; } known[] = {
        { "paypal",         "https://paypal.me/%s" },
        { "kofi",           "https://ko-fi.com/%s" },
        { "liberapay",      "https://liberapay.com/%s" },
        { "patreon",        "https://patreon.com/%s" },
        { "opencollective", "https://opencollective.com/%s" },
        { "buymeacoffee",   "https://buymeacoffee.com/%s" },
        { "github",         "https://github.com/sponsors/%s" },
    };

    if (handle == NULL || handle[0] == '\0')
        return;

    if (g_strcmp0 (platform, "custom") == 0)
    {
        g_ptr_array_add (urls, g_strdup (handle));
        return;
    }

    for (gsize i = 0; i < G_N_ELEMENTS (known); i++)
    {
        if (g_strcmp0 (platform, known[i].key) == 0)
        {
            g_ptr_array_add (urls, g_strdup_printf (known[i].url_format, handle));
            return;
        }
    }
}

static char **
build_local_donation_urls (GHashTable *donations)
{
    GPtrArray *urls;
    GHashTableIter iter;
    gpointer key, value;

    urls = g_ptr_array_new_with_free_func (g_free);

    g_hash_table_iter_init (&iter, donations);
    while (g_hash_table_iter_next (&iter, &key, &value))
    {
        const char *platform = key;
        GPtrArray  *handles = value;

        for (guint i = 0; i < handles->len; i++)
            add_local_donation_url (urls, platform, g_ptr_array_index (handles, i));
    }

    if (urls->len == 0)
    {
        g_ptr_array_free (urls, TRUE);
        return NULL;
    }

    g_ptr_array_add (urls, NULL);
    return (char **) g_ptr_array_free (urls, FALSE);
}

gboolean
exm_unified_data_get_donation_urls (ExmUnifiedData   *self,
                                    char            ***donation_urls)
{
    g_return_val_if_fail (donation_urls != NULL, FALSE);

    *donation_urls = NULL;

    if (self->web_data)
    {
        g_object_get (self->web_data, "donation_urls", donation_urls, NULL);
        return TRUE;
    }

    if (self->local_data)
    {
        GHashTable *donations = NULL;
        g_object_get (self->local_data, "donations", &donations, NULL);

        if (donations != NULL)
        {
            *donation_urls = build_local_donation_urls (donations);
            if (*donation_urls != NULL)
                return TRUE;
        }
    }

    return FALSE;
}

gboolean
exm_unified_data_get_session_modes (ExmUnifiedData   *self,
                                    char            ***session_modes)
{
    GPtrArray *modes = NULL;

    g_return_val_if_fail (session_modes != NULL, FALSE);

    *session_modes = NULL;

    if (!self->local_data)
        return FALSE;

    g_object_get (self->local_data, "session-modes", &modes, NULL);

    if (modes == NULL || modes->len == 0)
    {
        g_clear_pointer (&modes, g_ptr_array_unref);
        return FALSE;
    }

    char **strv = g_new0 (char *, modes->len + 1);
    for (guint i = 0; i < modes->len; i++)
        strv[i] = g_strdup (g_ptr_array_index (modes, i));

    g_ptr_array_unref (modes);

    *session_modes = strv;
    return TRUE;
}

static void
exm_unified_data_class_init (ExmUnifiedDataClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize     = exm_unified_data_finalize;
    object_class->get_property = exm_unified_data_get_property;
    object_class->set_property = exm_unified_data_set_property;

    properties [PROP_NAME] =
        g_param_spec_string ("name", "Name", "Name",
                             NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

    properties [PROP_CREATOR] =
        g_param_spec_string ("creator", "Creator", "Creator",
                             NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

    properties [PROP_CREATOR_USERNAME] =
        g_param_spec_string ("creator-username", "Creator Username", "Creator Username",
                             NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

    properties [PROP_CREATOR_ID] =
        g_param_spec_uint ("creator-id", "Creator ID", "Creator ID",
                           0, G_MAXUINT, 0,
                           G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

    properties [PROP_UUID] =
        g_param_spec_string ("uuid", "UUID", "UUID",
                             NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

    properties [PROP_DESCRIPTION] =
        g_param_spec_string ("description", "Description", "Description",
                             NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

    properties [PROP_SCREENSHOT_URI] =
        g_param_spec_string ("screenshot-uri", "Screenshot URI", "Screenshot URI",
                             NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

    properties [PROP_LINK] =
        g_param_spec_string ("link", "Link", "Link",
                             NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

    properties [PROP_HOMEPAGE] =
        g_param_spec_string ("homepage", "Homepage", "Homepage",
                             NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

    properties [PROP_ID] =
        g_param_spec_uint ("id", "ID", "ID",
                           0, G_MAXUINT, 0,
                           G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

    properties [PROP_DOWNLOADS] =
        g_param_spec_uint ("downloads", "Downloads", "Downloads",
                           0, G_MAXUINT, 0,
                           G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

    properties [PROP_ICON_URI] =
        g_param_spec_string ("icon-uri", "Icon URI", "Icon URI",
                             NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

    properties [PROP_DONATION_URLS] =
        g_param_spec_boxed ("donation-urls", "Donation URLs", "Donation URLs",
                            G_TYPE_STRV,
                            G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
exm_unified_data_init (ExmUnifiedData *self)
{
    self->local_data = NULL;
    self->web_data   = NULL;
}
