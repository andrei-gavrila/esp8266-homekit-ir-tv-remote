#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <esp/uart.h>
#include <esp/gpio.h>
#include <espressif/esp_common.h>
#include <sysparam.h>
#include <FreeRTOS.h>
#include <task.h>
#include <homekit/characteristics.h>
#include <homekit/homekit.h>
#include <wifi_config.h>
#include <ir/ir.h>
#include <ir/generic.h>
#include <ir/raw.h>

uint8_t ch_data[10][4] = 
{
    {0x20, 0x08, 0x47, 0xB8},
    {0x20, 0x08, 0x1C, 0xE3},
    {0x20, 0x08, 0x1D, 0xE2},
    {0x20, 0x08, 0x1E, 0xE1},
    {0x20, 0x08, 0x40, 0xBF},
    {0x20, 0x08, 0x41, 0xBE},
    {0x20, 0x08, 0x42, 0xBD},
    {0x20, 0x08, 0x44, 0xBB},
    {0x20, 0x08, 0x45, 0xBA},
    {0x20, 0x08, 0x46, 0xB9},
};

ir_generic_config_t protocol = {
    .header_mark = 9000,
    .header_space = -4500,

    .bit1_mark = 560,
    .bit1_space = -1690,

    .bit0_mark = 560,
    .bit0_space = -560,

    .footer_mark = 560,
    .footer_space = -9000,

    .tolerance = 20,
};

void ir_send(uint8_t *data, uint8_t size)
{
    ir_generic_send(&protocol, data, size);
}

struct
{
    uint8_t active;
    uint32_t active_identifier;
    char configured_name[1024];
    uint8_t sleep_discovery_mode;
} device_state;

void device_identify(homekit_value_t value)
{
    printf("device_identify: %d\n", value.bool_value);
}

homekit_value_t device_active_get()
{
    printf("device_active_get: %d\n", device_state.active);

    return HOMEKIT_UINT8(device_state.active);
}

void device_active_set(homekit_value_t value)
{
    if (device_state.active != value.uint8_value)
    {
        int16_t data[] = {8934, -4534, 525, -630, 503, -630, 503, -1713, 527, -601, 532, -630, 503, -630, 503, -630, 502, -631, 503, -1714, 525, -1738, 502, -630, 503, -1716, 524, -1738, 501, -1714, 526, -1716, 524, -1737, 502, -631, 502, -631, 502, -631, 502, -1715, 525, -631, 502, -631, 502, -602, 531, -631, 502, -1738, 502, -1716, 523, -1716, 525, -630, 502, -1714, 526, -1709, 531, -1715, 524, -1710, 530};

        ir_raw_send(data, sizeof(data)/sizeof(*data));
    }

    device_state.active = value.uint8_value;

    printf("device_active_set: %d\n", device_state.active);
}

homekit_value_t device_active_identifier_get()
{
    printf("device_active_identifier_get: %u\n", device_state.active_identifier);

    return HOMEKIT_UINT32(device_state.active_identifier);
}

void device_active_identifier_set(homekit_value_t value)
{
    if (device_state.active_identifier != value.uint32_value)
    {
        char *ch_name = NULL;

        if (value.uint32_value == 1) { sysparam_get_string("ch_name_0", &ch_name); }
        else if (value.uint32_value == 2) { sysparam_get_string("ch_name_1", &ch_name); }
        else if (value.uint32_value == 3) { sysparam_get_string("ch_name_2", &ch_name); }
        else if (value.uint32_value == 4) { sysparam_get_string("ch_name_3", &ch_name); }
        else if (value.uint32_value == 5) { sysparam_get_string("ch_name_4", &ch_name); }

        if (ch_name)
        {
            printf("device_active_identifier_set: channel name: %s\n", ch_name);

            bool sent = false;

            for (uint8_t i = 0; i < strlen(ch_name); i++)
            {
                if (ch_name[i] >= '0' && ch_name[i] <= '9')
                {
                    sent = true;

                    ir_send(ch_data[ch_name[i] - '0'], 4);

                    vTaskDelay(500 / portTICK_PERIOD_MS);

                    printf("device_active_identifier_set: sending code for %d\n", (int16_t)(ch_name[i] - '0'));
                }
                else
                {
                    break;
                }
            }

            if (sent)
            {
                uint8_t data[] = {0x20, 0x08, 0x15, 0xEA};

                ir_send(data, 4);
            }

            free(ch_name);
        } else {
            printf("device_active_identifier_set: channel not found: %u\n", value.uint32_value);
        }
    }

    device_state.active_identifier = value.uint32_value;

    printf("device_active_identifier_set: %u\n", device_state.active_identifier);
}

