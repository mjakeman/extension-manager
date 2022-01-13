#pragma once

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define EXM_TYPE_REQUEST_HANDLER (exm_request_handler_get_type())

G_DECLARE_DERIVABLE_TYPE (ExmRequestHandler, exm_request_handler, EXM, REQUEST_HANDLER, GObject)

typedef gpointer (*ResponseHandler)(GBytes *bytes, GError **out_error);

struct _ExmRequestHandlerClass
{
    GObjectClass parent_class;
    ResponseHandler handle_response;
};

ExmRequestHandler *exm_request_handler_new (void);

void
exm_request_handler_request_async (ExmRequestHandler   *self,
                                   const gchar         *url_endpoint,
                                   GCancellable        *cancellable,
                                   GAsyncReadyCallback  callback,
                                   gpointer             user_data);

gpointer
exm_request_handler_request_finish (ExmRequestHandler  *self,
                                    GAsyncResult       *result,
                                    GError            **error);

G_END_DECLS
