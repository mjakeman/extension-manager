#pragma once

#include <glib-object.h>
#include <gio/gio.h>

#include "exm-extension.h"

G_BEGIN_DECLS

#define EXM_TYPE_MANAGER (exm_manager_get_type())

G_DECLARE_FINAL_TYPE (ExmManager, exm_manager, EXM, MANAGER, GObject)

ExmManager *exm_manager_new (void);
void exm_manager_enable_extension (ExmManager *manager, ExmExtension *extension);
void exm_manager_disable_extension (ExmManager *manager, ExmExtension *extension);
void exm_manager_remove_extension (ExmManager *self, ExmExtension *extension);
void exm_manager_open_prefs (ExmManager *self, ExmExtension *extension);
gboolean exm_manager_is_installed_uuid (ExmManager *self, const gchar *uuid);
ExmExtension *exm_manager_get_by_uuid (ExmManager  *self, const gchar *uuid);

void exm_manager_install_async (ExmManager          *self,
                                const gchar         *uuid,
                                GCancellable        *cancellable,
                                GAsyncReadyCallback  callback,
                                gpointer             user_data);

gboolean exm_manager_install_finish (ExmManager    *self,
                                     GAsyncResult  *result,
                                     GError       **error);

G_END_DECLS
