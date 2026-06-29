#include <zephyr/types.h>
#include <string.h>
#include <stdio.h>

#include "data.h"
#include "config.h"
#include "spi.h"
#include "timer.h"
#include "ADC.h"

stim_setting settings;
stim_setting pending_settings;
uint8_t ble_received_data[BLE_DATA_BUFFER_SIZE];
uint16_t ble_data_length;

void data_init_defaults(void)
{
	settings.DAC_amplitude = CONFIG_STIM_AMPLITUDE;
	settings.pulse_width = (uint16_t)CONFIG_PULSE_WIDTH_US;
	settings.frequency = (uint16_t)CONFIG_STIM_FREQUENCY_HZ;
	pending_settings = settings;
	update_dac1_amplitude(settings.DAC_amplitude);
	update_dac2_amplitude(settings.DAC_amplitude);
}

#if defined(CONFIG_BT)

static void stimulation_engine_start(const stim_setting *s)
{
	if (s->frequency == 0u || s->pulse_width == 0u) {
		printf("Stim start ignored: invalid frequency or pulse width\n");
		return;
	}

	update_stim_frequency(s->frequency);
	update_pulse_width(s->pulse_width);
	update_dac1_amplitude(s->DAC_amplitude);
	update_dac2_amplitude(s->DAC_amplitude);
	timer_stimulation_enable();
}

void process_received_data(stim_setting *active_settings, uint8_t *data, uint16_t len)
{
    if (len == 0) {
        printf("BLE packet empty\n");
        return;
    }

    const uint8_t ctrl = data[0];    // read ctrl byte FIRST

    /* --- Stimulation control --- */
    if (ctrl == STIM_CTRL_START || ctrl == STIM_CTRL_STOP) {
        if (len != STIM_PACKET_LEN) {
            printf("Stim packet length %u (expected %u)\n", len, (unsigned int)STIM_PACKET_LEN);
            return;
        }
        stim_setting payload;
        memcpy(&payload, data + 1, sizeof(payload));
        const bool running = timer_stimulation_is_enabled();

        if (ctrl == STIM_CTRL_START) {
            memcpy(active_settings, &payload, sizeof(stim_setting));
            memcpy(&pending_settings, &payload, sizeof(stim_setting));
            if (running) {
                update_dac1_amplitude(payload.DAC_amplitude);
                update_dac2_amplitude(payload.DAC_amplitude);
                return;
            }
            stimulation_engine_start(active_settings);
            return;
        }

        if (ctrl == STIM_CTRL_STOP) {
            if (running) {
                stim_timer_request_stop_after_burst();
            } else {
                memcpy(&pending_settings, &payload, sizeof(stim_setting));
            }
            return;
        }
    }

    /* --- ADC recording control --- */
    if (ctrl == ADC_CTRL_START) {
        if (len != ADC_PACKET_LEN) {
            printf("ADC packet length %u (expected %u)\n", len, (unsigned int)ADC_PACKET_LEN);
            return;
        }
        adc_recording_start();
        return;
    }

    if (ctrl == ADC_CTRL_STOP) {
        if (len != ADC_PACKET_LEN) {
            printf("ADC packet length %u (expected %u)\n", len, (unsigned int)ADC_PACKET_LEN);
            return;
        }
        adc_recording_stop();
        return;
    }

    printf("BLE unknown control byte 0x%02x\n", ctrl);
}

#else /* !CONFIG_BT */

void process_received_data(stim_setting *active_settings, uint8_t *data, uint16_t len)
{
	ARG_UNUSED(active_settings);
	ARG_UNUSED(data);
	ARG_UNUSED(len);
}

#endif /* CONFIG_BT */