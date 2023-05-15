#include <nvs_flash.h>
#include <lwip/def.h>

#include "N2kBt.h"

static const char *TAG = "N2kBt";

N2kBt *N2kBt::instance = nullptr;
static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    N2kBt::instance->espBtSppCb(event, param);
}
static void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    N2kBt::espBtGapCb(event, param);
}

N2kBt::N2kBt(SideTwaiBusInterface &twaiBusSender)
: twaiBusSender(twaiBusSender)
, slipPacket(*this, *this)
{
    for(uint32_t & m_BtSppHandle : m_BtSppHandles) {
        m_BtSppHandle = INVALID_HANDLE;
    }
}

char *N2kBt::bda2str(uint8_t *bda, char *str, size_t size)
{
    if (bda == nullptr || str == nullptr || size < 18) {
        return nullptr;
    }

    uint8_t *p = bda;
    sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
            p[0], p[1], p[2], p[3], p[4], p[5]);
    return str;
}

void N2kBt::espBtGapCb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    char bda_str[18] = {0};

    switch (event) {
        case ESP_BT_GAP_AUTH_CMPL_EVT:{
            if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(TAG, "authentication success: %s bda:[%s]", param->auth_cmpl.device_name,
                         bda2str(param->auth_cmpl.bda, bda_str, sizeof(bda_str)));
            } else {
                ESP_LOGE(TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
            }
            break;
        }
        case ESP_BT_GAP_PIN_REQ_EVT:{
            ESP_LOGI(TAG, "ESP_BT_GAP_PIN_REQ_EVT min_16_digit:%d", param->pin_req.min_16_digit);
            if (param->pin_req.min_16_digit) {
                ESP_LOGI(TAG, "Input pin code: 0000 0000 0000 0000");
                esp_bt_pin_code_t pin_code = {0};
                esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
            } else {
                ESP_LOGI(TAG, "Input pin code: 1234");
                esp_bt_pin_code_t pin_code;
                pin_code[0] = '1';
                pin_code[1] = '2';
                pin_code[2] = '3';
                pin_code[3] = '4';
                esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
            }
            break;
        }

#if (CONFIG_BT_SSP_ENABLED == true)
        case ESP_BT_GAP_CFM_REQ_EVT:
            ESP_LOGI(TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d", param->cfm_req.num_val);
            esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
            break;
        case ESP_BT_GAP_KEY_NOTIF_EVT:
            ESP_LOGI(TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%d", param->key_notif.passkey);
            break;
        case ESP_BT_GAP_KEY_REQ_EVT:
            ESP_LOGI(TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
            break;
#endif

        case ESP_BT_GAP_MODE_CHG_EVT:
            ESP_LOGI(TAG, "ESP_BT_GAP_MODE_CHG_EVT mode:%d bda:[%s]", param->mode_chg.mode,
                     bda2str(param->mode_chg.bda, bda_str, sizeof(bda_str)));
            break;

        default: {
            ESP_LOGI(TAG, "event: %d", event);
            break;
        }
    }
}


void N2kBt::espBtSppCb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    char bda_str[18] = {0};

    switch (event) {
        case ESP_SPP_INIT_EVT:
            if (param->init.status == ESP_SPP_SUCCESS) {
                ESP_LOGI(TAG, "ESP_SPP_INIT_EVT");
                esp_spp_start_srv(sec_mask, role_slave, 0, SPP_SERVER_NAME);
            } else {
                ESP_LOGE(TAG, "ESP_SPP_INIT_EVT status:%d", param->init.status);
            }
            break;
        case ESP_SPP_DISCOVERY_COMP_EVT:
            ESP_LOGI(TAG, "ESP_SPP_DISCOVERY_COMP_EVT");
            break;
        case ESP_SPP_OPEN_EVT:
            ESP_LOGI(TAG, "ESP_SPP_OPEN_EVT");
            break;
        case ESP_SPP_CLOSE_EVT:
            ESP_LOGI(TAG, "ESP_SPP_CLOSE_EVT status:%d handle:%d close_by_remote:%d", param->close.status,
                     param->close.handle, param->close.async);

            for(uint32_t & m_BtSppHandle : m_BtSppHandles){
                if( m_BtSppHandle == param->close.handle){
                    m_BtSppHandle = INVALID_HANDLE;
                    break;
                }
            }

            break;
        case ESP_SPP_START_EVT:
            if (param->start.status == ESP_SPP_SUCCESS) {
                ESP_LOGI(TAG, "ESP_SPP_START_EVT handle:%d sec_id:%d scn:%d", param->start.handle, param->start.sec_id,
                         param->start.scn);
                esp_bt_dev_set_device_name(EXAMPLE_DEVICE_NAME);
                esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            } else {
                ESP_LOGE(TAG, "ESP_SPP_START_EVT status:%d", param->start.status);
            }
            break;
        case ESP_SPP_CL_INIT_EVT:
            ESP_LOGI(TAG, "ESP_SPP_CL_INIT_EVT");
            break;
        case ESP_SPP_DATA_IND_EVT:
            ESP_LOG_BUFFER_HEX_LEVEL("BT_RX", param->data_ind.data, param->data_ind.len, ESP_LOG_DEBUG);
            for(uint16_t i=0; i < param->data_ind.len; i++){
                slipPacket.onSlipByteReceived(param->data_ind.data[i]);
            }
            break;
        case ESP_SPP_CONG_EVT:
            ESP_LOGI(TAG, "ESP_SPP_CONG_EVT %d", param->cong.cong);
            m_bCongDetected = param->cong.cong;             // Store congestion state
            break;
        case ESP_SPP_WRITE_EVT:
            m_bCongDetected = param->write.cong;             // Store congestion state
            break;
        case ESP_SPP_SRV_OPEN_EVT:
            ESP_LOGI(TAG, "ESP_SPP_SRV_OPEN_EVT status:%d handle:%d, rem_bda:[%s]", param->srv_open.status,
                     param->srv_open.handle, bda2str(param->srv_open.rem_bda, bda_str, sizeof(bda_str)));

            m_bCongDetected = false;                        // Clear congestion state
            // Find free handle and store it
            for(uint32_t & m_BtSppHandle : m_BtSppHandles){
                if( m_BtSppHandle == INVALID_HANDLE){
                    m_BtSppHandle = param->srv_open.handle;
                    break;
                }
            }

            break;
        case ESP_SPP_SRV_STOP_EVT:
            ESP_LOGI(TAG, "ESP_SPP_SRV_STOP_EVT");
            break;
        case ESP_SPP_UNINIT_EVT:
            ESP_LOGI(TAG, "ESP_SPP_UNINIT_EVT");
            break;
        default:
            break;
    }
}


void N2kBt::Start() {

    instance = this;

    char bda_str[18] = {0};
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    bt_cfg.mode = ESP_BT_MODE_CLASSIC_BT;
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE(TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
        ESP_LOGE(TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_init()) != ESP_OK) {
        ESP_LOGE(TAG, "%s initialize bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(TAG, "%s enable bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_gap_register_callback(esp_bt_gap_cb)) != ESP_OK) {
        ESP_LOGE(TAG, "%s gap register failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_spp_register_callback(esp_spp_cb)) != ESP_OK) {
        ESP_LOGE(TAG, "%s spp register failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_spp_init(esp_spp_mode)) != ESP_OK) {
        ESP_LOGE(TAG, "%s spp init failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

#if (CONFIG_BT_SSP_ENABLED == true)
    /* Set default parameters for Secure Simple Pairing */
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));
#endif

    /*
     * Set default parameters for Legacy Pairing
     * Use variable pin, input pin code when pairing
     */
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
    esp_bt_pin_code_t pin_code;
    esp_bt_gap_set_pin(pin_type, 0, pin_code);

    ESP_LOGI(TAG, "Own address:[%s]", bda2str((uint8_t *)esp_bt_dev_get_address(), bda_str, sizeof(bda_str)));

}

void N2kBt::onTwaiFrameReceived(unsigned long id, unsigned char len, const unsigned char *buf) {

    unsigned char net_data[TWAI_FRAME_MAX_DLC + 5];
    int net_id = htonl(id);
    memcpy(net_data, &net_id, 4);
    memcpy(net_data + 4, &len, 1);
    for(int i = 0; i < len; i++){
        net_data[5 + i] = buf[i];
    }
    int total_len = 5 + len;

    slipPacket.EncodeAndSendPacket(net_data, total_len);
}

void N2kBt::onTwaiFrameTransmit(unsigned long id, unsigned char len, const unsigned char *buf) {
    onTwaiFrameReceived(id, len, buf);
}

void N2kBt::sendEncodedBytes(unsigned char *buf, unsigned char len) {

    // Buffer multiple SLIP packets into single SPP packet up to MAX_SPP_MTU bytes
    if( m_usSppBufferLen + len < MAX_SPP_MTU ){
        memcpy(m_ucSppBuffer + m_usSppBufferLen, buf, len);
        m_usSppBufferLen += len;
        return;
    }

    // Enough data accumulated send it
    flush();
}

void N2kBt::flush() {

    if( m_usSppBufferLen == 0){
        return;
    }

    if(m_bCongDetected){  // TODO check it for each handle separately
        m_usSppBufferLen = 0;  // Drop this data, it will be obsolete anyway
        return;
    }

    for(uint32_t & m_BtSppHandle : m_BtSppHandles){
        if( m_BtSppHandle != INVALID_HANDLE){
            esp_err_t err = esp_spp_write(m_BtSppHandle, m_usSppBufferLen, m_ucSppBuffer);
            if ( err == ESP_OK){
                ESP_LOG_BUFFER_HEX_LEVEL("BT_TX", m_ucSppBuffer, m_usSppBufferLen, ESP_LOG_DEBUG);
            }else{
                ESP_LOGE(TAG, "Error sending bytes %s", esp_err_to_name(err));
            }
        }
    }

    m_usSppBufferLen = 0;
}


void N2kBt::onPacketReceived(const unsigned char *recvbuf, unsigned char len) {
    int net_id = 0;
    memcpy(&net_id, recvbuf, 4);
    int twai_id = ntohl(net_id);
    unsigned char twai_len = recvbuf[4];

    if( twai_len <= TWAI_FRAME_MAX_DLC ){
        ESP_LOGD(TAG, "received %d bytes: id=%08X len=%02X", len, twai_id, twai_len);
        twaiBusSender.onSideIfcTwaiFrame(twai_id, twai_len, &recvbuf[5]);
    }else{
        ESP_LOGE(TAG, "Invalid DLC %02X", twai_len);
    }
}

