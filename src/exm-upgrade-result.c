/*
 * exm-upgrade-result.c
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

#include "exm-upgrade-result.h"

struct _ExmUpgradeResult
{
    GObject parent_instance;

    ExmUpgradeSupport support;
    ExmExtension *local_data;
};

G_DEFINE_FINAL_TYPE (ExmUpgradeResult, exm_upgrade_result, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_NAME,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmUpgradeResult *
exm_upgrade_result_new (void)
{
    return g_object_new (EXM_TYPE_UPGRADE_RESULT, NULL);
}

static void
exm_upgrade_result_finalize (GObject *object)
{
    ExmUpgradeResult *self = (ExmUpgradeResult *)object;

    if (self->local_data)
        g_object_unref (self->local_data);

    G_OBJECT_CLASS (exm_upgrade_result_parent_class)->finalize (object);
}

static void
exm_upgrade_result_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
    ExmUpgradeResult *self = EXM_UPGRADE_RESULT (object);

    switch (prop_id)
    {
    case PROP_NAME:
        g_value_set_string (value, exm_upgrade_result_get_name (self));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

ExmExtension *
exm_upgrade_result_get_local_data (ExmUpgradeResult *self)
{
    return self->local_data;
}

void
exm_upgrade_result_set_local_data (ExmUpgradeResult *self,
                                   ExmExtension     *extension)
{
    self->local_data = g_object_ref (extension);
}

ExmUpgradeSupport
exm_upgrade_result_get_support (ExmUpgradeResult *self)
{
    return self->support;
}

void
exm_upgrade_result_set_support (ExmUpgradeResult *self,
                                ExmUpgradeSupport support)
{
    self->support = support;
}

const char *
exm_upgrade_result_get_name (ExmUpgradeResult *self)
{
    const char *name;

    if (self->local_data)
    {
        g_object_get (self->local_data, "name", &name, NULL);
        return name;
    }

    return NULL;
}

const char *
exm_upgrade_result_get_uuid (ExmUpgradeResult *self)
{
    const char *uuid;

    if (self->local_data)
    {
        g_object_get (self->local_data, "uuid", &uuid, NULL);
        return uuid;
    }

    return NULL;
}

static void
exm_upgrade_result_class_init (ExmUpgradeResultClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_upgrade_result_finalize;
    object_class->get_property = exm_upgrade_result_get_property;

    properties [PROP_NAME] =
        g_param_spec_string ("name", "Name", "Name", NULL, G_PARAM_READABLE);

    g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
exm_upgrade_result_init (ExmUpgradeResult *self)
{
    self->local_data = NULL;
    self->support = EXM_UPGRADE_SUPPORT_UNKNOWN;
}

