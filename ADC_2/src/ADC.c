

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(Chronos_adc, LOG_LEVEL_DBG);

/* STEP 2 - Include header for nrfx drivers */
#include <nrfx_saadc.h>
#include <nrfx_timer.h>
#include <helpers/nrfx_gppi.h>
// Include headers for zephyr SPI driver//
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include "BLE.h"
#include "ADC.h"





//#define SAADC_INPUT_PIN2 NRFX_ANALOG_EXTERNAL_AIN2
static nrfx_saadc_channel_t channel1 = NRFX_SAADC_DEFAULT_CHANNEL_SE(SAADC_INPUT_PIN1, 0);
//static nrfx_saadc_channel_t channel2 = NRFX_SAADC_DEFAULT_CHANNEL_SE(SAADC_INPUT_PIN2, 0);
//Define the SPI INIT
//SPI worc set changed to 16, see what happens, change back to 8 if not helpful
/* STEP 3.2 - Declaring an instance of nrfx_timer for TIMER2. */
nrfx_timer_t timer_instance = NRFX_TIMER_INSTANCE(TIMER_INSTANCE_NUMBER);
//New added up here hmmmmmm lets see
/* STEP 4.2 - Declare the buffers for the SAADC */
static int16_t saadc_sample_buffer[4][SAADC_BUFFER_SIZE];
//establishing a message queue for SAADC filled buffer pointers
K_MSGQ_DEFINE(ble_msgq, sizeof(int16_t *), 4, 4);
//Begins the spi thread of communication//
static void ble_thread(void *a, void *b, void *c) {
    k_sem_take(&ble_init_ok, K_FOREVER);
    int16_t *buf;
    while (1) {
        k_msgq_get(&ble_msgq, &buf, K_FOREVER);
        if (!current_conn) {
            continue;
        }
        ble_send_samples(buf, SAADC_BUFFER_SIZE);
        LOG_INF("Sample sent succesfully!");
    }
}
K_THREAD_DEFINE(ble_tid, 4096, ble_thread, NULL, NULL, NULL, 5, 0, 0);

/* STEP 4.3 - Declare variable used to keep track of which buffer was last assigned to the SAADC driver */
static uint32_t saadc_current_buffer = 2;



void configure_timer(void)
{
    nrfx_err_t err;

    /* STEP 3.3 - Declaring timer config and intialize nrfx_timer instance. */
    nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG(1000000);
    err = nrfx_timer_init(&timer_instance, &timer_config, NULL);
    if (err !=0) {
        LOG_ERR("nrfx_timer_init error: %08x", err);
        return;
    }

    /* STEP 3.4 - Set compare channel 0 to generate event every SAADC_SAMPLE_INTERVAL_US. */
    uint32_t timer_ticks = nrfx_timer_us_to_ticks(&timer_instance, SAADC_SAMPLE_INTERVAL_US);
    nrfx_timer_extended_compare(&timer_instance, NRF_TIMER_CC_CHANNEL0, timer_ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false);

}

