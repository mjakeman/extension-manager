#pragma once

#include <adwaita.h>

#include "web/model/exm-comment.h"

G_BEGIN_DECLS

#define EXM_TYPE_COMMENT_TILE (exm_comment_tile_get_type())

G_DECLARE_FINAL_TYPE (ExmCommentTile, exm_comment_tile, EXM, COMMENT_TILE, GtkWidget)

ExmCommentTile *exm_comment_tile_new (ExmComment *comment);

G_END_DECLS
