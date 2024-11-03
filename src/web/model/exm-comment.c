#include "exm-comment.h"

#include "exm-utils.h"

#include <json-glib/json-glib.h>

struct _ExmComment
{
    GObject parent_instance;

    gboolean is_extension_creator;
    gchar *comment;
    gchar *author;
    int rating;
    gchar *date;
};

static JsonSerializableIface *serializable_iface = NULL;

static void json_serializable_iface_init (JsonSerializableIface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (ExmComment, exm_comment, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (JSON_TYPE_SERIALIZABLE,
                                                      json_serializable_iface_init))

enum {
    PROP_0,
    PROP_IS_EXTENSION_CREATOR,
    PROP_COMMENT,
    PROP_AUTHOR,
    PROP_RATING,
    PROP_DATE,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmComment *
exm_comment_new (void)
{
    return g_object_new (EXM_TYPE_COMMENT, NULL);
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
    case PROP_IS_EXTENSION_CREATOR:
        g_value_set_boolean (value, self->is_extension_creator);
        break;
    case PROP_COMMENT:
        g_value_set_string (value, exm_utils_convert_html (self->comment));
        break;
    case PROP_AUTHOR:
        g_value_set_string (value, self->author);
        break;
    case PROP_RATING:
        g_value_set_int (value, self->rating);
        break;
    case PROP_DATE:
        g_value_set_string (value, self->date);
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
    case PROP_IS_EXTENSION_CREATOR:
        self->is_extension_creator = g_value_get_boolean (value);
        break;
    case PROP_COMMENT:
        self->comment = g_value_dup_string (value);
        break;
    case PROP_AUTHOR:
        self->author = g_value_dup_string (value);
        break;
    case PROP_RATING:
        self->rating = g_value_get_int (value);
        break;
    case PROP_DATE:
        self->date = g_value_dup_string (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_comment_class_init (ExmCommentClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->get_property = exm_comment_get_property;
    object_class->set_property = exm_comment_set_property;

    properties [PROP_IS_EXTENSION_CREATOR] =
        g_param_spec_boolean ("is_extension_creator",
                              "Is Extension Creator",
                              "Is the creator of the extension",
                              FALSE,
                              G_PARAM_READWRITE);

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

    properties [PROP_RATING] =
        g_param_spec_int ("rating",
                          "Rating",
                          "Rating",
                          -1, 5, -1, /* min -1, max 5, default -1 */
                          G_PARAM_READWRITE);

    properties [PROP_DATE] =
        g_param_spec_string ("date",
                             "Date",
                             "Date",
                             NULL,
                             G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
exm_comment_init (ExmComment *self)
{
    // Initialise to -1 for no rating
    self->rating = -1;
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
    else if (strcmp (property_name, "date") == 0)
    {
        JsonObject *obj;

        obj = json_node_get_object (property_node);
        g_value_set_string (value, json_object_get_string_member (obj, "timestamp"));
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
