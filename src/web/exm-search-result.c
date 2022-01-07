#include "exm-search-result.h"

struct _ExmSearchResult
{
    GObject parent_instance;
};

G_DEFINE_FINAL_TYPE (ExmSearchResult, exm_search_result, G_TYPE_OBJECT)

enum {
    PROP_0,
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
}

static void
exm_search_result_init (ExmSearchResult *self)
{
}
