/*
 * exm-upgrade-assistant.c
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
#include "exm-upgrade-assistant.h"

struct _ExmUpgradeAssistant
{
    AdwWindow parent_instance;

    GtkListView *list_view;
};

G_DEFINE_FINAL_TYPE (ExmUpgradeAssistant, exm_upgrade_assistant, ADW_TYPE_WINDOW)

enum {
    PROP_0,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmUpgradeAssistant *
exm_upgrade_assistant_new (void)
{
    return g_object_new (EXM_TYPE_UPGRADE_ASSISTANT, NULL);
}

static void
exm_upgrade_assistant_finalize (GObject *object)
{
    ExmUpgradeAssistant *self = (ExmUpgradeAssistant *)object;

    G_OBJECT_CLASS (exm_upgrade_assistant_parent_class)->finalize (object);
}

static void
exm_upgrade_assistant_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
    ExmUpgradeAssistant *self = EXM_UPGRADE_ASSISTANT (object);

    switch (prop_id)
      {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      }
}

static void
exm_upgrade_assistant_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
    ExmUpgradeAssistant *self = EXM_UPGRADE_ASSISTANT (object);

    switch (prop_id)
      {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      }
}

static void
exm_upgrade_assistant_class_init (ExmUpgradeAssistantClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_upgrade_assistant_finalize;
    object_class->get_property = exm_upgrade_assistant_get_property;
    object_class->set_property = exm_upgrade_assistant_set_property;

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, "/com/mattjakeman/ExtensionManager/exm-upgrade-assistant.ui");
    gtk_widget_class_bind_template_child (widget_class, ExmUpgradeAssistant, list_view);
}

static void
exm_upgrade_assistant_init (ExmUpgradeAssistant *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}

