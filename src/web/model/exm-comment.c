#include "exm-comment.h"

struct _ExmComment
{
    GObject parent_instance;
};

G_DEFINE_FINAL_TYPE (ExmComment, exm_comment, G_TYPE_OBJECT)

enum {
    PROP_0,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmComment *
exm_comment_new (void)
{
    return g_object_new (EXM_TYPE_COMMENT, NULL);
}

static void
exm_comment_finalize (GObject *object)
{
    ExmComment *self = (ExmComment *)object;

    G_OBJECT_CLASS (exm_comment_parent_class)->finalize (object);
}

static void
exm_comment_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
    ExmComment *self = EXM_COMMENT (object);

    switch (prop_id)
      {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      }
}

static void
exm_comment_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
    ExmComment *self = EXM_COMMENT (object);

    switch (prop_id)
      {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      }
}

static void
exm_comment_class_init (ExmCommentClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_comment_finalize;
    object_class->get_property = exm_comment_get_property;
    object_class->set_property = exm_comment_set_property;
}

static void
exm_comment_init (ExmComment *self)
{
}
