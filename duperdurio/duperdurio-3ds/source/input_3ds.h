#pragma once

typedef struct {
    int left;
    int right;
    int run;       /* Y button held */
    int jump_down; /* A or B pressed this frame */
    int jump_held; /* A or B held */
    int pause;     /* Start pressed */
    int confirm;   /* A pressed */
    int back;      /* B pressed */
} InputState;

void input_poll_3ds(InputState *s);
