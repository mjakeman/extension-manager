#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define EXM_TYPE_EXTENSION (exm_extension_get_type())

G_DECLARE_FINAL_TYPE (ExmExtension, exm_extension, EXM, EXTENSION, GObject)

ExmExtension *exm_extension_new (const gchar *uuid);

G_END_DECLS
