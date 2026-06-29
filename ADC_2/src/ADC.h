#ifndef ADC_H
#define ADC_H
#endif
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>

#if defined(CONFIG_NRFX_SAADC)
#include <nrfx_saadc.h>
#endif
#define SAADC_SAMPLE_INTERVAL_US 500
#define SAADC_BUFFER_SIZE 8
#define SAADC_INPUT_PIN1 NRFX_ANALOG_EXTERNAL_AIN5
#define TIMER_INSTANCE_NUMBER NRF_TIMER22
void configure_timer(void);
void configure_saadc(void);
void configure_ppi(void);
void adc_recording_start(void);
void adc_recording_stop(void);
bool adc_recording_is_active(void);
