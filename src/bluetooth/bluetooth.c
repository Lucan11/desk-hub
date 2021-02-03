#include "bluetooth.h"

#include "bsp.h"
#include "nrf_soc.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "ble_advdata.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "Si7021.h"


/**< A tag identifying the SoftDevice BLE configuration. */
#define APP_BLE_CONN_CFG_TAG             1
/**< The advertising interval for non-connectable advertisement (100 ms). This value can vary between 100ms to 10.24s). */
#define NON_CONNECTABLE_ADV_INTERVAL    MSEC_TO_UNITS(100, UNIT_0_625_MS)
#define DEVICE_NAME                     ((uint8_t const *)"tempsensor")


/**< Advertising handle used to identify an advertising set. */
static uint8_t m_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;
/**< Buffer for storing an encoded advertising set. */
static uint8_t m_enc_advdata[BLE_GAP_ADV_SET_DATA_SIZE_MAX];

// Not sure which of these need to be double to update the advertisement data, so double them al for now
static ble_advdata_t        advdata;
static ble_gap_adv_params_t adv_params;
static ble_advdata_manuf_data_t manuf_specific_data;

/**@brief Struct that contains pointers to the encoded advertising data. */
static ble_gap_adv_data_t m_adv_data = {
    .adv_data = {
            .p_data = m_enc_advdata,
            .len = BLE_GAP_ADV_SET_DATA_SIZE_MAX
    },
    .scan_rsp_data =  {
            .p_data = NULL,
            .len = 0
    }
};


// The manufacturer data in the BLE advertisement
static struct _m_beacon_info {
    float temperature;
    float humidity;
}
PACKED
m_beacon_info = {
    .temperature = 0,
    .humidity = 0
};


/**@brief Function for initializing the Advertising functionality.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 */
static void advertising_init(void) {
    ret_code_t           err_code;
    ble_gap_conn_sec_mode_t sec_mode;

    // Set the device name
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
    err_code = sd_ble_gap_device_name_set(&sec_mode, DEVICE_NAME, strlen((const char*)DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    // Build and set advertising data.
    memset(&advdata, 0, sizeof(advdata));
    memset(&manuf_specific_data, 0, sizeof(ble_advdata_manuf_data_t));

    advdata.name_type          = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance = true;
    advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

    advdata.p_manuf_specific_data = &manuf_specific_data;
    advdata.p_manuf_specific_data->company_identifier = 0xFFFF;
    advdata.p_manuf_specific_data->data.p_data = (uint8_t*)&m_beacon_info;
    advdata.p_manuf_specific_data->data.size = sizeof(m_beacon_info);
    err_code = ble_advdata_encode(&advdata, m_adv_data.adv_data.p_data, &m_adv_data.adv_data.len);
    APP_ERROR_CHECK(err_code);

    // Start advertising.
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.p_peer_addr   = NULL;
    adv_params.filter_policy = BLE_GAP_ADV_FP_ANY;
    adv_params.interval      = NON_CONNECTABLE_ADV_INTERVAL;

    adv_params.properties.type = BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED;
    adv_params.duration        = 0;
    adv_params.primary_phy     = BLE_GAP_PHY_AUTO;
    adv_params.secondary_phy   = BLE_GAP_PHY_AUTO;

    err_code = sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data, &adv_params);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for starting advertising.
 */
static void advertising_start(void)
{
    ret_code_t err_code;

    err_code = sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);
}



void bluetooth_init() {
    ble_stack_init();

    advertising_init();
}


void bluetooth_start_advertisement() {
    advertising_start();
}


void bluetooth_update_advertisement_data(const temperature_sensor_data_t * const data) {
    NRFX_ASSERT(data != NULL);

    m_beacon_info.humidity = data->humidity;
    m_beacon_info.temperature = data->temperature;

    // Not the most efficient way to update the advertising data
    // only need to call ble_advdata_encode and sd_ble_gap_adv_set_configure, I think.
    // update_advertising_data(advdata, adv_params, manuf_specific_data);

    // ble_advertising_advdata_update();
}
