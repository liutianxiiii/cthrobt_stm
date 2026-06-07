#pragma once
#include <stdint.h>

typedef enum {
    GRIPPER_OPEN   = 0,
    GRIPPER_CLOSED = 1,
} GripperState;

void         gripper_init(void);
void         gripper_close(void);
void         gripper_open(void);
GripperState gripper_get_state(void);
