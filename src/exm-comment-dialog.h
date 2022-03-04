#pragma once

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EXM_TYPE_COMMENT_DIALOG (exm_comment_dialog_get_type())

G_DECLARE_FINAL_TYPE (ExmCommentDialog, exm_comment_dialog, EXM, COMMENT_DIALOG, GtkWindow)

ExmCommentDialog *exm_comment_dialog_new (int web_id);

G_END_DECLS
