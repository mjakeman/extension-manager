#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define EXM_TYPE_MANAGER (exm_manager_get_type())

G_DECLARE_FINAL_TYPE (ExmManager, exm_manager, EXM, MANAGER, GObject)

ExmManager *exm_manager_new (void);

G_END_DECLS
