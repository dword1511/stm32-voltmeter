#ifndef __VOLTMETER_ANALOG_H__
#define __VOLTMETER_ANALOG_H__

/* Setup ADC. */
extern void     adc_setup(void);
/* Do one measurement. */
extern uint32_t adc_read(void);

#endif /* __VOLTMETER_ANALOG_H__ */
