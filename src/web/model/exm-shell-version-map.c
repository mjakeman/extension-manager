#include "exm-shell-version-map.h"

struct _ExmShellVersionMap
{
    GObject parent_instance;
};

G_DEFINE_FINAL_TYPE (ExmShellVersionMap, exm_shell_version_map, G_TYPE_OBJECT)

enum {
    PROP_0,
    N_PROPS
};

static GParamSpec *properties [N_PROPS];

ExmShellVersionMap *
exm_shell_version_map_new (void)
{
    return g_object_new (EXM_TYPE_SHELL_VERSION_MAP, NULL);
}

static void
exm_shell_version_map_finalize (GObject *object)
{
    ExmShellVersionMap *self = (ExmShellVersionMap *)object;

    G_OBJECT_CLASS (exm_shell_version_map_parent_class)->finalize (object);
}

static void
exm_shell_version_map_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
    ExmShellVersionMap *self = EXM_SHELL_VERSION_MAP (object);

    switch (prop_id)
      {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      }
}

static void
exm_shell_version_map_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
    ExmShellVersionMap *self = EXM_SHELL_VERSION_MAP (object);

    switch (prop_id)
      {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      }
}

static void
exm_shell_version_map_class_init (ExmShellVersionMapClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = exm_shell_version_map_finalize;
    object_class->get_property = exm_shell_version_map_get_property;
    object_class->set_property = exm_shell_version_map_set_property;
}

static void
exm_shell_version_map_init (ExmShellVersionMap *self)
{
}
