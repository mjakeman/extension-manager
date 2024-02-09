/* exm-comment-dialog.c
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

#include "exm-comment-dialog.h"

#include "exm-comment-tile.h"
#include "web/exm-comment-provider.h"
#include "web/model/exm-comment.h"

#include <glib/gi18n.h>

#include "exm-config.h"

struct _ExmCommentDialog
{
    AdwDialog parent_instance;

    ExmCommentProvider *comment_provider;
    GCancellable *cancellable;
    gboolean closed;

    GtkListBox *list_box;
    GtkStack *stack;
    AdwStatusPage *error_status;

    int web_id;
};

G_DEFINE_FINAL_TYPE (ExmCommentDialog, exm_comment_dialog, ADW_TYPE_DIALOG)

enum {
    PROP_0,
    PROP_WEB_ID,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

static void exm_comment_dialog_constructed (GObject *object);

ExmCommentDialog *
exm_comment_dialog_new (int web_id)
{
    return g_object_new (EXM_TYPE_COMMENT_DIALOG,
                         "web-id", web_id,
                         NULL);
}

static void
exm_comment_dialog_finalize (GObject *object)
{
    G_OBJECT_CLASS (exm_comment_dialog_parent_class)->finalize (object);
}

static void
exm_comment_dialog_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
    ExmCommentDialog *self = EXM_COMMENT_DIALOG (object);

    switch (prop_id)
    {
    case PROP_WEB_ID:
        g_value_set_int (value, self->web_id);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_comment_dialog_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
    ExmCommentDialog *self = EXM_COMMENT_DIALOG (object);

    switch (prop_id)
    {
    case PROP_WEB_ID:
        self->web_id = g_value_get_int (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_comment_dialog_class_init (ExmCommentDialogClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_comment_dialog_finalize;
    object_class->get_property = exm_comment_dialog_get_property;
    object_class->set_property = exm_comment_dialog_set_property;
    object_class->constructed = exm_comment_dialog_constructed;

    properties [PROP_WEB_ID]
        = g_param_spec_int ("web-id",
                            "Web ID",
                            "Web ID",
                            0, G_MAXINT, 0,
                            G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class, g_strdup_printf ("%s/exm-comment-dialog.ui", RESOURCE_PATH));

    gtk_widget_class_bind_template_child (widget_class, ExmCommentDialog, list_box);
    gtk_widget_class_bind_template_child (widget_class, ExmCommentDialog, stack);
    gtk_widget_class_bind_template_child (widget_class, ExmCommentDialog, error_status);
}

static GtkWidget *
comment_factory (ExmComment *comment)
{
    GtkWidget *row;

    row = gtk_list_box_row_new ();
    gtk_list_box_row_set_activatable (GTK_LIST_BOX_ROW (row), FALSE);
    gtk_list_box_row_set_child (GTK_LIST_BOX_ROW (row), GTK_WIDGET (exm_comment_tile_new (comment)));

    return row;
}

static void
on_get_comments (GObject          *source,
                 GAsyncResult     *res,
                 ExmCommentDialog *self)
{
    GError *error = NULL;

    if (self->closed)
      return;

    GListModel *model = exm_comment_provider_get_comments_finish (EXM_COMMENT_PROVIDER (source), res, &error);

    if (error)
    {
        // Filter 5xx status codes (server errors)
        if (error->code / 100 == 5)
            adw_status_page_set_description (self->error_status, _("Check <a href='https://status.gnome.org/'>GNOME infrastructure status</a> and try again later"));
        else
            adw_status_page_set_description (self->error_status, _("Check your network status and try again"));

        gtk_stack_set_visible_child_name (self->stack, "page_error");

        return;
    }

    gtk_stack_set_visible_child_name (self->stack, "page_comments");

    gtk_list_box_bind_model (self->list_box, model,
                             (GtkListBoxCreateWidgetFunc) comment_factory,
                             g_object_ref (self), g_object_unref);
}

static void
on_dialog_closed (ExmCommentDialog *self)
{
  self->closed = TRUE;
}

static void
exm_comment_dialog_constructed (GObject *object)
{
    ExmCommentDialog *self = EXM_COMMENT_DIALOG (object);

    gtk_stack_set_visible_child_name (self->stack, "page_spinner");

    g_cancellable_cancel (self->cancellable);
    self->cancellable = g_cancellable_new ();

    exm_comment_provider_get_comments_async (self->comment_provider,
                                             self->web_id,
                                             true,
                                             self->cancellable,
                                             (GAsyncReadyCallback) on_get_comments,
                                             self);

    G_OBJECT_CLASS (exm_comment_dialog_parent_class)->constructed (object);
}

static void
exm_comment_dialog_init (ExmCommentDialog *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));

    self->comment_provider = exm_comment_provider_new ();

    self->closed = FALSE;

    g_signal_connect (self, "closed", G_CALLBACK (on_dialog_closed), NULL);
}

