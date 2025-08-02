/*
 * exm-search-result.c
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

#include "exm-search-result.h"

#include <json-glib/json-glib.h>

struct _ExmSearchResult
{
    GObject parent_instance;

    guint id;
    gchar *uuid;
    gchar *name;
    gchar *creator;
    gchar *description;
    gchar *created;
    gchar *updated;
    guint downloads;
    gint popularity;
    gchar *screenshot;
    gchar *icon;
    gdouble rating;
    gint rated;
    gchar *url;
    gchar **donation_urls;
    gchar *link;
};

static JsonSerializableIface *serializable_iface = NULL;

static void json_serializable_iface_init (JsonSerializableIface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (ExmSearchResult, exm_search_result, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (JSON_TYPE_SERIALIZABLE,
                                                      json_serializable_iface_init))

enum {
    PROP_0,
    PROP_ID,
    PROP_UUID,
    PROP_NAME,
    PROP_CREATOR,
    PROP_DESCRIPTION,
    PROP_CREATED,
    PROP_UPDATED,
    PROP_DOWNLOADS,
    PROP_POPULARITY,
    PROP_SCREENSHOT,
    PROP_ICON,
    PROP_RATING,
    PROP_RATED,
    PROP_URL,
    PROP_DONATION_URLS,
    PROP_LINK,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmSearchResult *
exm_search_result_new (void)
{
    return g_object_new (EXM_TYPE_SEARCH_RESULT, NULL);
}

static void
exm_search_result_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
    ExmSearchResult *self = EXM_SEARCH_RESULT (object);

    switch (prop_id)
    {
    case PROP_ID:
        g_value_set_uint (value, self->id);
        break;
    case PROP_UUID:
        g_value_set_string (value, self->uuid);
        break;
    case PROP_NAME:
        g_value_set_string (value, self->name);
        break;
    case PROP_CREATOR:
        g_value_set_string (value, self->creator);
        break;
    case PROP_DESCRIPTION:
        g_value_set_string (value, self->description);
        break;
    case PROP_CREATED:
        g_value_set_string (value, self->created);
        break;
    case PROP_UPDATED:
        g_value_set_string (value, self->updated);
        break;
    case PROP_DOWNLOADS:
        g_value_set_uint (value, self->downloads);
        break;
    case PROP_POPULARITY:
        g_value_set_int (value, self->popularity);
        break;
    case PROP_SCREENSHOT:
        g_value_set_string (value, self->screenshot);
        break;
    case PROP_ICON:
        g_value_set_string (value, self->icon);
        break;
    case PROP_RATING:
        g_value_set_double (value, self->rating);
        break;
    case PROP_RATED:
        g_value_set_int (value, self->rated);
        break;
    case PROP_URL:
        g_value_set_string (value, self->url);
        break;
    case PROP_DONATION_URLS:
        g_value_set_boxed (value, self->donation_urls);
        break;
    case PROP_LINK:
        g_value_set_string (value, self->link);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_search_result_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
    ExmSearchResult *self = EXM_SEARCH_RESULT (object);

    switch (prop_id)
    {
    case PROP_ID:
        self->id = g_value_get_uint (value);
        break;
    case PROP_UUID:
        self->uuid = g_value_dup_string (value);
        break;
    case PROP_NAME:
        self->name = g_value_dup_string (value);
        break;
    case PROP_CREATOR:
        self->creator = g_value_dup_string (value);
        break;
    case PROP_DESCRIPTION:
        self->description = g_value_dup_string (value);
        break;
    case PROP_CREATED:
        self->created = g_value_dup_string (value);
        break;
    case PROP_UPDATED:
        self->updated = g_value_dup_string (value);
        break;
    case PROP_DOWNLOADS:
        self->downloads = g_value_get_uint (value);
        break;
    case PROP_POPULARITY:
        self->popularity = g_value_get_int (value);
        break;
    case PROP_SCREENSHOT:
        self->screenshot = g_value_dup_string (value);
        break;
    case PROP_ICON:
        self->icon = g_value_dup_string (value);
        break;
    case PROP_RATING:
        self->rating = g_value_get_double (value);
        break;
    case PROP_RATED:
        self->rated = g_value_get_int (value);
        break;
    case PROP_URL:
        self->url = g_value_dup_string (value);
        break;
    case PROP_DONATION_URLS:
        self->donation_urls = g_value_dup_boxed (value);
        break;
    case PROP_LINK:
        self->link = g_value_dup_string (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_search_result_class_init (ExmSearchResultClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->get_property = exm_search_result_get_property;
    object_class->set_property = exm_search_result_set_property;

    properties [PROP_ID] =
        g_param_spec_uint ("id",
                           "ID",
                           "ID",
                           0, G_MAXUINT, 0,
                           G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

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
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_CREATOR] =
        g_param_spec_string ("creator",
                             "Creator",
                             "Creator",
                             NULL,
                             G_PARAM_READWRITE);

    properties [PROP_DESCRIPTION] =
        g_param_spec_string ("description",
                             "Description",
                             "Description",
                             NULL,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_CREATED] =
        g_param_spec_string ("created",
                             "Created",
                             "Created",
                             NULL,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_UPDATED] =
        g_param_spec_string ("updated",
                             "Updated",
                             "Updated",
                             NULL,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_DOWNLOADS] =
        g_param_spec_uint ("downloads",
                           "Downloads",
                           "Downloads",
                           0, G_MAXUINT, 0,
                           G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_POPULARITY] =
        g_param_spec_int ("popularity",
                          "Popularity",
                          "Popularity",
                          G_MININT, G_MAXINT, 0,
                          G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_ICON] =
        g_param_spec_string ("icon",
                             "Icon",
                             "Icon",
                             NULL,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_SCREENSHOT] =
        g_param_spec_string ("screenshot",
                             "Screenshot",
                             "Screenshot",
                             NULL,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_RATING] =
        g_param_spec_double ("rating",
                             "Rating",
                             "Rating",
                             0.0, G_MAXDOUBLE, 0.0,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_RATED] =
        g_param_spec_int ("rated",
                          "Rated",
                          "Rated",
                          G_MININT, G_MAXINT, 0,
                          G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_URL] =
        g_param_spec_string ("url",
                             "Url",
                             "Url",
                             NULL,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties[PROP_DONATION_URLS] =
        g_param_spec_boxed ("donation_urls",
                            "Donation URLs",
                            "Donation URLs",
                            G_TYPE_STRV,
                            G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_LINK] =
        g_param_spec_string ("link",
                             "Link",
                             "Link",
                             NULL,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
exm_search_result_init (ExmSearchResult *self G_GNUC_UNUSED)
{
}

static gboolean
exm_search_result_deserialize_property (JsonSerializable *serializable,
                                        const gchar      *property_name,
                                        GValue           *value,
                                        GParamSpec       *pspec,
                                        JsonNode         *property_node)
{
    if (g_strcmp0 (property_name, "creator") == 0)
    {
        JsonObject *obj;

        obj = json_node_get_object (property_node);
        g_value_set_string (value, json_object_get_string_member (obj, "username"));
        return TRUE;
    }

    return serializable_iface->deserialize_property (serializable,
                                                     property_name,
                                                     value,
                                                     pspec,
                                                     property_node);
}

static void
json_serializable_iface_init (JsonSerializableIface *iface)
{
    serializable_iface = g_type_default_interface_peek (JSON_TYPE_SERIALIZABLE);

    iface->deserialize_property = exm_search_result_deserialize_property;
}
