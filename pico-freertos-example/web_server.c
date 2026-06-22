#include "web_server.h"

#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "app_state.h"
#include "task.h"

#ifndef ENABLE_WIFI_WEB
#define ENABLE_WIFI_WEB 0
#endif

#if ENABLE_WIFI_WEB

#include "cyw43.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "lwip/sockets.h"
#include "pico/cyw43_arch.h"

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif

#ifndef WIFI_AUTH
#define WIFI_AUTH CYW43_AUTH_WPA2_AES_PSK
#endif

enum {
    WEB_SERVER_PORT = 80,
    WIFI_CONNECT_TIMEOUT_MS = 30000,
    WIFI_RETRY_DELAY_MS = 15000,
    HTTP_BACKLOG = 4,
    HTTP_REQUEST_BUFFER_SIZE = 512,
    HTTP_RESPONSE_BUFFER_SIZE = 1536
};

static const char index_html[] =
    "<!doctype html><html lang=\"zh-Hant\"><head>"
    "<meta charset=\"utf-8\"><meta name=\"viewport\" "
    "content=\"width=device-width,initial-scale=1\">"
    "<title>Pico 2 W DHT11 Dashboard</title>"
    "<style>"
    ":root{color-scheme:dark light;font-family:system-ui,-apple-system,"
    "BlinkMacSystemFont,'Segoe UI',sans-serif}"
    "body{margin:0;background:#10131a;color:#eef2ff}"
    "main{max-width:880px;margin:auto;padding:24px}"
    "h1{font-size:1.7rem;margin:0 0 16px}"
    ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(190px,1fr));"
    "gap:14px}"
    ".card{background:#1b2130;border:1px solid #30384d;border-radius:16px;"
    "padding:18px;box-shadow:0 10px 28px #0005}"
    ".label{color:#9aa7bd;font-size:.9rem}.value{font-size:2.2rem;"
    "font-weight:750;margin-top:6px}.small{font-size:1rem;color:#c5cede}"
    ".ok{color:#68f59b}.bad{color:#ff7a8a}.switches{display:flex;gap:8px;"
    "flex-wrap:wrap}.sw{border-radius:999px;padding:8px 12px;background:#2a3144;"
    "color:#aeb8ca}.sw.on{background:#09a36d;color:white}"
    "code{background:#0b0d13;border-radius:6px;padding:2px 6px}"
    "footer{margin-top:18px;color:#8f9ab0;font-size:.9rem}"
    "</style></head><body><main>"
    "<h1>Pico 2 W DHT11 / DIP / LED</h1>"
    "<div class=\"grid\">"
    "<section class=\"card\"><div class=\"label\">Temperature</div>"
    "<div id=\"temp\" class=\"value\">--.- °C</div></section>"
    "<section class=\"card\"><div class=\"label\">Humidity</div>"
    "<div id=\"hum\" class=\"value\">--.- %</div></section>"
    "<section class=\"card\"><div class=\"label\">DHT11 status</div>"
    "<div id=\"dht\" class=\"value small\">waiting</div></section>"
    "<section class=\"card\"><div class=\"label\">Active switch</div>"
    "<div id=\"active\" class=\"value\">None</div></section>"
    "</div>"
    "<section class=\"card\" style=\"margin-top:14px\">"
    "<div class=\"label\">DIP switches</div>"
    "<div class=\"switches\">"
    "<span id=\"sw1\" class=\"sw\">S1 OFF</span>"
    "<span id=\"sw2\" class=\"sw\">S2 OFF</span>"
    "<span id=\"sw3\" class=\"sw\">S3 OFF</span>"
    "<span id=\"sw4\" class=\"sw\">S4 OFF</span>"
    "</div></section>"
    "<section class=\"card\" style=\"margin-top:14px\">"
    "<div class=\"label\">System</div>"
    "<p>Wi‑Fi IP: <code id=\"ip\">--</code></p>"
    "<p>Switch LED: <span id=\"led\">off</span></p>"
    "<p>Uptime: <span id=\"up\">0.0</span> s</p>"
    "<p>Last update: <span id=\"seen\">never</span></p>"
    "</section>"
    "<footer>Data source: <code>/api/status</code>. "
    "This page refreshes once per second.</footer>"
    "</main><script>"
    "function f(v,d){return Number(v).toFixed(d)}"
    "function sw(id,on){const e=document.getElementById('sw'+id);"
    "e.textContent='S'+id+' '+(on?'ON':'OFF');e.className='sw '+(on?'on':'')}"
    "async function tick(){try{"
    "const r=await fetch('/api/status',{cache:'no-store'});"
    "const s=await r.json();"
    "document.getElementById('ip').textContent=s.wifi.ip||'--';"
    "document.getElementById('up').textContent=f(s.uptime_ms/1000,1);"
    "document.getElementById('led').textContent=s.led.blinking?'blinking':'off';"
    "document.getElementById('active').textContent=s.dip.active_switch?('S'+s.dip.active_switch):'None';"
    "for(let i=1;i<=4;i++)sw(i,s.dip.switches[i-1]);"
    "const d=document.getElementById('dht');"
    "if(s.dht.ok){"
    "document.getElementById('temp').textContent=f(s.dht.temperature,1)+' °C';"
    "document.getElementById('hum').textContent=f(s.dht.humidity,1)+' %';"
    "d.textContent='OK';d.className='value small ok';"
    "}else{d.textContent=s.dht.status_text+' ('+s.dht.status+')';"
    "d.className='value small bad';}"
    "document.getElementById('seen').textContent=new Date().toLocaleTimeString();"
    "}catch(e){document.getElementById('seen').textContent='offline';}}"
    "tick();setInterval(tick,1000);"
    "</script></body></html>";

