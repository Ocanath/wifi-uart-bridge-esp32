#include "console_cmds.h"
#include "parse_console.h"
#include "nvs.h"
#include "network_manager.h"
#include <WiFi.h>
#include <Arduino.h>

int cmd_match(const char *in, const char *cmd)
{
    int i = 0;
    for (i = 0; cmd[i] != '\0'; i++)
    {
        if (in[i] == '\0') return -1;
        if (in[i] != cmd[i]) return -1;
    }
    return i;
}

void handle_console_cmds(void)
{
    get_console_lines();
    if (gl_console_cmd.parsed != 0)
        return;

    uint8_t match = 0;
    uint8_t save  = 0;
    int cmp = -1;

    //////////////////////////////////////////////////////////////////////
    cmp = cmd_match((const char *)gl_console_cmd.buf, "setstaticip ");
    if (cmp > 0)
    {
        match = 1;
        const char *arg = (const char *)(&gl_console_cmd.buf[cmp]);
        ip_from_cmd_arg(arg, gl_prefs.static_ip);
        Serial.printf("Setting static ip to %s\r\n", gl_prefs.static_ip.toString());
        save = 1;
    }

    //////////////////////////////////////////////////////////////////////
    cmp = cmd_match((const char *)gl_console_cmd.buf, "setsubnetmask ");
    if (cmp > 0)
    {
        match = 1;
        const char *arg = (const char *)(&gl_console_cmd.buf[cmp]);
        ip_from_cmd_arg(arg, gl_prefs.subnet);
        Serial.printf("Setting subnet mask to %s\r\n", gl_prefs.subnet.toString());
        save = 1;
    }

    //////////////////////////////////////////////////////////////////////
    cmp = cmd_match((const char *)gl_console_cmd.buf, "setgateway ");
    if (cmp > 0)
    {
        match = 1;
        const char *arg = (const char *)(&gl_console_cmd.buf[cmp]);
        ip_from_cmd_arg(arg, gl_prefs.gateway);
        Serial.printf("Setting gateway to %s\r\n", gl_prefs.gateway.toString());
        save = 1;
    }

    //////////////////////////////////////////////////////////////////////
    cmp = cmd_match((const char *)gl_console_cmd.buf, "setdns ");
    if (cmp > 0)
    {
        match = 1;
        const char *arg = (const char *)(&gl_console_cmd.buf[cmp]);
        ip_from_cmd_arg(arg, gl_prefs.dns);
        Serial.printf("Setting dns to %s\r\n", gl_prefs.dns.toString());
        save = 1;
    }

    //////////////////////////////////////////////////////////////////////
    cmp = cmd_match((const char *)gl_console_cmd.buf, "readstaticipsettings");
    if (cmp > 0)
    {
        match = 1;
        Serial.printf("Desired static ip %s\r\n", gl_prefs.static_ip.toString());
        Serial.printf("Subnet mask %s\r\n", gl_prefs.subnet.toString());
        Serial.printf("Gateway is %s\r\n", gl_prefs.gateway.toString());
        Serial.printf("DNS is %s\r\n", gl_prefs.dns.toString());
    }

    //////////////////////////////////////////////////////////////////////
    cmp = strcmp((const char *)gl_console_cmd.buf, "ipconfig\r");
    if (cmp == 0)
    {
        match = 1;
        if (WiFi.status() == WL_CONNECTED)
            Serial.printf("Connected to: %s\r\n", gl_prefs.ssid);
        else
            Serial.printf("Not connected to: %s\r\n", gl_prefs.ssid);
        Serial.printf("UDP server on port: %d\r\n", gl_prefs.port);
        Serial.printf("TCP server on port: %d\r\n", gl_prefs.port);
        if (tcp_client && tcp_client.connected())
            Serial.printf("TCP client connected: %s\r\n", tcp_client.remoteIP().toString().c_str());
        else
            Serial.printf("No TCP client connected\r\n");
        Serial.printf("Server Response Offset: %d\r\n", gl_prefs.reply_offset);
        Serial.printf("IP address: %s\r\n", WiFi.localIP().toString().c_str());
        if (gl_prefs.static_ip != IPAddress((uint32_t)0))
        {
            Serial.printf("Requested Static IP: %s\r\n", gl_prefs.static_ip.toString());
            Serial.printf("Subnet Mask: %s\r\n", gl_prefs.subnet.toString());
            Serial.printf("Gateway: %s\r\n", gl_prefs.gateway.toString());
            Serial.printf("DNS: %s\r\n", gl_prefs.dns.toString());
        }
    }

    //////////////////////////////////////////////////////////////////////
    cmp = cmd_match((const char *)gl_console_cmd.buf, "udpconfig\r");
    if (cmp > 0)
    {
        match = 1;
        Serial.printf("UDP server on port: %d\r\n", gl_prefs.port);
    }

    //////////////////////////////////////////////////////////////////////
    cmp = cmd_match((const char *)gl_console_cmd.buf, "setssid ");
    if (cmp > 0)
    {
        match = 1;
        const char *arg = (const char *)(&gl_console_cmd.buf[cmp]);
        for (int i = 0; i < WIFI_MAX_SSID_LEN; i++)
            gl_prefs.ssid[i] = '\0';
        for (int i = 0; arg[i] != '\0'; i++)
        {
            if (arg[i] != '\r' && arg[i] != '\n')
                gl_prefs.ssid[i] = arg[i];
        }
        Serial.printf("Changing ssid to: %s\r\n", gl_prefs.ssid);
        save = 1;
    }

    //////////////////////////////////////////////////////////////////////
    cmp = cmd_match((const char *)gl_console_cmd.buf, "setname ");
    if (cmp > 0)
    {
        match = 1;
        const char *arg = (const char *)(&gl_console_cmd.buf[cmp]);
        for (int i = 0; i < DEVICE_NAME_LEN; i++)
		{
            gl_prefs.name[i] = '\0';
		}
        for (int i = 0; arg[i] != '\0'; i++)
        {
            if (arg[i] != '\r' && arg[i] != '\n')
                gl_prefs.name[i] = arg[i];
        }
        Serial.printf("Changing device name to: %s\r\n", gl_prefs.name);
        save = 1;
    }

    //////////////////////////////////////////////////////////////////////
    cmp = cmd_match((const char *)gl_console_cmd.buf, "settargetip ");
    if (cmp > 0)
    {
        match = 1;
        const char *arg = (const char *)(&gl_console_cmd.buf[cmp]);
        char copy[15] = {0};
        Serial.printf("Raw str arg: ");
        for (int i = 0; arg[i] != 0 && arg[i] != '\r' && arg[i] != '\n'; i++)
        {
            copy[i] = arg[i];
            Serial.printf("%0.2X", arg[i]);
        }
        Serial.printf(": %s\r\n", copy);
        IPAddress addr;
        if (addr.fromString((const char *)copy) == true)
            Serial.printf("Parsed IP address successfully\r\n");
        else
            Serial.printf("Invalid IP string entered\r\n");
        gl_prefs.remote_target_ip = (uint32_t)addr;
        Serial.printf("%X\r\n", gl_prefs.remote_target_ip);
        IPAddress parseconfirm(gl_prefs.remote_target_ip);
        Serial.printf("Target IP: %s\r\n", parseconfirm.toString().c_str());
        save = 1;
    }

    //////////////////////////////////////////////////////////////////////
    cmp = cmd_match((const char *)gl_console_cmd.buf, "fixedtarget ");
    if (cmp > 0)
    {
        match = 1;
        const char *arg = (const char *)(&gl_console_cmd.buf[cmp]);
        int argcmp = cmd_match(arg, "enable");
        if (argcmp > 0)
        {
            gl_prefs.en_fixed_target = 1;
            IPAddress parseconfirm(gl_prefs.remote_target_ip);
            Serial.printf("Enabling Fixed Target: %s\r\n", parseconfirm.toString().c_str());
        }
        argcmp = cmd_match(arg, "disable");
        if (argcmp > 0)
        {
            gl_prefs.en_fixed_target = 0;
            Serial.printf("Disabling Fixed Target\r\n");
        }
        save = 1;
    }

    //////////////////////////////////////////////////////////////////////
    cmp = cmd_match((const char *)gl_console_cmd.buf, "setpwd ");
    if (cmp > 0)
    {
        match = 1;
        const char *arg = (const char *)(&gl_console_cmd.buf[cmp]);
        for (int i = 0; i < WIFI_MAX_PWD_LEN; i++)
            gl_prefs.password[i] = '\0';
        for (int i = 0; arg[i] != '\0'; i++)
        {
            if (arg[i] != '\r' && arg[i] != '\n')
                gl_prefs.password[i] = arg[i];
        }
        Serial.printf("Changing pwd to: %s\r\n", gl_prefs.password);
        save = 1;
    }

    //////////////////////////////////////////////////////////////////////
    cmp = cmd_match((const char *)gl_console_cmd.buf, "setport ");
    if (cmp > 0)
    {
        match = 1;
        const char *arg = (const char *)(&gl_console_cmd.buf[cmp]);
        char *tmp;
        int port = strtol(arg, &tmp, 10);
        Serial.printf("Changing port to: %d\r\n", port);
        gl_prefs.port = port;
        save = 1;
    }

    //////////////////////////////////////////////////////////////////////
    cmp = cmd_match((const char *)gl_console_cmd.buf, "setaudio ");
    if (cmp > 0)
    {
        match = 1;
        const char *arg = (const char *)(&gl_console_cmd.buf[cmp]);
        char *tmp;
        int setting = (strtol(arg, &tmp, 10)) & 1;
        gl_prefs.use_i2s = setting;
        Serial.printf("Using audio: %d\r\n", setting);
        save = 1;
    }

    //////////////////////////////////////////////////////////////////////
    cmp = cmd_match((const char *)gl_console_cmd.buf, "readaudio");
    if (cmp > 0)
    {
        match = 1;
        Serial.printf(gl_prefs.use_i2s ? "I2S Mic Enabled\r\n" : "I2S Mic Disabled\r\n");
    }

    //////////////////////////////////////////////////////////////////////
    cmp = cmd_match((const char *)gl_console_cmd.buf, "setledpin ");
    if (cmp > 0)
    {
        match = 1;
        const char *arg = (const char *)(&gl_console_cmd.buf[cmp]);
        char *tmp;
        uint8_t pin = (uint8_t)(strtol(arg, &tmp, 10));
        Serial.printf("Changing led pin to: %d\r\n", pin);
        gl_prefs.led_pin = pin;
        save = 1;
    }

    //////////////////////////////////////////////////////////////////////
    cmp = cmd_match((const char *)gl_console_cmd.buf, "readledpin");
    if (cmp > 0)
    {
        match = 1;
        Serial.printf("LED Pin is %d\r\n", gl_prefs.led_pin);
    }

    //////////////////////////////////////////////////////////////////////
    cmp = cmd_match((const char *)gl_console_cmd.buf, "setbaudrate ");
    if (cmp > 0)
    {
        match = 1;
        const char *arg = (const char *)(&gl_console_cmd.buf[cmp]);
        char *tmp;
        unsigned long baudrate = strtol(arg, &tmp, 10);
        if (baudrate > 2000000) baudrate = 2000000;
        if (baudrate < 9600)    baudrate = 9600;
        Serial.printf("Changing baudrate to: %d\r\n", baudrate);
        gl_prefs.baudrate = baudrate;
        save = 1;
    }

    //////////////////////////////////////////////////////////////////////
    cmp = cmd_match((const char *)gl_console_cmd.buf, "readbaudrate");
    if (cmp > 0)
    {
        match = 1;
        Serial.printf("Baudrate is: %d\r\n", gl_prefs.baudrate);
    }

    //////////////////////////////////////////////////////////////////////
    cmp = cmd_match((const char *)gl_console_cmd.buf, "setTXoff ");
    if (cmp > 0)
    {
        match = 1;
        const char *arg = (const char *)(&gl_console_cmd.buf[cmp]);
        char *tmp;
        int offset = strtol(arg, &tmp, 10);
        Serial.printf("Setting port offset to: %d\r\n", offset);
        gl_prefs.reply_offset = offset;
        save = 1;
    }

    //////////////////////////////////////////////////////////////////////
    cmp = cmd_match((const char *)gl_console_cmd.buf, "setencoding ");
    if (cmp > 0)
    {
        match = 1;
        const char *arg = (const char *)(&gl_console_cmd.buf[cmp]);
        char *tmp;
        int mode = strtol(arg, &tmp, 10);
		if(mode >= 0 && mode <= 1)
        {
			// Serial.printf("Setting serial mode to: %d\r\n", mode);
			if(mode == COBS_FRAMING)
			{
				Serial.printf("Setting mode to COBS\n");
			}
			else
			{
				Serial.printf("Setting mode to PPP\n");
			}
        	gl_prefs.serial_frame_encoding_mode = mode;
		}
		else
		{
			Serial.printf("Invalid serial encoding mode selection\n");
		}
        save = 1;
    }

	//////////////////////////////////////////////////////////////////////
    cmp = cmd_match((const char *)gl_console_cmd.buf, "readencoding");
    if (cmp > 0)
    {
        match = 1;
		if(gl_prefs.serial_frame_encoding_mode == COBS_FRAMING)
		{
			Serial.printf("Using mode: COBS\n");
		}
		else
		{
			Serial.printf("Using mode: PPP\n");
		}
    }



	//////////////////////////////////////////////////////////////////////
    cmp = cmd_match((const char *)gl_console_cmd.buf, "setspam ");
    if (cmp > 0)
    {
        match = 1;
        const char *arg = (const char *)(&gl_console_cmd.buf[cmp]);
        char *tmp;
        int mode = strtol(arg, &tmp, 10);
		if(mode >= 0 && mode <= 1)
        {
			if(mode == 0)
			{
				Serial.printf("Disabling spam\n");
			}
			else
			{
				Serial.printf("Spam mode ON\n");
			}
        	gl_prefs.spam_target = mode;
		}
		else
		{
			Serial.printf("Invalid spam setting\n");
		}
        save = 1;
    }

	//////////////////////////////////////////////////////////////////////
    cmp = cmd_match((const char *)gl_console_cmd.buf, "readspam");
    if (cmp > 0)
    {
        match = 1;
		if(gl_prefs.spam_target == 0)
		{
			Serial.printf("Spam disabled\n");
		}
		else
		{
			Serial.printf("Spam enabled\n");
		}
    }

    //////////////////////////////////////////////////////////////////////
    cmp = cmd_match((const char *)gl_console_cmd.buf, "readcred");
    if (cmp > 0)
    {
        match = 1;
        Serial.printf("SSID: \'");
        for (int i = 0; gl_prefs.ssid[i] != 0; i++)
        {
            char c = gl_prefs.ssid[i];
            if (c >= 0x1f && c <= 0x7E) Serial.printf("%c", c);
            else Serial.printf("%0.2X", c);
        }
        Serial.printf("\'\r\n");
        Serial.printf("Password: \'");
        for (int i = 0; gl_prefs.password[i] != 0; i++)
        {
            char c = gl_prefs.password[i];
            if (c >= 0x1f && c <= 0x7E) Serial.printf("%c", c);
            else Serial.printf("%0.2X", c);
        }
        Serial.printf("\'\r\n");
    }

    //////////////////////////////////////////////////////////////////////
    cmp = cmd_match((const char *)gl_console_cmd.buf, "reconnect\r");
    if (cmp > 0)
    {
        match = 1;
        Serial.printf("restarting wifi connection...\r\n");
        WiFi.disconnect();
        WiFi.begin((const char *)gl_prefs.ssid, (const char *)gl_prefs.password);
        network_begin_udp_tcp(&gl_prefs);
    }

    //////////////////////////////////////////////////////////////////////
    cmp = cmd_match((const char *)gl_console_cmd.buf, "restart\r");
    if (cmp > 0)
    {
        Serial.printf("restarting chip...\r\n");
        ESP.restart();
    }

    /****************************** Cleanup ******************************/
    if (match == 0)
        Serial.printf("Failed to parse: %s\r\n", gl_console_cmd.buf);

    if (save != 0)
    {
        int nb = preferences.putBytes("settings", &gl_prefs, sizeof(nvs_settings_t));
        Serial.printf("Saved %d bytes\r\n", nb);
    }

    for (int i = 0; i < BUFFER_SIZE; i++)
        gl_console_cmd.buf[i] = 0;
    gl_console_cmd.parsed = 1;
}
