#include <cstring>
#include <esp_netif.h>
#include <esp_event.h>
#include <esp_wifi_default.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_system.h>
#include <lwip/sockets.h>
#include "N2kWifi.h"

#include "KnownSsidList.h"

static const char *TAG = "imu2nmea_N2kWifi";
static const char * BROADCAST_IPV4_ADDR  = "255.255.255.255";

N2kWifi::N2kWifi(SideTwaiBusInterface &twaiBusSender)
        : twaiBusSender(twaiBusSender) {
    txFrameQueue = xQueueCreate(20, sizeof(NetworkMsg));
    rxFrameQueue = xQueueCreate(10, sizeof(NetworkMsg));
}

static void tx_frame_task(void *me ) {
    ((N2kWifi *) me)->TransmitFrameTask();
}

static void rx_frame_task(void *me ) {
    ((N2kWifi *) me)->ReceiveFrameTask();
}

static void wifi_event_handler(void* self, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if ( event_base == WIFI_EVENT ) {
        ESP_LOGI(TAG, "Wifi event %d", event_id);
        ((N2kWifi *) self)->WifiEventHandler(event_id, event_data);
    }
}

static esp_event_loop_handle_t wifi_loop_handle;

static void sys_event_handler(void* , esp_event_base_t event_base,
                              int32_t event_id, void* event_data)
{
    // Forward the event to the Wi-Fi loop
    esp_event_post_to(wifi_loop_handle, event_base, event_id, event_data, 0, 0);
}

void N2kWifi::Start() {

    xTaskCreate(
            tx_frame_task,         /* Function that implements the task. */
            "TxFrameTask",            /* Text name for the task. */
            16 * 1024,        /* Stack size in words, not bytes. */
            (void *) this,  /* Parameter passed into the task. */
            tskIDLE_PRIORITY + 1, /* Priority at which the task is created. */
            nullptr);        /* Used to pass out the created task's handle. */

    xTaskCreate(
            rx_frame_task,         /* Function that implements the task. */
            "RxFrameTask",            /* Text name for the task. */
            16 * 1024,        /* Stack size in words, not bytes. */
            (void *) this,  /* Parameter passed into the task. */
            tskIDLE_PRIORITY + 1, /* Priority at which the task is created. */
            nullptr);        /* Used to pass out the created task's handle. */

    // Create ESP event loop
    esp_event_loop_args_t loop_args = {
            .queue_size = 10,
            .task_name = "WiFiLoop",
            .task_priority = tskIDLE_PRIORITY + 1,
            .task_stack_size = 48 * 1024,
            .task_core_id = 0
    };

    ESP_ERROR_CHECK(esp_event_loop_create(&loop_args, &wifi_loop_handle));

    // Register event handler
    ESP_ERROR_CHECK(esp_event_handler_register_with(wifi_loop_handle, WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, this));

    StartWifi();
}