static bool wifi_credentials_configured(void) {
    return strlen(WIFI_SSID) > 0;
}

static bool get_sta_ip(char *buffer, size_t buffer_size) {
    struct netif *netif = &cyw43_state.netif[CYW43_ITF_STA];
    const ip4_addr_t *addr = netif_ip4_addr(netif);

    if (addr == NULL || addr->addr == 0) {
        if (buffer_size > 0) {
            buffer[0] = '\0';
        }
        return false;
    }

    ip4addr_ntoa_r(addr, buffer, (int)buffer_size);
    return true;
}

static void send_all(int socket_fd, const char *data, size_t length) {
    while (length > 0) {
        int sent = lwip_send(socket_fd, data, length, 0);
        if (sent <= 0) {
            return;
        }
        data += sent;
        length -= (size_t)sent;
    }
}

static void send_response(
    int socket_fd,
    const char *status,
    const char *content_type,
    const char *body) {
    char header[192];
    int body_length = (int)strlen(body);
    int header_length = snprintf(
        header,
        sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Cache-Control: no-store\r\n"
        "Connection: close\r\n"
        "Content-Length: %d\r\n"
        "\r\n",
        status,
        content_type,
        body_length);

    if (header_length <= 0) {
        return;
    }

    send_all(socket_fd, header, (size_t)header_length);
    send_all(socket_fd, body, (size_t)body_length);
}

static void send_status_json(int socket_fd) {
    app_state_snapshot_t snapshot;
    app_state_get_snapshot(&snapshot);

    uint8_t temperature = snapshot.dht_valid ? snapshot.dht.temperature : 0;
    uint8_t temperature_decimal =
        snapshot.dht_valid ? snapshot.dht.temperature_decimal : 0;
    uint8_t humidity = snapshot.dht_valid ? snapshot.dht.humidity : 0;
    uint8_t humidity_decimal =
        snapshot.dht_valid ? snapshot.dht.humidity_decimal : 0;

    char body[HTTP_RESPONSE_BUFFER_SIZE];
    int length = snprintf(
        body,
        sizeof(body),
        "{"
        "\"uptime_ms\":%lu,"
        "\"wifi\":{\"connected\":%s,\"ip\":\"%s\"},"
        "\"dht\":{"
        "\"ok\":%s,"
        "\"status\":%u,"
        "\"status_text\":\"%s\","
        "\"temperature\":%u.%u,"
        "\"humidity\":%u.%u"
        "},"
        "\"dip\":{"
        "\"mask\":%u,"
        "\"active_switch\":%u,"
        "\"switches\":[%s,%s,%s,%s]"
        "},"
        "\"led\":{\"blinking\":%s}"
        "}\n",
        (unsigned long)snapshot.uptime_ms,
        snapshot.wifi_connected ? "true" : "false",
        snapshot.wifi_ip,
        snapshot.dht_valid ? "true" : "false",
        (unsigned)snapshot.dht_status,
        dht11_status_string(snapshot.dht_status),
        (unsigned)temperature,
        (unsigned)temperature_decimal,
        (unsigned)humidity,
        (unsigned)humidity_decimal,
        (unsigned)snapshot.switch_mask,
        (unsigned)snapshot.active_switch,
        (snapshot.switch_mask & 0x01u) ? "true" : "false",
        (snapshot.switch_mask & 0x02u) ? "true" : "false",
        (snapshot.switch_mask & 0x04u) ? "true" : "false",
        (snapshot.switch_mask & 0x08u) ? "true" : "false",
        snapshot.switch_led_blinking ? "true" : "false");

    if (length <= 0 || length >= (int)sizeof(body)) {
        send_response(
            socket_fd,
            "500 Internal Server Error",
            "text/plain; charset=utf-8",
            "JSON response buffer too small\n");
        return;
    }

    send_response(socket_fd, "200 OK", "application/json", body);
}

