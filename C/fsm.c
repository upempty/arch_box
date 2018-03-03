#include <stdlib.h>

typedef enum {
    ST_ANY,
    ST_INIT,
    ST_IDLE = 0,
    ST_LED_ON,
    ST_LED_OFF,
    ST_FLASHING,
    ST_STOPPED
} state_t;

typedef enum {
    EV_ANY,
    EV_BUTTON_PUSHED,
    EV_TIME_OUT,
    EV_NONE
} event_t;

typedef struct {
    state_t state;
    event_t event;
    state_t (*func)(void);
} state_trans_t;

state_t main_init(void);
state_t main_work(void);
state_t main_idle(void);

state_t main_init(void)
{
    return ST_IDLE;
}

state_t main_work(void)
{
    return ST_IDLE;
}

state_t main_idle(void)
{
    return ST_IDLE;
}

static state_trans_t state_trans[] = {
{ ST_INIT, EV_ANY, &main_init },
{ ST_IDLE, EV_BUTTON_PUSHED, &main_idle },
{ ST_LED_ON, EV_BUTTON_PUSHED, &main_work },
};

static state_t state = ST_INIT;
static event_t event = EV_ANY;

event_t get_next_event(void)
{
	  return EV_NONE;
}

#define STATETRANS_COUNT (sizeof(state_trans)/sizeof(*state_trans))

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    int i;
    state_t state;

    //while(state)
    while(1)
    {
        event = get_next_event();
        
        for(i = 0; i < STATETRANS_COUNT; i++)
        {
            if((state_trans[i].state == state) || (state_trans[i].state == ST_ANY))
            {
                if((state_trans[i].event == event) || (state_trans[i].event == EV_ANY))
                {
                    state = (state_trans[i].func)();
                    break;
                }
            }
        }
	  }
}
