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

void
exm_unified_data_set_local_data (ExmUnifiedData *self,
                                   ExmExtension     *extension)
{
    self->local_data = g_object_ref (extension);
}

ExmSearchResult *
exm_unified_data_get_web_data (ExmUnifiedData *self)
{
    return self->web_data;
}

void
exm_unified_data_set_web_data (ExmUnifiedData *self,
                                 ExmSearchResult  *extension)
{
    self->web_data = g_object_ref (extension);
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

static void
exm_unified_data_class_init (ExmUnifiedDataClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_unified_data_finalize;
    object_class->get_property = exm_unified_data_get_property;
    object_class->set_property = exm_unified_data_set_property;

    properties [PROP_NAME] =
        g_param_spec_string ("name", "Name", "Name", NULL, G_PARAM_READABLE);

    g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
exm_unified_data_init (ExmUnifiedData *self)
{
    self->local_data = NULL;
    self->web_data = NULL;
}

