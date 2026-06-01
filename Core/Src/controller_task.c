#include "controller_task.h"
#include "lwip/api.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CTRL_PORT   7777
#define RX_BUF_SIZE 512
#define OUT_BUF_SIZE 256

/* ── HAT direction table ─────────────────────────────────────────────────── */
static const char *hat_label(int x, int y) {
    if (x ==  0 && y ==  1) return "UP";
    if (x ==  0 && y == -1) return "DOWN";
    if (x == -1 && y ==  0) return "LEFT";
    if (x ==  1 && y ==  0) return "RIGHT";
    if (x ==  1 && y ==  1) return "UP-RIGHT";
    if (x == -1 && y ==  1) return "UP-LEFT";
    if (x ==  1 && y == -1) return "DN-RIGHT";
    if (x == -1 && y == -1) return "DN-LEFT";
    return NULL;
}

/* ── JSON parser ─────────────────────────────────────────────────────────── */
static void parse_and_print(const char *json)
{
    char out[OUT_BUF_SIZE];
    int  pos = 0;

    /* --- buttons: find "NAME":true pairs --- */
    const char *p = strstr(json, "\"buttons\":{");
    if (p) {
        p += 11;
        while (*p && *p != '}') {
            if (*p != '"') { p++; continue; }
            p++;
            const char *nstart = p;
            int nlen = 0;
            while (*p && *p != '"') { p++; nlen++; }
            if (*p == '"') p++;
            if (*p == ':') p++;
            if (strncmp(p, "true", 4) == 0 && pos + nlen + 2 < OUT_BUF_SIZE) {
                memcpy(out + pos, nstart, nlen);
                pos += nlen;
                out[pos++] = ' ';
            }
            while (*p && *p != ',' && *p != '}') p++;
            if (*p == ',') p++;
        }
    }

    /* --- axes: find "NAME":non-zero pairs --- */
    p = strstr(json, "\"axes\":{");
    if (p) {
        p += 8;
        while (*p && *p != '}') {
            if (*p != '"') { p++; continue; }
            p++;
            const char *nstart = p;
            int nlen = 0;
            while (*p && *p != '"') { p++; nlen++; }
            if (*p == '"') p++;
            if (*p == ':') p++;

            const char *vstart = p;
            while (*p && *p != ',' && *p != '}') p++;
            int vlen = (int)(p - vstart);

            char vbuf[24];
            if (vlen > 0 && vlen < (int)sizeof(vbuf)) {
                memcpy(vbuf, vstart, vlen);
                vbuf[vlen] = '\0';
                float val = strtof(vbuf, NULL);
                if (val != 0.0f) {
                    int n = snprintf(out + pos, OUT_BUF_SIZE - pos,
                                     "%.*s=%+.3f  ", nlen, nstart, val);
                    if (n > 0) pos += n;
                }
            }
            if (*p == ',') p++;
        }
    }

    /* --- hat: [x,y] --- */
    p = strstr(json, "\"hat\":[");
    if (p) {
        p += 7;
        char *end;
        int hx = (int)strtol(p, &end, 10);
        if (*end == ',') end++;
        int hy = (int)strtol(end, NULL, 10);
        const char *lbl = hat_label(hx, hy);
        if (lbl && pos + 16 < OUT_BUF_SIZE) {
            int n = snprintf(out + pos, OUT_BUF_SIZE - pos, "HAT:%s", lbl);
            if (n > 0) pos += n;
        }
    }

    out[pos] = '\0';
#ifdef SIMULATION_MODE
    printf("[CTRL] %s\n", pos > 0 ? out : "(idle)");
#else
    printf("[CTRL] %s\r\n", pos > 0 ? out : "(idle)");
#endif
}

/* ── Per-connection handler ──────────────────────────────────────────────── */
static void handle_client(struct netconn *client)
{
    struct netbuf *nbuf;
    void    *data;
    uint16_t len;
    char     rx[RX_BUF_SIZE];
    uint16_t rx_pos = 0;

    printf("[CTRL] Controller connected\r\n");

    while (netconn_recv(client, &nbuf) == ERR_OK) {
        do {
            netbuf_data(nbuf, &data, &len);
            const char *src = (const char *)data;
            for (uint16_t i = 0; i < len; i++) {
                char c = src[i];
                if (c == '\n') {
                    rx[rx_pos] = '\0';
                    if (rx_pos > 0)
                        parse_and_print(rx);
                    rx_pos = 0;
                } else if (rx_pos < RX_BUF_SIZE - 1) {
                    rx[rx_pos++] = c;
                }
            }
        } while (netbuf_next(nbuf) >= 0);
        netbuf_delete(nbuf);
    }

    netconn_close(client);
    netconn_delete(client);
    printf("[CTRL] Controller disconnected\r\n");
}

/* ── FreeRTOS task entry ─────────────────────────────────────────────────── */
void StartControllerTask(void const *argument)
{
    (void)argument;

    printf("[CTRL] Task started\r\n");

#ifdef SIMULATION_MODE
    /* Simulation: read gamepad JSON lines from UART3 (injected by Renode bridge) */
    printf("[CTRL] Simulation mode - reading gamepad data from UART3\r\n");
    extern UART_HandleTypeDef huart3;
    char rx_buf[512];
    uint16_t rx_pos = 0;
    uint8_t rx_byte;
    for (;;) {
        if (HAL_UART_Receive(&huart3, &rx_byte, 1, 10) == HAL_OK) {
            if (rx_byte == '\n') {
                rx_buf[rx_pos] = '\0';
                if (rx_pos > 0) {
                    parse_and_print(rx_buf);
                    rx_pos = 0;
                }
            } else if (rx_byte != '\r' && rx_pos < sizeof(rx_buf) - 1) {
                rx_buf[rx_pos++] = (char)rx_byte;
            }
        }
    }
#endif

    /* Wait for LwIP to call netif_set_up (happens in MX_LWIP_Init) */
    extern struct netif gnetif;
    while (!netif_is_up(&gnetif)) {
        osDelay(10);
    }
    printf("[CTRL] LwIP stack ready\r\n");

    struct netconn *server = netconn_new(NETCONN_TCP);
    if (!server) {
        printf("[CTRL] Failed to create netconn\r\n");
        for (;;) osDelay(1000);
    }

    netconn_bind(server, IP_ADDR_ANY, CTRL_PORT);
    netconn_listen(server);
    printf("[CTRL] Listening on port %d\r\n", CTRL_PORT);

    for (;;) {
        struct netconn *client;
        if (netconn_accept(server, &client) == ERR_OK)
            handle_client(client);
    }
}