void N2kWifi::TransmitFrameTask() {

    while (true) {
        if (isWifiConnected) {
            BroadcastCanFramesOverUdp();
        }else{
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}

void N2kWifi::ReceiveFrameTask() {

    while (true) {
        if (isWifiConnected){
            vTaskDelay(1000 / portTICK_PERIOD_MS);  // Let network to settle down
            xQueueReset(rxFrameQueue);
            ListenForUdpInput();
        }else{
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}


bool N2kWifi::CheckScanResults(char *ssid, char *password) {
    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    memset(ap_info, 0, sizeof(ap_info));

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    uint16_t ap_count = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);

    for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++) {
        ESP_LOGI(TAG, "SSID \t\t[%s]", ap_info[i].ssid);
        for (auto item : knownSsidList) {
            if (strcmp((char *)ap_info[i].ssid, item.ssid) == 0) {
                ESP_LOGI(TAG, "Found known SSID %s", item.ssid);
                strcpy((char *)ssid, item.ssid);
                strcpy((char *)password, item.password);
                return true;
            }
        }
    }
    return false;
}

void N2kWifi::StartWifi() {
    ESP_LOGI(TAG, "Start wifi");

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &sys_event_handler, this, nullptr));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void N2kWifi::WifiEventHandler(int32_t event_id, void *) {
    char ssid[32]="xxx";
    char pass[64]="xxx";
    switch (event_id) {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "Start scan");
            esp_wifi_scan_start(nullptr, false);
            break;
        case WIFI_EVENT_SCAN_DONE:
            ESP_LOGI(TAG, "Scan done");
            if (CheckScanResults(ssid, pass)){
                ESP_LOGI(TAG, "Will connect to %s", ssid);
                // Read default config
                wifi_config_t wifi_config = {};
                ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_config));
                // Set desired ssid and password
                strcpy((char *)wifi_config.sta.ssid, ssid);
                strcpy((char *)wifi_config.sta.password, pass);
                ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
                ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
                // Start the connection  process
                ESP_ERROR_CHECK(esp_wifi_connect());
            }else{
                ESP_LOGI(TAG, "No known SSID found, scan again");
                esp_wifi_scan_start(nullptr, false);
            }
            break;
        case WIFI_EVENT_STA_CONNECTED:
            StartServer();
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            StopServer();
            ESP_LOGI(TAG, "Lost WiFi connection, scan again");
            esp_wifi_scan_start(nullptr, false);
            break;
        default:
            break;
    }

}

void N2kWifi::StartServer() {
    isWifiConnected = true;
}

void N2kWifi::StopServer() {
    isWifiConnected = false;
}

int N2kWifi::CreateBoundUdpSocket()
{
    int err;
    struct sockaddr_in saddr = { 0 ,  PF_INET, 0, { 0 }, { 0 } } ;

    int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Failed to create socket. Error %d", errno);
        return -1;
    }

    int enable = 1;
    if (-1 == setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(enable)))
    {
        ESP_LOGE(TAG, "Failed to set socket option SO_REUSEADDR. Error %d", errno);
        goto err;
    }

    // Bind the socket to any address
    saddr.sin_family = PF_INET;
    saddr.sin_port = htons(UDP_RX_PORT);
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    err = bind(sock, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
    if (err < 0) {
        ESP_LOGE(TAG, "Failed to bind socket. Error %d", errno);
        goto err;
    }

    // All set, socket is configured for sending and receiving
    return sock;

    err:
    close(sock);
    return -1;
}

void N2kWifi::ListenForUdpInput() {

    int sock = CreateBoundUdpSocket();
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return;
    }
    ESP_LOGI(TAG, "Socket created");

    struct timeval tv = {
            .tv_sec = 2,
            .tv_usec = 0,
    };
    fd_set rfds;

    while (isWifiConnected) {
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);

        int s = select(sock + 1, &rfds, nullptr, nullptr, &tv);
        if (s < 0) {
            ESP_LOGE(TAG, "Select failed: errno %d", errno);
            break;
        }
        if (s == 0) {
            ESP_LOGD(TAG, "Select timeout");
            continue;
        }

        if (FD_ISSET(sock, &rfds)) {
            // Incoming datagram received
            unsigned char recvbuf[MAX_UDP_FRAME_SIZE];
            char raddr_name[32] = { 0 };

            struct sockaddr_storage raddr{}; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(raddr);
            int len = recvfrom(sock, recvbuf, sizeof(recvbuf), 0,
                               (struct sockaddr *)&raddr, &socklen);
            if (len < 0) {
                ESP_LOGE(TAG, "multicast recvfrom failed: errno %d", errno);
                break;
            }

            // Get the sender's address as a string
            if (raddr.ss_family == PF_INET) {
                    inet_ntoa_r(((struct sockaddr_in *)&raddr)->sin_addr,
                                raddr_name, sizeof(raddr_name)-1);
            }
            int net_id = 0;
            memcpy(&net_id, recvbuf, 4);
            int twai_id = ntohl(net_id);
            unsigned char twai_len = recvbuf[4];

            if( twai_len <= TWAI_FRAME_MAX_DLC ){
                ESP_LOGD(TAG, "received %d bytes from %s: id=%08X len=%02X", len, raddr_name, twai_id, twai_len);
                twaiBusSender.onSideIfcTwaiFrame(twai_id, twai_len, &recvbuf[5]);
            }else{
                ESP_LOGE(TAG, "Invalid DLC %02X", twai_len);
            }
        }
    }
}

