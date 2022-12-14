#pragma once

#include <adwaita.h>

#include "local/exm-extension.h"

G_BEGIN_DECLS

#define EXM_TYPE_EXTENSION_ROW (exm_extension_row_get_type())

G_DECLARE_FINAL_TYPE (ExmExtensionRow, exm_extension_row, EXM, EXTENSION_ROW, AdwActionRow)

ExmExtensionRow *
exm_extension_row_new (ExmExtension *extension);

G_END_DECLS