homekit_value_t device_configured_name_get()
{
    printf("device_configured_name_get: %s\n", device_state.configured_name);

    return HOMEKIT_STRING(strdup(device_state.configured_name), .is_static = 0);
}

void device_configured_name_set(homekit_value_t value)
{
    strncpy(device_state.configured_name, value.string_value, 1024);

    sysparam_set_string("device_state_configured_name", device_state.configured_name);

    printf("device_configured_name_set: %s\n", device_state.configured_name);
}

homekit_value_t device_sleep_discovery_mode_get()
{
    printf("device_sleep_discovery_mode_get: %d\n", device_state.sleep_discovery_mode);

    return HOMEKIT_UINT8(device_state.sleep_discovery_mode);
}

void device_sleep_discovery_mode_set(homekit_value_t value)
{
    device_state.sleep_discovery_mode = value.uint8_value;

    printf("device_sleep_discovery_mode_set: %d\n", device_state.sleep_discovery_mode);
}

void device_remote_key_set(homekit_value_t value)
{
    switch (value.uint8_value)
    {
        case HOMEKIT_REMOTE_KEY_ARROW_UP:
        {
            uint8_t data[] = {0x20, 0x08, 0x12, 0xED};

            ir_send(data, 4);
        }
        break;

        case HOMEKIT_REMOTE_KEY_ARROW_DOWN:
        {
            uint8_t data[] = {0x20, 0x08, 0x13, 0xEC};

            ir_send(data, 4);
        }
        break;

        case HOMEKIT_REMOTE_KEY_ARROW_LEFT:
        {
            uint8_t data[] = {0x20, 0x08, 0x14, 0xEB};

            ir_send(data, 4);
        }
        break;

        case HOMEKIT_REMOTE_KEY_ARROW_RIGHT:
        {
            uint8_t data[] = {0x20, 0x08, 0x16, 0xE9};

            ir_send(data, 4);
        }
        break;

        case HOMEKIT_REMOTE_KEY_SELECT:
        {
            uint8_t data[] = {0x20, 0x08, 0x15, 0xEA};

            ir_send(data, 4);
        }
        break;

        case HOMEKIT_REMOTE_KEY_BACK:
        {
            uint8_t data[] = {0x20, 0x08, 0x0C, 0xF3};

            ir_send(data, 4);
        }
        break;

        case HOMEKIT_REMOTE_KEY_INFORMATION:
        {
            uint8_t data[] = {0x20, 0x08, 0x0D, 0xF2};

            ir_send(data, 4);
        }
        break;

        default:
        break;
    }

    printf("device_remote_key_set: %d\n", value.uint8_value);
}

void speaker_mute_set(homekit_value_t value)
{
    printf("speaker_mute_set: %d\n", value.bool_value);
}

void speaker_volume_selector_set(homekit_value_t value)
{
    switch (value.uint8_value)
    {
        case HOMEKIT_VOLUME_SELECTOR_INCREMENT:
        {
            uint8_t data[] = {0x20, 0x08, 0x02, 0xFD};

            ir_send(data, 4);
        }
        break;

        case HOMEKIT_VOLUME_SELECTOR_DECREMENT:
        {
            uint8_t data[] = {0x20, 0x08, 0x03, 0xFC};
            ir_send(data, 4);
        }
        break;

        default:
        break;
    }

    printf("speaker_volume_selector_set: %d\n", value.uint8_value);
}

void channel_on_configured_name(homekit_characteristic_t *ch, homekit_value_t value, void *arg);

homekit_characteristic_t ch_name_0 = HOMEKIT_CHARACTERISTIC_(CONFIGURED_NAME, "Channel 0", .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(channel_on_configured_name));
homekit_characteristic_t ch_name_1 = HOMEKIT_CHARACTERISTIC_(CONFIGURED_NAME, "Channel 1", .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(channel_on_configured_name));
homekit_characteristic_t ch_name_2 = HOMEKIT_CHARACTERISTIC_(CONFIGURED_NAME, "Channel 2", .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(channel_on_configured_name));
homekit_characteristic_t ch_name_3 = HOMEKIT_CHARACTERISTIC_(CONFIGURED_NAME, "Channel 3", .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(channel_on_configured_name));
homekit_characteristic_t ch_name_4 = HOMEKIT_CHARACTERISTIC_(CONFIGURED_NAME, "Channel 4", .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(channel_on_configured_name));

