/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/uart.h>
/* STEP 2 - Include header for nrfx SAADC driver */
#include <nrfx_saadc.h>

/* STEP 3.1 - Declare the struct to hold the configuration for the SAADC channel used to sample the battery voltage */
#define SAADC_INPUT_PIN NRFX_ANALOG_EXTERNAL_AIN0
static nrfx_saadc_channel_t channel = NRFX_SAADC_DEFAULT_CHANNEL_SE(SAADC_INPUT_PIN, 0);
//define uart device
static const struct device *uart=DEVICE_DT_GET(DT_NODELABEL(uart0));

static K_SEM_DEFINE(uart_tx_done,1, 1);


static void uart_cb(const struct device *dev,
struct uart_event *evt,
void *user_data)

{
        switch (evt->type) {
                case UART_TX_DONE:
                k_sem_give(&uart_tx_done);
                break;
                default:
                break;
        }
}
  // ensure the device is ready, return error if not     
static int uart_init(void)
{
        int err;
        //printk("Uart initil");

        uart = DEVICE_DT_GET(DT_NODELABEL(uart0));
        if (!device_is_ready(uart)) {
                return -ENODEV;
        }

        err = uart_callback_set(uart, uart_cb, NULL);
        if (err) {
                return err;
        }
       // printk("Uart is init");
        return 0;
}

/* STEP 3.2 - Declare the buffer to hold the SAAD sample value */
static int16_t sample;

/* STEP 4.1 - Define the battery sample interval */
#define BATTERY_SAMPLE_INTERVAL_US 500
/* STEP 4.3 - Add forward declaration of timer callback handler */
static void battery_sample_timer_handler(struct k_timer * timer);

/* STEP 4.2 - Define the battery sample timer instance */
K_TIMER_DEFINE(battery_sample_timer, battery_sample_timer_handler, NULL);

/* STEP 7.1 - Implement timer callback handler function */
void battery_sample_timer_handler(struct k_timer *timer)
{

  /* Step 7.2 - Trigger the sampling */
  nrfx_err_t err = nrfx_saadc_mode_trigger();
        if (err != 0) {
	//printk("nrfx_saadc_mode_trigger error: %08x", err);
	//printk("ADC reading");
        return;
}

  /* STEP 7.3 - Calculate and print voltage */
  int battery_voltage = ((600*6) * sample) / ((1<<12));
      // printk("Voltage: %d\n\r", battery_voltage);


static uint8_t tx_buf[2]; // 2 (Start) + 2 (Size) + 2 (Data)

// Frame Start (CD AB)
//tx_buf[0] = 0xCD;
//tx_buf[1] = 0xAB;

//Payload Size (2 bytes for a uint16)
//tx_buf[2] = 0x02; 
//tx_buf[3] = 0x00;

// (Little Endian)
tx_buf[0] = (uint8_t)(battery_voltage & 0xFF); //low byte
tx_buf[1] = (uint8_t)((battery_voltage >> 8) & 0xFF); //high byte

k_sem_take(&uart_tx_done, K_FOREVER);

int ret = uart_tx(uart, tx_buf, sizeof(tx_buf), SYS_FOREVER_US);
if (ret) {
    return;
}









}


static void configure_saadc(void)
{
        /* STEP 5.1 - Connect ADC interrupt to nrfx interrupt handler */
        IRQ_CONNECT(DT_IRQN(DT_NODELABEL(adc)),
            DT_IRQ(DT_NODELABEL(adc), priority),
            nrfx_isr, nrfx_saadc_irq_handler, 0);
        
        /* STEP 5.2 - Initialize the nrfx_SAADC driver */
        nrfx_err_t err = nrfx_saadc_init(DT_IRQ(DT_NODELABEL(adc), priority));
        if (err != 0) {
       // printk("nrfx_saadc_mode_trigger error: %08x", err);
        return;
        }

        /* STEP 5.3 - Configure the SAADC channel */
        channel.channel_config.gain = NRF_SAADC_GAIN1_6;
        err = nrfx_saadc_channels_config(&channel, 1);
        if (err != 0) {
      //  printk("nrfx_saadc_channels_config error: %08x", err);
        return;
        }

        /* STEP 5.4 - Configure nrfx_SAADC driver in simple and blocking mode */
        err = nrfx_saadc_simple_mode_set(BIT(0),
                                 NRF_SAADC_RESOLUTION_12BIT,
                                 NRF_SAADC_OVERSAMPLE_DISABLED,
                                 NULL);
        if (err != 0) {
     //   printk("nrfx_saadc_simple_mode_set error: %08x", err);
         return;
        }


        /* STEP 5.5 - Set buffer where sample will be stored */
        err = nrfx_saadc_buffer_set(&sample, 1);
        if (err != 0) {
     //   printk("nrfx_saadc_buffer_set error: %08x", err);
         return;
        }

        /* STEP 6 - Start periodic timer for battery sampling */
        k_timer_start(&battery_sample_timer, K_NO_WAIT, K_USEC(BATTERY_SAMPLE_INTERVAL_US));


}

int main(void)

{
            /* STEP 4.2 - Verify that the UART device is ready */
        if (!device_is_ready(uart)) {
     //   printk("UART device not ready\n");
        return 1;

    }
    configure_saadc();
    uart_init();
    int ret;
        ret = uart_callback_set(uart, uart_cb, NULL);
         if (ret) {
        return ret;
        }
        

        
        
       
        k_sleep(K_FOREVER);
        return 0;
}