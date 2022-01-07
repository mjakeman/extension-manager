#pragma once

#include <glib-object.h>

#include "exm-extension.h"

G_BEGIN_DECLS

#define EXM_TYPE_MANAGER (exm_manager_get_type())

G_DECLARE_FINAL_TYPE (ExmManager, exm_manager, EXM, MANAGER, GObject)

ExmManager *exm_manager_new (void);
void exm_manager_enable_extension (ExmManager *manager, ExmExtension *extension);
void exm_manager_disable_extension (ExmManager *manager, ExmExtension *extension);

G_END_DECLS