homekit_service_t ch_service_0 = HOMEKIT_SERVICE_(INPUT_SOURCE, .characteristics = (homekit_characteristic_t*[]) {HOMEKIT_CHARACTERISTIC(NAME, "Channel 0"), HOMEKIT_CHARACTERISTIC(IDENTIFIER,  1), &ch_name_0, HOMEKIT_CHARACTERISTIC(INPUT_SOURCE_TYPE, HOMEKIT_INPUT_SOURCE_TYPE_TUNER), HOMEKIT_CHARACTERISTIC(IS_CONFIGURED, 1), HOMEKIT_CHARACTERISTIC(CURRENT_VISIBILITY_STATE, HOMEKIT_CURRENT_VISIBILITY_STATE_SHOWN), NULL});
homekit_service_t ch_service_1 = HOMEKIT_SERVICE_(INPUT_SOURCE, .characteristics = (homekit_characteristic_t*[]) {HOMEKIT_CHARACTERISTIC(NAME, "Channel 1"), HOMEKIT_CHARACTERISTIC(IDENTIFIER,  2), &ch_name_1, HOMEKIT_CHARACTERISTIC(INPUT_SOURCE_TYPE, HOMEKIT_INPUT_SOURCE_TYPE_TUNER), HOMEKIT_CHARACTERISTIC(IS_CONFIGURED, 1), HOMEKIT_CHARACTERISTIC(CURRENT_VISIBILITY_STATE, HOMEKIT_CURRENT_VISIBILITY_STATE_SHOWN), NULL});
homekit_service_t ch_service_2 = HOMEKIT_SERVICE_(INPUT_SOURCE, .characteristics = (homekit_characteristic_t*[]) {HOMEKIT_CHARACTERISTIC(NAME, "Channel 2"), HOMEKIT_CHARACTERISTIC(IDENTIFIER,  3), &ch_name_2, HOMEKIT_CHARACTERISTIC(INPUT_SOURCE_TYPE, HOMEKIT_INPUT_SOURCE_TYPE_TUNER), HOMEKIT_CHARACTERISTIC(IS_CONFIGURED, 1), HOMEKIT_CHARACTERISTIC(CURRENT_VISIBILITY_STATE, HOMEKIT_CURRENT_VISIBILITY_STATE_SHOWN), NULL});
homekit_service_t ch_service_3 = HOMEKIT_SERVICE_(INPUT_SOURCE, .characteristics = (homekit_characteristic_t*[]) {HOMEKIT_CHARACTERISTIC(NAME, "Channel 3"), HOMEKIT_CHARACTERISTIC(IDENTIFIER,  4), &ch_name_3, HOMEKIT_CHARACTERISTIC(INPUT_SOURCE_TYPE, HOMEKIT_INPUT_SOURCE_TYPE_TUNER), HOMEKIT_CHARACTERISTIC(IS_CONFIGURED, 1), HOMEKIT_CHARACTERISTIC(CURRENT_VISIBILITY_STATE, HOMEKIT_CURRENT_VISIBILITY_STATE_SHOWN), NULL});
homekit_service_t ch_service_4 = HOMEKIT_SERVICE_(INPUT_SOURCE, .characteristics = (homekit_characteristic_t*[]) {HOMEKIT_CHARACTERISTIC(NAME, "Channel 4"), HOMEKIT_CHARACTERISTIC(IDENTIFIER,  5), &ch_name_4, HOMEKIT_CHARACTERISTIC(INPUT_SOURCE_TYPE, HOMEKIT_INPUT_SOURCE_TYPE_TUNER), HOMEKIT_CHARACTERISTIC(IS_CONFIGURED, 1), HOMEKIT_CHARACTERISTIC(CURRENT_VISIBILITY_STATE, HOMEKIT_CURRENT_VISIBILITY_STATE_SHOWN), NULL});

void channel_on_configured_name(homekit_characteristic_t *ch, homekit_value_t value, void *arg)
{
    const char *ch_name = NULL;

    if (ch == &ch_name_0) { ch_name = "ch_name_0"; }
    else if (ch == &ch_name_1) { ch_name = "ch_name_1"; }
    else if (ch == &ch_name_2) { ch_name = "ch_name_2"; }
    else if (ch == &ch_name_3) { ch_name = "ch_name_3"; }
    else if (ch == &ch_name_4) { ch_name = "ch_name_4"; }

    sysparam_set_string(ch_name, value.string_value);
}