void N2kWifi::BroadcastCanFramesOverUdp() {

    int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Failed to create socket. Error %d", errno);
        return;
    }

    /* Set socket to allow broadcast */
    int broadcastPermission = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void *) &broadcastPermission,
                   sizeof(broadcastPermission)) < 0){
        ESP_LOGE(TAG, "Failed to get broadcast permission. Error %d", errno);
        close(sock);
        return;
    }

    struct sockaddr_in broadcastAddr{};                 /* Broadcast address */
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));   /* Zero out structure */
    broadcastAddr.sin_family = AF_INET;                 /* Internet address family */
    broadcastAddr.sin_addr.s_addr = inet_addr(BROADCAST_IPV4_ADDR);/* Broadcast IP address */
    broadcastAddr.sin_port = htons(UDP_TX_PORT);         /* Broadcast port */
    broadcastAddr.sin_len = sizeof(broadcastAddr);

    xQueueReset(txFrameQueue);

    while (isWifiConnected){
        NetworkMsg msg{};

        portBASE_TYPE evt_res = xQueueReceive(txFrameQueue, &msg, 1000 / portTICK_PERIOD_MS);

        if (evt_res == pdTRUE) {
            if (msg.type == NetworkMsgType::WIFI_DISCONNECTED) {
                ESP_LOGI(TAG, "Wifi disconnected");
                break;
            }
            if( msg.type == NetworkMsgType::CAN_FRAME) {
                unsigned char udp_data[MAX_UDP_FRAME_SIZE];
                int net_id = htonl(msg.id);
                memcpy(udp_data, &net_id, 4);
                memcpy(udp_data + 4, &msg.len, 1);
                for(int i = 0; i < msg.len; i++){
                    udp_data[5 + i] = msg.pdu[i];
                }
                int len = 5 + msg.len;
                int sent = sendto(sock, udp_data, len, 0, (struct sockaddr *)&broadcastAddr, sizeof(broadcastAddr));
                if (sent < 0) {
                    ESP_LOGE(TAG, "Failed to send CAN frame. Error %d", errno);
                    break;
                }else{
                    ESP_LOGD(TAG, "Sent CAN frame %d bytes", sent);
                }
            }
        }
    }
    close(sock);
}

void N2kWifi::onTwaiFrameReceived(unsigned long id, unsigned char len, const unsigned char *buf) {
    ESP_LOGD(TAG, "onTwaiFrameReceived CAN frame %08lX %02X", id, len);
    NetworkMsg msg{};
    msg.type = NetworkMsgType::CAN_FRAME;
    msg.id = id;
    msg.len = len;
    memcpy(msg.pdu, buf, len);
    if( !xQueueSend(txFrameQueue, &msg, 0) ){
        ESP_LOGD(TAG, "onTwaiFrameReceived Failed to send CAN frame to queue");
    }
}

void N2kWifi::onTwaiFrameTransmit(unsigned long id, unsigned char len, const unsigned char *buf) {
    ESP_LOGD(TAG, "onTwaiFrameTransmit CAN frame %08lX %02X", id, len);
    NetworkMsg msg{};
    msg.type = NetworkMsgType::CAN_FRAME;
    msg.id = id;
    msg.len = len;
    memcpy(msg.pdu, buf, len);
    if ( !xQueueSend(txFrameQueue, &msg, 0) ){
        ESP_LOGD(TAG, "onTwaiFrameTransmit Failed to send CAN frame to queue");
    }
}


