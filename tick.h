#ifndef __VOLTMETER_TICK_H__
#define __VOLTMETER_TICK_H__


#include <stdbool.h>


/* Initializes ticks. */
extern void tick_setup(void);
/* Check if it is time for auto-power-off. */
extern bool tick_auto_poweroff(void);

#endif /* __VOLTMETER_TICK_H__ */
