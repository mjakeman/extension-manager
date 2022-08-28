#pragma once

// Should really be in the `local/` subdirectory but meson's mkenums
// has trouble with relative paths...

typedef enum
{
    EXM_EXTENSION_TYPE_SYSTEM = 1,
    EXM_EXTENSION_TYPE_PER_USER = 2,
} ExmExtensionType;

typedef enum
{
    EXM_EXTENSION_STATE_ENABLED = 1,
    EXM_EXTENSION_STATE_DISABLED = 2,
    EXM_EXTENSION_STATE_ERROR = 3,
    EXM_EXTENSION_STATE_OUT_OF_DATE = 4,
    EXM_EXTENSION_STATE_DOWNLOADING = 5,
    EXM_EXTENSION_STATE_INITIALIZED = 6,
    EXM_EXTENSION_STATE_UNINSTALLED = 99,
} ExmExtensionState;

typedef enum
{
    EXM_INSTALL_BUTTON_STATE_DEFAULT,
    EXM_INSTALL_BUTTON_STATE_INSTALLED,
    EXM_INSTALL_BUTTON_STATE_UNSUPPORTED
} ExmInstallButtonState;

typedef enum
{
    EXM_UPGRADE_ASSISTANT_STATE_SUPPORTED,
    EXM_UPGRADE_ASSISTANT_STATE_UNSUPPORTED
} ExmUpgradeAssistantState;
