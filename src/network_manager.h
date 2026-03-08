#pragma once
#include <WiFiUdp.h>
#include <WiFiServer.h>
#include <WiFiClient.h>
#include "nvs.h"

extern WiFiUDP    udp;
extern IPAddress  server_address;
extern WiFiServer tcp_server;
extern WiFiClient tcp_client;

extern uint8_t  led_state;
extern uint32_t led_ts;
extern uint32_t blink_per;

void ip_from_cmd_arg(const char *arg, IPAddress &addr);
void manage_static_ip(nvs_settings_t *p);
void network_begin_udp_tcp(nvs_settings_t *p);
void network_manager_setup(nvs_settings_t *p);
void handle_wifi_reconnect(nvs_settings_t *p, uint32_t ts);
void handle_led_timeout(nvs_settings_t *p, uint32_t ts);
void handle_i2s_audio(void);
