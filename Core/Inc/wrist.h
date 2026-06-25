#pragma once

typedef enum { WRIST_HOME = 0, WRIST_CW90 = 1 } WristPosition;

void          wrist_init(void);
void          wrist_go_cw90(void);   /* rotate to 90° clockwise  */
void          wrist_go_home(void);   /* rotate back to 0°        */
WristPosition wrist_get_position(void);
