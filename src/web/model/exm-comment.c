#include "exm-comment.h"

struct _ExmComment
{
    GObject parent_instance;

    gchar *comment;
};

G_DEFINE_FINAL_TYPE (ExmComment, exm_comment, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_COMMENT,
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
    case PROP_COMMENT:
        g_value_set_string (value, self->comment);
        break;
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
    case PROP_COMMENT:
        self->comment = g_value_dup_string (value);
        break;
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

    properties [PROP_COMMENT] =
        g_param_spec_string ("comment",
                             "Comment",
                             "Comment",
                             NULL,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
exm_comment_init (ExmComment *self)
{
}