homekit_accessory_t *accessories[] =
{
    HOMEKIT_ACCESSORY(
        .id = 1,
        .category = homekit_accessory_category_television,
        .services = (homekit_service_t*[])
        {
            HOMEKIT_SERVICE(
                ACCESSORY_INFORMATION,
                .characteristics = (homekit_characteristic_t*[])
                {
                    HOMEKIT_CHARACTERISTIC(NAME, "IrTvRemote"),
                    HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Andrei Gavrila"),
                    HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "000000000000"),
                    HOMEKIT_CHARACTERISTIC(MODEL, "Infrared Television Remote"),
                    HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
                    HOMEKIT_CHARACTERISTIC(IDENTIFY, device_identify),
                    NULL
                }
            ),
            HOMEKIT_SERVICE(
                TELEVISION,
                .primary = true,
                .characteristics = (homekit_characteristic_t*[])
                {
                    HOMEKIT_CHARACTERISTIC(ACTIVE, 0, .getter = device_active_get, .setter = device_active_set),
                    HOMEKIT_CHARACTERISTIC(ACTIVE_IDENTIFIER, 1, .getter = device_active_identifier_get, .setter = device_active_identifier_set),
                    HOMEKIT_CHARACTERISTIC(CONFIGURED_NAME, "Television", .getter = device_configured_name_get, .setter = device_configured_name_set),
                    HOMEKIT_CHARACTERISTIC(SLEEP_DISCOVERY_MODE, HOMEKIT_SLEEP_DISCOVERY_MODE_ALWAYS_DISCOVERABLE, .getter = device_sleep_discovery_mode_get, .setter = device_sleep_discovery_mode_set),
                    HOMEKIT_CHARACTERISTIC(REMOTE_KEY, .setter = device_remote_key_set),
                    NULL
                },
                .linked = (homekit_service_t*[])
                {
                    &ch_service_0, &ch_service_1, &ch_service_2, &ch_service_3, &ch_service_4,
                    NULL
                }
            ),
            &ch_service_0, &ch_service_1, &ch_service_2, &ch_service_3, &ch_service_4,
            HOMEKIT_SERVICE(
                TELEVISION_SPEAKER,
                .characteristics = (homekit_characteristic_t*[])
                {
                    HOMEKIT_CHARACTERISTIC(MUTE, 0, .setter = speaker_mute_set),
                    HOMEKIT_CHARACTERISTIC(ACTIVE, 1),
                    HOMEKIT_CHARACTERISTIC(VOLUME_CONTROL_TYPE, HOMEKIT_VOLUME_CONTROL_TYPE_RELATIVE),
                    HOMEKIT_CHARACTERISTIC(VOLUME_SELECTOR, .setter = speaker_volume_selector_set),
                    NULL
                }
            ),
            NULL
        }
    ),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "111-11-111"
};

void on_wifi_ready()
{
    homekit_server_init(&config);
}

void user_init(void) {
    char *s = NULL;

    uart_set_baud(0, 115200);

    gpio_set_pullup(14, false, false);

    ir_tx_init();

    device_state.active = 0;
    device_state.active_identifier = 1;
    if (sysparam_get_string("device_state_configured_name", &s) == SYSPARAM_OK)
    {
        strncpy(device_state.configured_name, s, 1024);

        free(s);
    }
    else
    {
        strncpy(device_state.configured_name, "Television", 1024);
    }
    device_state.sleep_discovery_mode = HOMEKIT_SLEEP_DISCOVERY_MODE_ALWAYS_DISCOVERABLE;

    if (sysparam_get_string("ch_name_0", &s) == SYSPARAM_OK) { homekit_value_destruct(&ch_name_0.value); ch_name_0.value = HOMEKIT_STRING(s); }
    if (sysparam_get_string("ch_name_1", &s) == SYSPARAM_OK) { homekit_value_destruct(&ch_name_1.value); ch_name_1.value = HOMEKIT_STRING(s); }
    if (sysparam_get_string("ch_name_2", &s) == SYSPARAM_OK) { homekit_value_destruct(&ch_name_2.value); ch_name_2.value = HOMEKIT_STRING(s); }
    if (sysparam_get_string("ch_name_3", &s) == SYSPARAM_OK) { homekit_value_destruct(&ch_name_3.value); ch_name_3.value = HOMEKIT_STRING(s); }
    if (sysparam_get_string("ch_name_4", &s) == SYSPARAM_OK) { homekit_value_destruct(&ch_name_4.value); ch_name_4.value = HOMEKIT_STRING(s); }

    wifi_config_init("Infrared Television Remote", NULL, on_wifi_ready);
}
