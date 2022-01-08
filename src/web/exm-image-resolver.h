#pragma once

#include <glib-object.h>
#include <gio/gio.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define EXM_TYPE_IMAGE_RESOLVER (exm_image_resolver_get_type())

G_DECLARE_FINAL_TYPE (ExmImageResolver, exm_image_resolver, EXM, IMAGE_RESOLVER, GObject)

ExmImageResolver *exm_image_resolver_new (void);
void exm_image_resolver_resolve_async (ExmImageResolver    *self,
                                       const gchar         *rel_path,
                                       GCancellable        *cancellable,
                                       GAsyncReadyCallback  callback,
                                       gpointer             user_data);
GdkTexture *exm_image_resolver_resolve_finish (ExmImageResolver  *self,
                                               GAsyncResult      *result,
                                               GError           **error);

G_END_DECLS
