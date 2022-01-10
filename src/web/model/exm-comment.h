#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define EXM_TYPE_COMMENT (exm_comment_get_type())

G_DECLARE_FINAL_TYPE (ExmComment, exm_comment, EXM, COMMENT, GObject)

ExmComment *exm_comment_new (void);

G_END_DECLS