static void saadc_event_handler(nrfx_saadc_evt_t const * p_event)
{
    nrfx_err_t err;
    switch (p_event->type)
    {

        case NRFX_SAADC_EVT_READY:
        
           /* STEP 5.1 - Buffer is ready, timer (and sampling) can be started. */
           nrfx_timer_enable(&timer_instance);

            break;                        
            
       case NRFX_SAADC_EVT_BUF_REQ:
        uint32_t next = saadc_current_buffer % 4;
           err = nrfx_saadc_buffer_set(saadc_sample_buffer[next], SAADC_BUFFER_SIZE);
            if (err != 0) {
            LOG_ERR("nrfx_saadc_buffer_set error: %08x  init", err);
            return;
            }
            saadc_current_buffer++;
            break;
         //   uint32_t next = saadc_current_buffer % 4;
           // err = nrfx_saadc_buffer_set(saadc_sample_buffer[next], SAADC_BUFFER_SIZE);
            //if (err != 0) {
            //LOG_ERR("nrfx_saadc_buffer_set error: %08x", err);
            //return;
            //}
            //saadc_current_buffer++;
        
            /* STEP 5.2 - Set up the next available buffer. Alternate through buffers 0-4 */
           
            

           // err = nrfx_saadc_buffer_set(saadc_sample_buffer[(saadc_current_buffer++)%4], SAADC_BUFFER_SIZE);
            //if (err != 0) {
              //  LOG_ERR("nrfx_saadc_buffer_set error: %08x", err);
                //return;
            //}

            //break;

        case NRFX_SAADC_EVT_DONE:
           

           // err = nrfx_saadc_buffer_set(saadc_sample_buffer[next], SAADC_BUFFER_SIZE);
            //if (err != 0) {
            //LOG_ERR("nrfx_saadc_buffer_set error: %08x init", err);
            //return;
            //}
           // saadc_current_buffer++;
         /* STEP 5.3 - Buffer has been filled. Do something with the data and proceed */
          /*  int64_t average = 0;
            int16_t max = INT16_MIN;
            int16_t min = INT16_MAX;
            int16_t current_value; 
            for(int i=0; i < p_event->data.done.size; i++){
                current_value = ((int16_t *)(p_event->data.done.p_buffer))[i];
                average += current_value;
                if(current_value > max){
                    max = current_value;
                }
                if(current_value < min){
                    min = current_value;
                }
            }
            average = average/p_event->data.done.size;
            LOG_INF("SAADC buffer at 0x%x filled with %d samples", (uint32_t)p_event->data.done.p_buffer, p_event->data.done.size);
            LOG_INF("AVG=%d, MIN=%d, MAX=%d", (int16_t)average, min, max);
            break;*/
        //default:
          //  LOG_INF("Unhandled SAADC evt %d", p_event->type);
           // break;

            /* STEP 5.3 - Buffer has been filled. Do something with the data and proceed */ //This will be changed to instead pass the buffer through SPI to fRAM//

         //  { int16_t *buf   = ((int16_t *)(p_event->data.done.p_buffer));
           // uint16_t count = (p_event->data.done.size);   /* number of int16_t samples */

           //  spi_send_samples(buf, count);

                /* Re-arm the same buffer for the next fill */
           // }
           //puts message of filled buffer pointer into queue
           int16_t *buf = p_event->data.done.p_buffer;
           int ret = k_msgq_put(&ble_msgq, &buf, K_NO_WAIT);
            if (ret != 0) {
            LOG_ERR("BLE msgq full! Buffer dropped at 0x%x", (uint32_t)buf);
            }
            

            break;
             default:
            LOG_INF("Unhandled SAADC evt %d", p_event->type);
            break;
        }     
       

}


