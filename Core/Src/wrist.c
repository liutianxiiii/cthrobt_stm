#include "wrist.h"
#include <stdint.h>
#include <stdio.h>

/*
 * Wrist control — RDS3288 servo via CAN1
 *
 * TODO: fill in the three constants below after confirming with HW team.
 *   WRIST_NODE_ID    : CAN Node ID of the RDS3288 driver board
 *   WRIST_CMD_CW90   : byte sequence that commands 90° clockwise position
 *   WRIST_CMD_HOME   : byte sequence that commands 0° (home) position
 *
 * See HARDWARE_PROTOCOL_TODO.md for the full list of questions.
 */

#ifndef SIMULATION_MODE
#include "can_bus.h"

/* ── TODO ────────────────────────────────────────────────────────────────── */
#define WRIST_NODE_ID       0x02U         /* TODO: confirm Node ID           */
#define WRIST_FRAME_ID      (0x600U + WRIST_NODE_ID) /* TODO: confirm frame ID scheme */

static const uint8_t WRIST_CMD_CW90[]  = { 0x00, 0x00 }; /* TODO: 90° CW command */
static const uint8_t WRIST_CMD_HOME[]  = { 0x00, 0x00 }; /* TODO: 0° home command */
#define WRIST_CMD_LEN  2                  /* TODO: confirm frame data length */
/* ───────────────────────────────────────────────────────────────────────── */

#endif /* !SIMULATION_MODE */

static WristPosition g_position = WRIST_HOME;

void wrist_init(void)
{
    g_position = WRIST_HOME;
#ifdef SIMULATION_MODE
    printf("[WRIST] init — position: HOME\n");
#else
    printf("[WRIST] init — position: HOME\r\n");
#endif
}

void wrist_go_cw90(void)
{
    if (g_position == WRIST_CW90) return;
    g_position = WRIST_CW90;
#ifndef SIMULATION_MODE
    can_send(WRIST_FRAME_ID, WRIST_CMD_CW90, WRIST_CMD_LEN);
#endif
#ifdef SIMULATION_MODE
    printf("[WRIST] CW90\n");
#else
    printf("[WRIST] CW90\r\n");
#endif
}

void wrist_go_home(void)
{
    if (g_position == WRIST_HOME) return;
    g_position = WRIST_HOME;
#ifndef SIMULATION_MODE
    can_send(WRIST_FRAME_ID, WRIST_CMD_HOME, WRIST_CMD_LEN);
#endif
#ifdef SIMULATION_MODE
    printf("[WRIST] HOME\n");
#else
    printf("[WRIST] HOME\r\n");
#endif
}

WristPosition wrist_get_position(void)
{
    return g_position;
}
