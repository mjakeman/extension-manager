#include "exm-search-result.h"

#include "exm-shell-version-map.h"

#include <json-glib/json-glib.h>

struct _ExmSearchResult
{
    GObject parent_instance;

    gchar *uuid;
    gchar *name;
    gchar *creator;
    gchar *icon;
    gchar *screenshot;
    gchar *url;
    gchar *link;
    gchar *description;
    int pk;
    int downloads;
    ExmShellVersionMap *shell_version_map;
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
    PROP_URL,
    PROP_LINK,
    PROP_DESCRIPTION,
    PROP_PK,
    PROP_DOWNLOADS,
    PROP_SHELL_VERSION_MAP,
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

    g_clear_pointer (&self->shell_version_map, exm_shell_version_map_unref);

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
    case PROP_URL:
        g_value_set_string (value, self->url);
        break;
    case PROP_LINK:
        g_value_set_string (value, self->link);
        break;
    case PROP_DESCRIPTION:
        g_value_set_string (value, self->description);
        break;
    case PROP_PK:
        g_value_set_int (value, self->pk);
        break;
    case PROP_DOWNLOADS:
        g_value_set_int (value, self->downloads);
        break;
    case PROP_SHELL_VERSION_MAP:
        g_value_set_boxed (value, self->shell_version_map);
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

    const ExmShellVersionMap *map;

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
    case PROP_URL:
        self->url = g_value_dup_string (value);
        break;
    case PROP_LINK:
        self->link = g_value_dup_string (value);
        break;
    case PROP_DESCRIPTION:
        self->description = g_value_dup_string (value);
        break;
    case PROP_PK:
        self->pk = g_value_get_int (value);
        break;
    case PROP_DOWNLOADS:
        self->downloads = g_value_get_int (value);
        break;
    case PROP_SHELL_VERSION_MAP:
        if (self->shell_version_map)
            g_clear_pointer (&self->shell_version_map, exm_shell_version_map_unref);

        self->shell_version_map = exm_shell_version_map_ref (g_value_get_boxed (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

gboolean
exm_search_result_supports_shell_version (ExmSearchResult *self,
                                          const gchar     *shell_version)
{
    g_return_val_if_fail (shell_version, FALSE);

    return exm_shell_version_map_supports (self->shell_version_map,
                                           shell_version);
}

static void
deserialize_version (JsonObject         *object,
                     const gchar        *shell_version,
                     JsonNode           *member_node,
                     ExmShellVersionMap *version_map)
{
    int package;
    double version;
    JsonObject *version_obj;

    version_obj = json_node_get_object (member_node);

    package = json_object_get_int_member (version_obj, "pk");
    version = json_object_get_double_member (version_obj, "version");

    exm_shell_version_map_add (version_map, shell_version, package, version);
}

static gpointer
deserialize_shell_version_map (JsonNode* node)
{
    ExmShellVersionMap *version_map;
    JsonObject *object;

    version_map = exm_shell_version_map_new ();

    g_assert (JSON_NODE_HOLDS_OBJECT (node));
    object = json_node_get_object (node);

    json_object_foreach_member (object, (JsonObjectForeach) deserialize_version, version_map);

    return version_map;
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

    properties [PROP_URL] =
        g_param_spec_string ("url",
                             "Url",
                             "Url",
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

    properties [PROP_PK] =
        g_param_spec_int ("pk",
                          "Package ID",
                          "Package ID",
                          0, G_MAXINT, 0,
                          G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_DOWNLOADS] =
        g_param_spec_int ("downloads",
                          "Downloads",
                          "Downloads",
                          0, G_MAXINT, 0,
                          G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_SHELL_VERSION_MAP] =
        g_param_spec_boxed ("shell_version_map",
                             "Shell Version Map",
                             "Shell Version Map",
                             EXM_TYPE_SHELL_VERSION_MAP,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    json_boxed_register_deserialize_func (EXM_TYPE_SHELL_VERSION_MAP,
                                          JSON_NODE_OBJECT,
                                          deserialize_shell_version_map);
}

static void
exm_search_result_init (ExmSearchResult *self)
{
}

static void
json_serializable_iface_init (JsonSerializableIface *iface)
{
}
