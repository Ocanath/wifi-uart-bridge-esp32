#include "network_manager.h"
#include "config.h"
#include "array_process.h"
#include <WiFi.h>
#include <driver/i2s.h>

// Network globals
WiFiUDP    udp;
IPAddress  server_address;
WiFiServer tcp_server(0);   // port set at runtime via network_begin_udp_tcp()
WiFiClient tcp_client;

// LED globals
uint8_t  led_state = 1;
uint32_t led_ts    = 0;
uint32_t blink_per = PERIOD_DISCONNECTED;

// I2S audio — private to this file
static unsigned char  pld_buf[NUM_BYTES_HEADER + NUM_BYTES_SAMPLE_BUFFER];
static unsigned char *p_raw_sample_buf = pld_buf + NUM_BYTES_HEADER;
static uint16_t       sequence_number  = 0;

static i2s_config_t i2s_config = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate          = SAMPLE_RATE,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format       = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 4,
    .dma_buf_len          = 1024,
    .use_apll             = false,
    .tx_desc_auto_clear   = false,
    .fixed_mclk           = 0
};

static i2s_pin_config_t i2s_mic_pins = {
    .mck_io_num   = I2S_PIN_NO_CHANGE,
    .bck_io_num   = (gpio_num_t)I2S_MIC_SERIAL_CLOCK,
    .ws_io_num    = (gpio_num_t)I2S_MIC_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = (gpio_num_t)I2S_MIC_SERIAL_DATA
};

// ---------------------------------------------------------------------------

void ip_from_cmd_arg(const char *arg, IPAddress &addr)
{
    char ipstr[16] = {};
    for (int i = 0; arg[i] != '\0'; i++)
    {
        if (arg[i] != '\r' && arg[i] != '\n')
            ipstr[i] = arg[i];
    }
    addr.fromString(ipstr);
}

void manage_static_ip(nvs_settings_t *p)
{
    if (p == NULL)
        return;
    if (p->subnet == IPAddress((uint32_t)0))
        p->subnet.fromString("255.255.255.0");

    if (p->static_ip == IPAddress((uint32_t)IPV4_ADDR_ANY))
        Serial.printf("No static IP requested. Allow DNS\r\n");
    else
        Serial.printf("Static IP configured as: %s\r\n", p->static_ip.toString().c_str());

    if (WiFi.config(p->static_ip, p->gateway, p->subnet, p->dns))
        Serial.printf("Static IP configuration successs\r\n");
    else
        Serial.printf("Static IP configuration failed\r\n");
}

void network_begin_udp_tcp(nvs_settings_t *p)
{
    udp.begin(server_address, p->port);
    tcp_server.begin(p->port);
    tcp_server.setNoDelay(true);
}

void network_manager_setup(nvs_settings_t *p)
{
    if (p->led_pin == 0)
        p->led_pin = LED_PIN;
    if (p->ssid[0] == 0)
        snprintf(p->ssid, sizeof(p->ssid), "no ssid set!");

    pinMode(p->led_pin, OUTPUT);
    digitalWrite(p->led_pin, 1);

    manage_static_ip(p);

    Serial.begin(921600);

    if (p->baudrate == 0)
        p->baudrate = DEFAULT_BAUDRATE;
    Serial2.begin(p->baudrate, SERIAL_8N1, UART2_RX_PIN, UART2_TX_PIN);

    Serial.printf("\r\n\r\n Trying \'%s\' \'%s\'\r\n", p->ssid, p->password);

    WiFi.mode(WIFI_STA);
    WiFi.begin((const char *)p->ssid, (const char *)p->password);
    uint32_t timeout = millis() + 3000;
    while (millis() < timeout && WiFi.status() != WL_CONNECTED);
    int connected = WiFi.waitForConnectResult();
    if (connected != WL_CONNECTED)
        Serial.printf("Connection to network %s failed for an unknown reason\r\n", (const char *)p->ssid);
    else
        Serial.printf("Connected to network %s\r\n", (const char *)p->ssid);

    network_begin_udp_tcp(p);
    server_address = IPAddress((uint32_t)IPV4_ADDR_ANY);

    if (p->use_i2s)
    {
        i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
        i2s_set_pin(I2S_NUM_0, &i2s_mic_pins);
    }
}

void handle_wifi_reconnect(nvs_settings_t *p, uint32_t ts)
{
    static uint32_t retry_ts = 0;
    if (ts - retry_ts > blink_per)
    {
        retry_ts = ts;
        if (WiFi.status() != WL_CONNECTED)
        {
            WiFi.disconnect();
            WiFi.begin((const char *)p->ssid, (const char *)p->password);
            network_begin_udp_tcp(p);
            digitalWrite(p->led_pin, led_state);
            led_state = ~led_state & 1;
        }
    }
}

void handle_led_timeout(nvs_settings_t *p, uint32_t ts)
{
    if (ts - led_ts > 1)
    {
        led_state = 0;
        digitalWrite(p->led_pin, led_state);
    }
}

void handle_i2s_audio(void)
{
    if (!gl_prefs.use_i2s)
        return;

    size_t bytes_read = 0;
    i2s_read(I2S_NUM_0, p_raw_sample_buf, NUM_BYTES_SAMPLE_BUFFER, &bytes_read, portMAX_DELAY);
    if (bytes_read == 0)
        return;

    sequence_number++;
    pld_buf[0] = (sequence_number & 0x00FF);
    pld_buf[1] = (sequence_number & 0xFF00) >> 8;

    uint32_t tick = millis();
    pld_buf[0] = tick & 0x000000FF;
    pld_buf[1] = (tick & 0x0000FF00) >> 8;
    pld_buf[2] = (tick & 0x00FF0000) >> 16;
    pld_buf[3] = (tick & 0xFF000000) >> 24;

    process_32bit_to_16bit(p_raw_sample_buf, bytes_read, 65536 / 64, &bytes_read);

    udp.beginPacket(udp.remoteIP(), AUDIO_UDP_PORT);
    udp.write((uint8_t *)pld_buf, NUM_BYTES_HEADER + bytes_read);
    udp.endPacket();
}
