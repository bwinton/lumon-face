#include "settings.h"
#include "message_keys.auto.h"
#include <pebble.h>

#define PERSIST_KEY_LOW_POWER_MODE 1

static bool s_low_power_mode = false;
static SettingsLowPowerChangedHandler s_low_power_changed_handler = NULL;
static void *s_low_power_changed_handler_context = NULL;

static void prv_persist_low_power_mode(void)
{
    persist_write_bool(PERSIST_KEY_LOW_POWER_MODE, s_low_power_mode);
}

static void prv_load_persisted_settings(void)
{
    if (persist_exists(PERSIST_KEY_LOW_POWER_MODE))
    {
        s_low_power_mode = persist_read_bool(PERSIST_KEY_LOW_POWER_MODE);
    }
}

static void prv_inbox_received_callback(DictionaryIterator *iter, void *context)
{
    Tuple *tuple = dict_read_first(iter);
    while (tuple)
    {
        if (tuple->key == MESSAGE_KEY_LowPowerMode)
        {
            bool old_low_power_mode = s_low_power_mode;
            s_low_power_mode = tuple->value->uint8 != 0;
            prv_persist_low_power_mode();
            APP_LOG(APP_LOG_LEVEL_INFO, "Low Power Mode set to %d", (int)s_low_power_mode);
            if (s_low_power_changed_handler && s_low_power_mode != old_low_power_mode)
            {
                s_low_power_changed_handler(s_low_power_changed_handler_context);
            }
        }
        tuple = dict_read_next(iter);
    }
}

void settings_init(void)
{
    prv_load_persisted_settings();
    app_message_register_inbox_received(prv_inbox_received_callback);
    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

void settings_deinit(void)
{
    app_message_deregister_callbacks();
}

bool settings_low_power_mode(void)
{
    return s_low_power_mode;
}

void settings_register_low_power_changed_handler(SettingsLowPowerChangedHandler handler, void *context)
{
    s_low_power_changed_handler = handler;
    s_low_power_changed_handler_context = context;
}

void settings_unregister_low_power_changed_handler(void)
{
    s_low_power_changed_handler = NULL;
    s_low_power_changed_handler_context = NULL;
}
