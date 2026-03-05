

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
// Include Uart capabilities for serial plotting and data transfer
#include <zephyr/drivers/uart.h>

/* STEP 2 - Include header for nrfx SAADC driver */
#include <nrfx_saadc.h>
/* STEP 3.1 - Declare the struct to hold the configuration for the SAADC channel used to sample the electrode voltage */
#define SAADC_INPUT_PIN NRFX_ANALOG_EXTERNAL_AIN0
static nrfx_saadc_channel_t channel = NRFX_SAADC_DEFAULT_CHANNEL_SE(SAADC_INPUT_PIN, 0);
//device uart definition
static const struct device *uart;
//Callback for when uart_tx buffer is done transmitting before it sends another
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

        uart = DEVICE_DT_GET(DT_NODELABEL(uart0));
        if (!device_is_ready(uart)) {
                return -ENODEV;
        }

        err = uart_callback_set(uart, uart_cb, NULL);
        if (err) {
                return err;
        }
        return 0;
}
/* STEP 3.2 - Declare the buffer to hold the SAAD sample value */
static int16_t sample;
/* STEP 4.1 - Define the battery sample interval */
#define ELECTRODE_SAMPLE_INTERVAL_US 500


/* STEP 4.3 - Add forward declaration of timer callback handler */
static void electrode_sample_timer_handler(struct k_timer * timer);


/* STEP 4.2 - Define the electrode sample timer instance */
K_TIMER_DEFINE(electrode_sample_timer,electrode_sample_timer_handler, NULL);


/* STEP 7.1 - Implement timer callback handler function */
void electrode_sample_timer_handler(struct k_timer *timer)
{




  /* Step 7.2 - Trigger the sampling */
  nrfx_err_t err = nrfx_saadc_mode_trigger();
        if (err != NRFX_SUCCESS) {
	return;
        }

  /* STEP 7.3 - Calculate and send voltage to serial plotter*/
  int electrode_voltage_mv = ((600) * sample) / ((1<<12));
  /* convert to ASCII line*/
  static char tx_buf[16];
  int len = snprintk(tx_buf, sizeof(tx_buf), "%d\t", electrode_voltage_mv);
  if (len<= 0) {
        return;
  }
  //ensure previous buffer has trsansmitted before transmitting the next sample
  k_sem_take(&uart_tx_done, K_FOREVER);
  err = uart_tx(uart, tx_buf, len, SYS_FOREVER_US);
  if (err) {
        k_sem_give(&uart_tx_done);
  }



}


static void configure_saadc(void)
{
        /* STEP 5.1 - Connect ADC interrupt to nrfx interrupt handler */
        IRQ_CONNECT(DT_IRQN(DT_NODELABEL(adc)),
                    DT_IRQ(DT_NODELABEL(adc), priority), nrfx_isr, nrfx_saadc_irq_handler, 0);

        
        /* STEP 5.2 - Initialize the nrfx_SAADC driver */
        nrfx_err_t err = nrfx_saadc_init(DT_IRQ(DT_NODELABEL(adc), priority));
        if (err != NRFX_SUCCESS) {
               
                return;
        }


        /* STEP 5.3 - Configure the SAADC channel */
        channel.channel_config.gain = NRF_SAADC_GAIN1;
                err = nrfx_saadc_channels_config(&channel, 1);
                if (err != NRFX_SUCCESS) {
                       
                        return;
                }


        /* STEP 5.4 - Configure nrfx_SAADC driver in simple and blocking mode */
        /* STEP 5.4 - Configure nrfx_SAADC driver in simple and blocking mode */
        err = nrfx_saadc_simple_mode_set(BIT(0),
                                 NRF_SAADC_RESOLUTION_12BIT,   // changed
                                 NRF_SAADC_OVERSAMPLE_DISABLED,
                                 NULL);
        if (err != NRFX_SUCCESS) {
                return;
        }

        /* STEP 5.5 - Set buffer where sample will be stored */
        err = nrfx_saadc_buffer_set(&sample, 1);
        if (err != NRFX_SUCCESS) {
                
                 return;
        }
        
}

int main(void)
{//ensure uart device is ready
         configure_saadc();
 /* Optional small delay if needed */
   // while (1) {

       
       // k_sleep(K_USEC(100));
  //  }

        /* STEP 6 - Start periodic timer for electrode sampling */
        k_timer_start(&electrode_sample_timer , K_NO_WAIT , K_USEC(ELECTRODE_SAMPLE_INTERVAL_US));

        if (uart_init() != 0) {
                return 0;
        }

   
    return 0;
}
