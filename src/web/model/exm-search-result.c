#include "exm-search-result.h"

#include <json-glib/json-glib.h>

struct _ExmSearchResult
{
    GObject parent_instance;

    gchar *uuid;
    gchar *name;
    gchar *creator;
    gchar *icon;
    gchar *screenshot;
    gchar *link;
    gchar *description;
};

static void json_serializable_iface_init (JsonSerializableIface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (ExmSearchResult, exm_search_result, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (JSON_TYPE_SERIALIZABLE,
                                                      json_serializable_iface_init))

enum {
    PROP_0,
    PROP_UUID,
    PROP_NAME,
    PROP_CREATOR,
    PROP_ICON,
    PROP_SCREENSHOT,
    PROP_LINK,
    PROP_DESCRIPTION,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmSearchResult *
exm_search_result_new (void)
{
    return g_object_new (EXM_TYPE_SEARCH_RESULT, NULL);
}

static void
exm_search_result_finalize (GObject *object)
{
    ExmSearchResult *self = (ExmSearchResult *)object;

    G_OBJECT_CLASS (exm_search_result_parent_class)->finalize (object);
}

static void
exm_search_result_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
    ExmSearchResult *self = EXM_SEARCH_RESULT (object);

    switch (prop_id)
    {
    case PROP_UUID:
        g_value_set_string (value, self->uuid);
        break;
    case PROP_NAME:
        g_value_set_string (value, self->name);
        break;
    case PROP_CREATOR:
        g_value_set_string (value, self->creator);
        break;
    case PROP_ICON:
        g_value_set_string (value, self->icon);
        break;
    case PROP_SCREENSHOT:
        g_value_set_string (value, self->screenshot);
        break;
    case PROP_LINK:
        g_value_set_string (value, self->link);
        break;
    case PROP_DESCRIPTION:
        g_value_set_string (value, self->description);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_search_result_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
    ExmSearchResult *self = EXM_SEARCH_RESULT (object);

    switch (prop_id)
    {
    case PROP_UUID:
        self->uuid = g_value_dup_string (value);
        break;
    case PROP_NAME:
        self->name = g_value_dup_string (value);
        break;
    case PROP_CREATOR:
        self->creator = g_value_dup_string (value);
        break;
    case PROP_ICON:
        self->icon = g_value_dup_string (value);
        break;
    case PROP_SCREENSHOT:
        self->screenshot = g_value_dup_string (value);
        break;
    case PROP_LINK:
        self->link = g_value_dup_string (value);
        break;
    case PROP_DESCRIPTION:
        self->description = g_value_dup_string (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_search_result_class_init (ExmSearchResultClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_search_result_finalize;
    object_class->get_property = exm_search_result_get_property;
    object_class->set_property = exm_search_result_set_property;

    properties [PROP_UUID] =
        g_param_spec_string ("uuid",
                             "UUID",
                             "UUID",
                             NULL,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_NAME] =
        g_param_spec_string ("name",
                             "Name",
                             "Name",
                             NULL,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_CREATOR] =
        g_param_spec_string ("creator",
                             "Creator",
                             "Creator",
                             NULL,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_ICON] =
        g_param_spec_string ("icon",
                             "Icon",
                             "Icon",
                             NULL,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_SCREENSHOT] =
        g_param_spec_string ("screenshot",
                             "Screenshot",
                             "Screenshot",
                             NULL,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_LINK] =
        g_param_spec_string ("link",
                             "Link",
                             "Link",
                             NULL,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_DESCRIPTION] =
        g_param_spec_string ("description",
                             "Description",
                             "Description",
                             NULL,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
exm_search_result_init (ExmSearchResult *self)
{
}

static void
json_serializable_iface_init (JsonSerializableIface *iface)
{

}
