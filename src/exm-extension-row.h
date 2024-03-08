#pragma once

#include <adwaita.h>

#include "local/exm-extension.h"
#include "local/exm-manager.h"

G_BEGIN_DECLS

#define EXM_TYPE_EXTENSION_ROW (exm_extension_row_get_type())

G_DECLARE_FINAL_TYPE (ExmExtensionRow, exm_extension_row, EXM, EXTENSION_ROW, AdwExpanderRow)

ExmExtensionRow *
exm_extension_row_new (ExmExtension *extension,
                       ExmManager   *manager);

G_END_DECLS