static void handle_http_client(int socket_fd) {
    char request[HTTP_REQUEST_BUFFER_SIZE];
    int received = lwip_recv(socket_fd, request, sizeof(request) - 1, 0);
    if (received <= 0) {
        return;
    }

    request[received] = '\0';

    if (strncmp(request, "GET /api/status ", 16) == 0 ||
        strncmp(request, "GET /api/status?", 16) == 0) {
        send_status_json(socket_fd);
    } else if (strncmp(request, "GET / ", 6) == 0 ||
               strncmp(request, "GET /index.html ", 16) == 0) {
        send_response(socket_fd, "200 OK", "text/html; charset=utf-8", index_html);
    } else if (strncmp(request, "GET /favicon.ico ", 17) == 0) {
        send_response(socket_fd, "204 No Content", "text/plain", "");
    } else {
        send_response(
            socket_fd,
            "404 Not Found",
            "text/plain; charset=utf-8",
            "Not found\n");
    }
}

static void http_server_loop(void) {
    int listen_fd = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        printf("HTTP: socket() failed\n");
        return;
    }

    int yes = 1;
    lwip_setsockopt(
        listen_fd,
        SOL_SOCKET,
        SO_REUSEADDR,
        &yes,
        sizeof(yes));

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(WEB_SERVER_PORT);
    address.sin_addr.s_addr = PP_HTONL(INADDR_ANY);

    if (lwip_bind(
            listen_fd,
            (const struct sockaddr *)&address,
            sizeof(address)) < 0) {
        printf("HTTP: bind() failed\n");
        lwip_close(listen_fd);
        return;
    }

    if (lwip_listen(listen_fd, HTTP_BACKLOG) < 0) {
        printf("HTTP: listen() failed\n");
        lwip_close(listen_fd);
        return;
    }

    printf("HTTP: server listening on port %u\n", WEB_SERVER_PORT);

    for (;;) {
        int client_fd = lwip_accept(listen_fd, NULL, NULL);
        if (client_fd < 0) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        handle_http_client(client_fd);
        lwip_close(client_fd);
    }
}

static void wifi_web_task(void *parameter) {
    (void)parameter;

    if (!wifi_credentials_configured()) {
        printf(
            "Wi-Fi web server disabled: configure WIFI_SSID and "
            "WIFI_PASSWORD in CMake\n");
        app_state_set_wifi(false, "");
        vTaskDelete(NULL);
    }

    printf("Wi-Fi: initializing CYW43/lwIP\n");
    if (cyw43_arch_init() != 0) {
        printf("Wi-Fi: cyw43_arch_init() failed\n");
        app_state_set_wifi(false, "");
        vTaskDelete(NULL);
    }

    cyw43_arch_enable_sta_mode();

    for (;;) {
        printf("Wi-Fi: connecting to SSID \"%s\"\n", WIFI_SSID);
        app_state_set_wifi(false, "");

        int error = cyw43_arch_wifi_connect_timeout_ms(
            WIFI_SSID,
            WIFI_PASSWORD,
            WIFI_AUTH,
            WIFI_CONNECT_TIMEOUT_MS);
        if (error != 0) {
            printf("Wi-Fi: connect failed, error=%d\n", error);
            vTaskDelay(pdMS_TO_TICKS(WIFI_RETRY_DELAY_MS));
            continue;
        }

        char ip_address[16];
        for (uint retry = 0; retry < 50; ++retry) {
            if (get_sta_ip(ip_address, sizeof(ip_address))) {
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        if (!get_sta_ip(ip_address, sizeof(ip_address))) {
            printf("Wi-Fi: connected but no IPv4 address yet\n");
            vTaskDelay(pdMS_TO_TICKS(WIFI_RETRY_DELAY_MS));
            continue;
        }

        app_state_set_wifi(true, ip_address);
        printf("Wi-Fi: connected, open http://%s/\n", ip_address);

        http_server_loop();

        printf("HTTP: server stopped, retrying Wi-Fi\n");
        app_state_set_wifi(false, "");
        vTaskDelay(pdMS_TO_TICKS(WIFI_RETRY_DELAY_MS));
    }
}

void web_server_start(void) {
    BaseType_t created = xTaskCreate(
        wifi_web_task,
        "wifi-web",
        configMINIMAL_STACK_SIZE * 8,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL);
    configASSERT(created == pdPASS);
}

#else

void web_server_start(void) {
    printf("Wi-Fi web server disabled: this board/build has no CYW43 Wi-Fi\n");
}

#endif
