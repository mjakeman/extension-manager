#include "exm-extension.h"

struct _ExmExtension
{
    GObject parent_instance;
    gchar *uuid;
    gchar *display_name;
    gchar *description;
};

G_DEFINE_FINAL_TYPE (ExmExtension, exm_extension, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_UUID,
    PROP_DISPLAY_NAME,
    // PROP_DICTIONARY,
    PROP_DESCRIPTION,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmExtension *
exm_extension_new (gchar *uuid,
                   gchar *display_name,
                   gchar *description)
{
    return g_object_new (EXM_TYPE_EXTENSION,
                         "uuid", uuid,
                         "display-name", display_name,
                         "description", description,
                         NULL);
}

static void
exm_extension_finalize (GObject *object)
{
    ExmExtension *self = (ExmExtension *)object;

    G_OBJECT_CLASS (exm_extension_parent_class)->finalize (object);
}

static void
exm_extension_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
    ExmExtension *self = EXM_EXTENSION (object);

    switch (prop_id)
    {
    case PROP_UUID:
        g_value_set_string (value, self->uuid);
        break;
    case PROP_DISPLAY_NAME:
        g_value_set_string (value, self->display_name);
        break;
    case PROP_DESCRIPTION:
        g_value_set_string (value, self->description);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_extension_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
    ExmExtension *self = EXM_EXTENSION (object);

    switch (prop_id)
    {
    case PROP_UUID:
        self->uuid = g_value_dup_string (value);
        break;
    case PROP_DISPLAY_NAME:
        self->display_name = g_value_dup_string (value);
        break;
    case PROP_DESCRIPTION:
        self->description = g_value_dup_string (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
exm_extension_class_init (ExmExtensionClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_extension_finalize;
    object_class->get_property = exm_extension_get_property;
    object_class->set_property = exm_extension_set_property;

    properties [PROP_UUID] =
        g_param_spec_string ("uuid",
                             "UUID",
                             "UUID",
                             NULL,
                             G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

    properties [PROP_DISPLAY_NAME] =
        g_param_spec_string ("display-name",
                             "Display Name",
                             "Display Name",
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
exm_extension_init (ExmExtension *self)
{
}
