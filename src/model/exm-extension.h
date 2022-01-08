#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define EXM_TYPE_EXTENSION (exm_extension_get_type())

G_DECLARE_FINAL_TYPE (ExmExtension, exm_extension, EXM, EXTENSION, GObject)

ExmExtension *exm_extension_new (gchar    *uuid,
                                 gchar    *display_name,
                                 gchar    *description,
                                 gboolean  enabled,
                                 gboolean  has_prefs,
                                 gboolean  has_update);

G_END_DECLS
