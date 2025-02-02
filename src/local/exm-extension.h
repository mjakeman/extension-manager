#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define EXM_TYPE_EXTENSION (exm_extension_get_type())

G_DECLARE_FINAL_TYPE (ExmExtension, exm_extension, EXM, EXTENSION, GObject)

ExmExtension *exm_extension_new  (const gchar *uuid);

gint          compare_extension  (ExmExtension *a,
                                  ExmExtension *b,
                                  gpointer      user_data);

gboolean      is_extension_equal (ExmExtension *a,
                                  ExmExtension *b);

G_END_DECLS