void configure_saadc(void)
{
    nrfx_err_t err;

    /* STEP 4.4 - Connect ADC interrupt to nrfx interrupt handler */
    IRQ_CONNECT(DT_IRQN(DT_NODELABEL(adc)),
              DT_IRQ(DT_NODELABEL(adc), priority),
            nrfx_isr, nrfx_saadc_irq_handler, 0);
    /* STEP 4.5 - Initialize the nrfx_SAADC driver */
    err = nrfx_saadc_init(DT_IRQ(DT_NODELABEL(adc), priority));
    if (err!= 0) {
        LOG_ERR("nrfx_saadc_init error: %08x", err);
        return;
    }

    /* STEP 4.7 - Change gain config in default config and apply channel configuration */
    channel1.channel_config.gain = NRF_SAADC_GAIN1_4;
    channel1.channel_config.reference= NRF_SAADC_REFERENCE_INTERNAL;
    err = nrfx_saadc_channels_config(&channel1, 1);
    if (err != 0) {
        LOG_ERR("nrfx_saadc_channels_config error : %08x", err);
        return;
    }
    //channel2.channel_config.gain = NRF_SAADC_GAIN1_4;
    //channel2.channel_config.reference= NRF_SAADC_REFERENCE_INTERNAL;
    //err = nrfx_saadc_channels_config(&channel2, 1);
    //if (err != 0) {
      //  LOG_ERR("nrfx_saadc_channels_config error : %08x",err);
        //return;
    //}

    /* STEP 4.8 - Configure channel 0 in advanced mode with event handler (non-blocking mode) */

    nrfx_saadc_adv_config_t saadc_adv_config = NRFX_SAADC_DEFAULT_ADV_CONFIG;
    err = nrfx_saadc_advanced_mode_set(BIT(0),
                                        NRF_SAADC_RESOLUTION_12BIT,
                                        &saadc_adv_config,
                                        saadc_event_handler);
    if (err != 0) {
        LOG_ERR("nrfx_saadc_advanced_mode_set error: %08x", err);
        return;
    }

    /* STEP 4.9 - Configure two buffers to make use of double-buffering feature of SAADC */
    err = nrfx_saadc_buffer_set(saadc_sample_buffer[0], SAADC_BUFFER_SIZE);
    if (err != 0) {
        LOG_ERR("nrfx_saadc_buffer_set error: %08x 1", err);
        return;
    }
    err = nrfx_saadc_buffer_set(saadc_sample_buffer[1], SAADC_BUFFER_SIZE);
    if (err != 0) {
        LOG_ERR("nrfx_saadc_buffer_set error: %08x 2", err);
        return;
    }
   // err = nrfx_saadc_buffer_set(saadc_sample_buffer[2], SAADC_BUFFER_SIZE);
    //if (err != 0) {
      //  LOG_ERR("nrfx_saadc_buffer_set error: %08x 3", err);
        //return;
    //}
    //err = nrfx_saadc_buffer_set(saadc_sample_buffer[3], SAADC_BUFFER_SIZE);
    //if (err != 0) {
      //  LOG_ERR("nrfx_saadc_buffer_set error: %08x 4", err);
        //return;
    //}

    /* STEP 4.10 - Trigger the SAADC. This will not start sampling, but will prepare buffer for sampling triggered through PPI */
    err = nrfx_saadc_mode_trigger();
    if (err != 0) {
        LOG_ERR("nrfx_saadc_mode_trigger error: %08x", err);
        return;
    }

}

void configure_ppi(void)
{
    nrfx_err_t err;
    /* STEP 6.1 - Declare variables used to hold the (D)PPI channel number */
    nrfx_gppi_handle_t gppi_handle_sample;
    nrfx_gppi_handle_t gppi_handle_start;

    /* STEP 6.2 - Trigger task sample from timer */
    err = nrfx_gppi_conn_alloc( nrfx_timer_compare_event_address_get(&timer_instance, NRF_TIMER_CC_CHANNEL0),
                                nrf_saadc_task_address_get(NRF_SAADC, NRF_SAADC_TASK_SAMPLE), &gppi_handle_sample);
       if (err != 0) {
        LOG_ERR("nrfx_gppi_conn_alloc error: %08x", err);
        return;
       }                        
    /* STEP 6.3 - Trigger task start from end event */
    err = nrfx_gppi_conn_alloc(nrf_saadc_event_address_get(NRF_SAADC, NRF_SAADC_EVENT_END),
                                nrf_saadc_task_address_get(NRF_SAADC, NRF_SAADC_TASK_START), &gppi_handle_start);
    if (err != 0) {
        LOG_ERR("nrfx_gppi_conn_alloc error: %08x", err);
        return;
    } 
    /* STEP 6.4 - Enable both (D)PPI channels */ 
    nrfx_gppi_conn_enable(gppi_handle_sample);
nrfx_gppi_conn_enable(gppi_handle_start);
}

