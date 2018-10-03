#ifndef __VOLTMETER_DISP_H__
#define __VOLTMETER_DISP_H__

/* Initializes display control block. GPIO power should be already on. */
extern void disp_setup(void);
/* Update display contents. */
extern void disp_update(uint32_t voltage_mv, bool blinker);

#ifdef DISP_NEEDS_REFRESH
/* Called periodically to refresh the display. */
extern void disp_refresh(void);
#endif /* DISP_NEEDS_REFRESH */

#endif /* __VOLTMETER_DISP_H__ */
