#ifndef __VOLTMETER_ANALOG_H__
#define __VOLTMETER_ANALOG_H__

/* Setup ADC. */
extern void     analog_setup(void);
/* Do one measurement. */
extern uint32_t analog_read(void);

#endif /* __VOLTMETER_ANALOG_H__ */
