#include "exm-comment.h"

#include <json-glib/json-glib.h>

struct _ExmComment
{
    GObject parent_instance;

    gchar *comment;
    gchar *author;
};

static JsonSerializableIface *serializable_iface = NULL;

static void json_serializable_iface_init (JsonSerializableIface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (ExmComment, exm_comment, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (JSON_TYPE_SERIALIZABLE,
                                                      json_serializable_iface_init))

enum {
    PROP_0,
    PROP_COMMENT,
    PROP_AUTHOR,
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
    case PROP_AUTHOR:
        g_value_set_string (value, self->author);
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
    case PROP_AUTHOR:
        self->author = g_value_dup_string (value);
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
                             G_PARAM_READWRITE);

    properties [PROP_AUTHOR] =
        g_param_spec_string ("author",
                             "Author",
                             "Author",
                             NULL,
                             G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
exm_comment_init (ExmComment *self)
{
}

static gboolean
exm_comment_deserialize_property (JsonSerializable *serializable,
                                  const gchar      *property_name,
                                  GValue           *value,
                                  GParamSpec       *pspec,
                                  JsonNode         *property_node)
{
    if (strcmp (property_name, "author") == 0)
    {
        JsonObject *obj;

        obj = json_node_get_object (property_node);
        g_value_set_string (value, json_object_get_string_member (obj, "username"));
        return TRUE;
    }

    return serializable_iface->deserialize_property (serializable,
                                                     property_name,
                                                     value,
                                                     pspec,
                                                     property_node);
}

static void
json_serializable_iface_init (JsonSerializableIface *iface)
{
    serializable_iface = g_type_default_interface_peek (JSON_TYPE_SERIALIZABLE);

    iface->deserialize_property = exm_comment_deserialize_property;
}
