/**
 ****************************************************************************************
 *
 * @file bk_modem_uart.c
 *
 * @brief bk modem uart communicates with modem file.
 *        This module implements the UART interface for communication between the host
 *        and the modem, supporting both standard UART mode and NIC mode.
 *
 ****************************************************************************************
 */
//#if (CONFIG_BK_MODEM_UART)
#include <os/os.h>
#include "bk_modem_main.h"
#include "bk_modem_dte.h"
#include "bk_modem_uart.h"
#include <driver/gpio.h>
#include <driver/uart.h>
#include <driver/uart_types.h>
#include <modules/pm.h>
#include <driver/hal/hal_uart_types.h>
#include <os/mem.h>
#include "../components/bk_common/include/bk_misc.h"
#include "../middleware/driver/bk7259_ap/gpio_driver.h"

/* Global variable definitions - thread and queue handles */
static beken_thread_t bk_modem_uart_rx_thread = NULL;     /* UART receive thread handle */
static beken_thread_t bk_modem_uart_tx_thread = NULL;     /* UART transmit thread handle */
static beken_queue_t bk_modem_uart_rx_queue = NULL;       /* UART receive message queue */
static beken_queue_t bk_modem_uart_tx_queue = NULL;       /* UART transmit message queue */

uint8_t *g_uart_rx_buff = NULL;                           /* UART receive buffer pointer */
static uint32_t s_uart_tx_no = 0;                         /* UART transmit sequence number */
/* Set by SRDY falling-edge ISR to indicate a (>=1us) low pulse from slave was captured.
 * Used by the TX path to detect the ~50us ACK pulse that can't be caught by polling. */
static volatile uint8_t s_srdy_pulse_flag = 0;
beken2_timer_t uar_nic_sleep_timer;                       /* Sleep timer in NIC mode */
static uint32_t s_modem_uart_baud_rate = BK_MODEM_UART_5M2_BAUD;  /* Current UART baud rate */

/**
 * @brief Calculate checksum in UART NIC mode
 * @param header Pointer to UART NIC frame header
 * @return Calculated checksum
 */
static uint8_t bk_modem_uart_nic_csum(UART_NIC_HD_T *header)
{
    uint32_t csum = header->bytes[0] + header->bytes[1] + header->bytes[2];
    return (uint8_t)((csum >> 8) ^ csum ^ 0x3);
}

/**
 * @brief Check and reset sleep timer
 *        This function stops the sleep timer if it's running and restarts it,
 *        preventing the modem from entering sleep mode while there's active communication.
 */
static void bk_modem_uart_sleep_check(void)
{
    if (rtos_is_oneshot_timer_running(&uar_nic_sleep_timer))
    {
        rtos_stop_oneshot_timer(&uar_nic_sleep_timer);
    }
    rtos_start_oneshot_timer(&uar_nic_sleep_timer);
}

/**
 * @brief Handle sleep message in NIC mode
 * @param msg Pointer to bus message
 *        This function prepares the system for sleep by setting the MRDY signal
 *        and turning off the modem power module.
 */
void bk_modem_uart_nic_sleep_handle(BUS_MSG_T *msg)
{
    bk_modem_env.entered_sleep = true;

    /* Set MASTER_MRDY_GPIO high, indicating ready to sleep */
    bk_gpio_set_output_high(MASTER_MRDY_GPIO);

    for (int i = 0; i < 5; i ++)
    {
        if (bk_gpio_get_input(MASTER_SRDY_GPIO))
            break;

        rtos_delay_milliseconds(2);
    }

    /* Turn off Modem power module */
    bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_BK_MODEM, 0x1, 0x0);
}

/**
 * @brief Send message to UART receive queue
 * @param type Message type
 * @param arg Message argument
 * @param len Data length
 * @param param Data pointer
 * @return Operation result, BK_OK indicates success
 *        This function creates and sends a message to the UART receive queue
 *        for further processing by the receive thread.
 */
static bk_err_t bk_modem_uart_rx_send_msg(int type, uint32_t arg, uint32_t len, void *param)
{
    BUS_MSG_T msg;
    int ret;

    if (bk_modem_uart_rx_queue == NULL)
    {
        BK_MODEM_LOGI("%s: queue is null!", __func__);
        return BK_FAIL;
    }

    /* Reset sleep timer for all messages except sleep message */
    if (type != MSG_MODEM_UART_NIC_SLEEP)
    {
        bk_modem_uart_sleep_check();
    }
    msg.type = type;
    msg.arg = (uint32_t)arg;
    msg.len = len;
    msg.sema = NULL;
    msg.param = (void *)param;

    /* Push message to receive queue */
    ret = rtos_push_to_queue(&bk_modem_uart_rx_queue, &msg, BEKEN_NO_WAIT);
    if (kNoErr != ret)
    {
        //BK_MODEM_LOGI("%s: push rx queue fail!", __func__);
    }

    return ret;
}

/**
 * @brief UART receive interrupt callback function
 * @param id UART port ID
 * @param param User parameter
 *        This function is called when UART data is received, and it triggers
 *        the appropriate message handler based on the communication protocol mode.
 */
static void bk_modem_uart_rx_isr_callback(uart_id_t id, void *param)
{
    /* Send different messages according to communication protocol type */
    if (bk_modem_env.comm_proto == UART_NIC_MODE)
    {
        bk_modem_uart_rx_send_msg(MSG_MODEM_UART_NIC_RX,0,0,0);
    }
    else
    {
        bk_modem_uart_rx_send_msg(MSG_MODEM_UART_RX,0,0,0);
    }
}

/**
 * @brief Send data in NIC mode
 * @param msg Pointer to bus message containing data to send
 *        This function handles data transmission in NIC mode, including
 *        flow control, frame header preparation, and power management.
 */
static void bk_modem_uart_nic_send(BUS_MSG_T *msg)
{
    UART_NIC_HD_T *header = (UART_NIC_HD_T *)msg->param;

    /* Set MASTER_MRDY_GPIO low, indicating data to send */
    bk_gpio_set_output_low(MASTER_MRDY_GPIO);

    /* Wait for slave device to be ready to receive data */
    if (bk_gpio_get_input(MASTER_SRDY_GPIO))
    {
        for (uint8_t i=0; i<3; i++)
        {
            rtos_delay_milliseconds(10);
            if (!bk_gpio_get_input(MASTER_SRDY_GPIO))
                break;
        }
        BK_MODEM_LOGI("bk_modem_uart_nic_send wait modem %d\r\n", bk_gpio_get_input(MASTER_SRDY_GPIO));
    }
    
    /* Fill frame header information */
    header->field.data_len = msg->len;
    header->field.flow_control = 0;
    header->field.type = msg->arg;
    header->field.seq_no = s_uart_tx_no++;
    header->field.is_tx_end = false;
    header->field.checksum = bk_modem_uart_nic_csum(header);
    
    /* Arm pulse flag before the write; any SRDY falling edge after this point
     * (including the slave's ~50us ACK pulse) will set the flag via ISR. */
    s_srdy_pulse_flag = 0;

    /* Send data via UART (including header) */
    bk_uart_write_bytes(BK_MODEM_UART_ID, header, msg->len+UART_NIC_HD_SIZE);

    /* Wait for the ~50us SRDY low pulse from slave (captured by falling-edge ISR).
     * A 50us pulse is too short to be caught reliably by GPIO polling, so we rely
     * on the ISR-set flag and re-check every 2ms. */
    for (uint8_t i = 0; i < 5; i++)
    {
        if (s_srdy_pulse_flag)
            break;
        rtos_delay_milliseconds(2);
    }
    //BK_MODEM_LOGI("bk_modem_uart_nic_send len %d, pulse=%d\r\n", msg->len, s_srdy_pulse_flag);

    /* Turn on Modem power module */
    bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_BK_MODEM, 0x0, 0x0);
}

/**
 * @brief Receive data in NIC mode
 * @param msg Pointer to bus message
 *        This function handles data reception in NIC mode, including
 *        checksum verification, data parsing, and handshake signaling.
 */
static void bk_modem_uart_nic_rx(BUS_MSG_T *msg)
{
    uint32_t length;
    /* Clear receive buffer */
    os_memset(g_uart_rx_buff, 0, UART_NIC_RX_MTU);

    /* Read data from UART */
    length = bk_uart_read_bytes(BK_MODEM_UART_ID, g_uart_rx_buff, UART_NIC_RX_MTU, 0);

    if(length)
    {
        UART_NIC_HD_T *header = (UART_NIC_HD_T *)g_uart_rx_buff;
        /* Verify checksum */
        if (bk_modem_uart_nic_csum(header) == header->field.checksum)
        {
            uint32_t rx_data_len = header->field.data_len;
            if ((rx_data_len + UART_NIC_HD_SIZE) <= length)
            {
                /* Different processing according to data type */
                if (header->field.type == AT_CMD_MODE)
                {
                    if (length < UART_NIC_RX_MTU)
                    {
                        g_uart_rx_buff[length] = 0;
                        //BK_MODEM_LOGI("bk_modem_uart_nic_rx at cmd, len %d, rx_data_len %d\r\n",length, rx_data_len);
                        bk_modem_dte_recv_data_uart(rx_data_len, g_uart_rx_buff+UART_NIC_HD_SIZE, AT_CMD_MODE);
                    }
                }
                else if (header->field.type == NIC_DATA_MODE)
                {
                    //BK_MODEM_LOGI("bk_modem_uart_nic_rx data, len %d, rx_data_len %d\r\n",length, rx_data_len);
                    bk_modem_dte_recv_data_uart(rx_data_len, g_uart_rx_buff+UART_NIC_HD_SIZE, NIC_DATA_MODE);
                }
            }
        }

        /* Send handshake signal to slave device indicating data has been received */
        if (bk_gpio_get_output(MASTER_MRDY_GPIO))
        {
            bk_gpio_set_output_low(MASTER_MRDY_GPIO);
            bk_delay_us(50);
        }

        bk_gpio_set_output_high(MASTER_MRDY_GPIO);
        bk_delay_us(50);
        bk_gpio_set_output_low(MASTER_MRDY_GPIO);

        /* Turn on Modem power module */
        bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_BK_MODEM, 0x0, 0x0);
    }
}

/**
 * @brief UART receive thread main function
 * @param args Thread arguments
 *        This function is the main loop for the UART receive thread,
 *        processing messages from the receive queue.
 */
static void bk_modem_uart_rx_thread_main(void *args)
{
    int ret;
    BUS_MSG_T msg;

    /* Loop to process messages in receive queue */
    while (1) 
    {
        ret = rtos_pop_from_queue(&bk_modem_uart_rx_queue, &msg, BEKEN_WAIT_FOREVER);

        if (ret ==  kNoErr)
        {
            //BK_MODEM_LOGI("%s: msg.type %d\r\n", __func__, msg.type);
            switch (msg.type)
            {
                case MSG_MODEM_UART_RX:
                {
                    //bk_modem_uart_rx(&msg);
                    break;
                }

                case MSG_MODEM_UART_NIC_RX:
                {
                    bk_modem_uart_nic_rx(&msg);
                    break;
                }

                case MSG_MODEM_UART_NIC_SLEEP:
                {
                    bk_modem_uart_nic_sleep_handle(&msg);
                    break;
                }
                
                default:
                {
                    BK_MODEM_LOGI("%s: error!", __func__);
                    break;
                }
            }
        }

    }
}

/**
 * @brief UART transmit thread main function
 * @param args Thread arguments
 *        This function is the main loop for the UART transmit thread,
 *        processing messages from the transmit queue.
 */
static void bk_modem_uart_tx_thread_main(void *args)
{
    int ret;
    BUS_MSG_T msg;

    /* Loop to process messages in transmit queue */
    while (1) 
    {
        ret = rtos_pop_from_queue(&bk_modem_uart_tx_queue, &msg, BEKEN_WAIT_FOREVER);

        if (ret ==  kNoErr)
        {
            switch (msg.type)
            {
                case MSG_MODEM_UART_TX:
                {
                    //bk_modem_uart_send(&msg);
                    break;
                }

                case MSG_MODEM_UART_NIC_TX:
                {
                    bk_modem_uart_nic_send(&msg);
                    break;
                }
                
                default:
                {
                    BK_MODEM_LOGI("%s: error!", __func__);
                    break;
                }
            }
        }

        /* Release message parameter memory */
        if (msg.param)
            os_free(msg.param);
    }
}

/**
 * @brief Send message to UART transmit queue
 * @param type Message type
 * @param arg Message argument
 * @param len Data length
 * @param param Data pointer
 * @return Operation result, BK_OK indicates success
 *        This function creates and sends a message to the UART transmit queue
 *        for further processing by the transmit thread.
 */
bk_err_t bk_modem_uart_tx_send_msg(int type, uint32_t arg, uint32_t len, void *param)
{
    BUS_MSG_T msg;
    bk_err_t ret = BK_OK;

    if (bk_modem_uart_tx_queue == NULL)
    {
        BK_MODEM_LOGI("%s: queue is null!", __func__);
        return BK_FAIL;
    }

    msg.type = type;
    msg.arg = (uint32_t)arg;
    msg.len = len;
    
    /* Allocate different size buffers according to communication protocol type */
    if (len)
    {
#if CONFIG_LWIP_PPP_SUPPORT
        if (bk_modem_env.comm_proto == PPP_MODE)
        {
            msg.param = os_malloc(len);
            if (msg.param)
            {
                os_memset((uint8_t *)msg.param, 0, len);
                os_memcpy((uint8_t *)msg.param, (uint8_t *)param, len);            
            }
            else
            {
                BK_MODEM_LOGE("bk_modem_uart_tx_send_msg fail1!\r\n");
                return BK_FAIL;
            }
        }
        else
#endif
        if (bk_modem_env.comm_proto == UART_NIC_MODE)
        {
            bk_modem_uart_sleep_check();
            msg.param = os_malloc(len+UART_NIC_HD_SIZE);
            if (msg.param)
            {
                os_memset((uint8_t *)msg.param, 0, len+UART_NIC_HD_SIZE);
                os_memcpy((uint8_t *)msg.param+UART_NIC_HD_SIZE, (uint8_t *)param, len);
            }
            else
            {
                BK_MODEM_LOGE("bk_modem_uart_tx_send_msg fail2!\r\n");
                return BK_FAIL;
            }
        }
    }
    else
    {
        if (type == MSG_MODEM_UART_NIC_TX)
        {
            if (bk_modem_env.entered_sleep)
            {
                bk_modem_uart_sleep_check();
                bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_BK_MODEM, 0x0, 0x0);
                bk_uart_enable_rx_interrupt(BK_MODEM_UART_ID);

                bk_modem_env.entered_sleep = false;
            }

            //BK_MODEM_LOGI("bk_modem_uart_tx_send_msg wakeup!\r\n");
            return BK_OK;
        }
        else
        {
            BK_MODEM_LOGI("bk_modem_uart_tx_send_msg fail3!\r\n");
            return BK_FAIL;            
        }
    }
    msg.sema = NULL;

    /* Push message to transmit queue */
    ret = rtos_push_to_queue(&bk_modem_uart_tx_queue, &msg, BEKEN_NO_WAIT);
    if (kNoErr != ret)
    {
        if (len)
        {
            os_free(msg.param);
        }
    }

    return ret;
}

/**
 * @brief SRDY pin interrupt callback function
 * @param gpio_id GPIO pin ID
 * @note This function is called when MASTER_SRDY_GPIO pin triggers falling edge interrupt
 *       It sends a wakeup message to the transmit queue to handle incoming data from the modem.
 */
void bk_modem_uart_sdry_int_cb(gpio_id_t gpio_id)
{
    /* Verify GPIO ID is correct */
    if (gpio_id != MASTER_SRDY_GPIO)
    {
        BK_MODEM_LOGI("[%s][%d] gpio_id:%d\r\n", __FUNCTION__, __LINE__, gpio_id);
    }

    bk_gpio_clear_interrupt(MASTER_SRDY_GPIO);

    /* Latch the falling edge so the TX path can detect the slave's ~50us ACK pulse. */
    s_srdy_pulse_flag = 1;

    /* Send wakeup message to transmit queue */
    bk_modem_uart_tx_send_msg(MSG_MODEM_UART_NIC_TX,0,0,0);
}

/**
 * @brief Sleep timer callback function in NIC mode
 * @param arg1 User parameter 1
 * @param arg2 User parameter 2
 *        This function is called when the sleep timer expires, 
 *        sending a sleep message to prepare the system for sleep.
 */
static void bk_modem_uart_nic_timer_cb(void *arg1, void *arg2)
{
    /* Send sleep message to receive queue */
    bk_modem_uart_rx_send_msg(MSG_MODEM_UART_NIC_SLEEP,0,0,0);
}

/**
 * @brief Initialize Modem UART interface
 * @param baud_rate Baud rate
 * @return Operation result, BK_OK indicates success
 *        This function initializes the UART interface for modem communication,
 *        including UART configuration, queue creation, thread creation, and GPIO setup.
 */
bk_err_t bk_modem_uart_init(uint32_t baud_rate)
{
    bk_err_t ret = BK_FAIL;
    uint8_t temp_flag = 0xff; /* Used to identify initialization failure reason */

    s_uart_tx_no = 0;
    
    /* UART configuration structure */
    uart_config_t config = 
    {
        .baud_rate = BK_MODEM_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_NONE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_FLOWCTRL_DISABLE,
        .src_clk = UART_SCLK_XTAL_26M
    };

    /* Set baud rate and select clock source according to baud rate */
    config.baud_rate = baud_rate;
    if ((baud_rate == BK_MODEM_UART_3M_BAUD) || (baud_rate == BK_MODEM_UART_6M_BAUD))
    {
        config.src_clk = UART_SCLK_APLL;
    }

    #if CONFIG_UART_RX_DMA
    /* Disabled: rewind assumes "one idle IRQ == one whole frame drained in a
     * single read". Use plain streaming RX DMA + kfifo instead. */
    config.rx_dma_rewind_when_fifo_empty = 0;
    config.rx_dma_en = UART_DMA_ENABLE;
    #endif
    
    #if CONFIG_UART_TX_DMA
    config.tx_dma_en = UART_DMA_ENABLE;
    #endif
    BK_MODEM_LOGI("[%s][%d] set baud:%d, uart id %d\r\n", __FUNCTION__, __LINE__, config.baud_rate,BK_MODEM_UART_ID);
    /* Initialize UART */
    ret = bk_uart_init(BK_MODEM_UART_ID, &config);
    if(BK_OK != ret)
    {
        temp_flag = 0;
        goto fail;
    }

    /* Register UART receive interrupt callback function and enable interrupt */
    bk_uart_register_rx_isr(BK_MODEM_UART_ID, bk_modem_uart_rx_isr_callback, NULL);
    bk_uart_enable_rx_interrupt(BK_MODEM_UART_ID);

    /* Initialize receive and transmit queues */
    ret = rtos_init_queue(&bk_modem_uart_rx_queue, "bk_modem_uart_rx_queue", sizeof(BUS_MSG_T), 64);
    if(BK_OK != ret)
    {
        temp_flag = 1;
        goto fail;
    }

    ret = rtos_init_queue(&bk_modem_uart_tx_queue, "bk_modem_uart_tx_queue", sizeof(BUS_MSG_T), 64);
    if(BK_OK != ret)
    {
        temp_flag = 2;
        goto fail;
    }

    /* Create receive and transmit threads */
    ret = rtos_create_thread(&bk_modem_uart_rx_thread, 3, "bk_modem_uart_rx", bk_modem_uart_rx_thread_main, 3072, (beken_thread_arg_t)0);
    if(BK_OK != ret)
    {
        temp_flag = 4;
        goto fail;
    }

    ret = rtos_create_thread(&bk_modem_uart_tx_thread, 3, "bk_modem_uart_tx", bk_modem_uart_tx_thread_main, 3072, (beken_thread_arg_t)0);
    if(BK_OK != ret)
    {
        temp_flag = 5;
        goto fail;
    }

    /* Configure NIC mode related functions */
    if (bk_modem_env.comm_proto == UART_NIC_MODE)
    {
        /* Initialize sleep timer */
        rtos_init_oneshot_timer(&uar_nic_sleep_timer,UART_NIC_SLEEP_TIMER,bk_modem_uart_nic_timer_cb,(void *)0,(void *)0);

        BK_MODEM_LOGI("[%s][%d] NIC_MASTER_READY_GPIO:%d, NIC_SLAVE_READY_GPIO:%d\r\n", 
                                                            __FUNCTION__, __LINE__, 
                                                            CONFIG_UART_NIC_MASTER_READY_GPIO,
                                                            CONFIG_UART_NIC_SLAVE_READY_GPIO);
        
        /* Configure MASTER_MRDY_GPIO pin (host ready signal) */
        gpio_config_t cfg;
        gpio_dev_unmap(MASTER_MRDY_GPIO);
        cfg.func_mode = GPIO_SECOND_FUNC_DISABLE;
        cfg.io_mode = GPIO_OUTPUT_ENABLE;
        cfg.pull_mode = GPIO_PULL_DOWN_EN;
        bk_gpio_set_config(MASTER_MRDY_GPIO, &cfg);
		cfg.pull_mode = GPIO_PULL_UP_EN;
        #if CONFIG_GPIO_DYNAMIC_KPSTAT_SUPPORT
        bk_gpio_register_lowpower_keep_status(MASTER_MRDY_GPIO, &cfg);
        #endif
        bk_gpio_set_output_low(MASTER_MRDY_GPIO);

        /* Configure MASTER_SRDY_GPIO pin (slave ready signal) */
        cfg.func_mode = GPIO_SECOND_FUNC_DISABLE;
        cfg.io_mode = GPIO_INPUT_ENABLE;
        cfg.pull_mode = GPIO_PULL_UP_EN;
        bk_gpio_set_config(MASTER_SRDY_GPIO, &cfg);
        bk_gpio_set_interrupt_type(MASTER_SRDY_GPIO, GPIO_INT_TYPE_FALLING_EDGE);
        /* Register SRDY pin interrupt handler */
        bk_gpio_register_isr(MASTER_SRDY_GPIO, bk_modem_uart_sdry_int_cb);
        #if CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT
        /* Register SRDY as an AP wake source through the AP GPIO/PM client path. */
        bk_gpio_set_wakeup(MASTER_SRDY_GPIO, GPIO_INT_TYPE_FALLING_EDGE, true);
        #endif
        /* Enable interrupt */
        bk_gpio_enable_interrupt(MASTER_SRDY_GPIO);
        
        /* Allocate receive buffer in NIC mode */
        g_uart_rx_buff = os_malloc(UART_NIC_RX_MTU);
        if(!g_uart_rx_buff)
        {
            temp_flag = 6;
            goto fail;
        }
        
    }
    else
    {
        /* Allocate receive buffer in normal mode */
        g_uart_rx_buff = os_malloc(BK_MODEM_UART_READ_BUFF_SIZE);
        if(!g_uart_rx_buff)
        {
            temp_flag = 3;
            goto fail;
        }
    }
    return BK_OK;

fail:

    BK_MODEM_LOGE("bk_modem_uart_init fail, reason %d\r\n", temp_flag);
    bk_modem_uart_deinit();

    return BK_FAIL;
}

/**
 * @brief Send data to Modem
 * @param data_length Data length
 * @param data Data pointer
 * @param uart_trx_mode Transmission mode (AT command mode or data mode)
 */
void bk_modem_uart_data_send(uint32_t data_length, uint8_t *data, enum bk_modem_uart_trx_mode_e uart_trx_mode)
{
#if CONFIG_LWIP_PPP_SUPPORT
    if (bk_modem_env.comm_proto == PPP_MODE)
    {
        bk_modem_uart_tx_send_msg(MSG_MODEM_UART_TX,uart_trx_mode,data_length,data);
    }
    else
#endif
    {
        if ((bk_modem_ec_hs > 0) || (uart_trx_mode == AT_CMD_MODE))
        {
            bk_modem_uart_tx_send_msg(MSG_MODEM_UART_NIC_TX,uart_trx_mode,data_length,data);
        }
        else
        {
            BK_MODEM_LOGE("bk_modem_uart_tx fail\r\n");
        }
    }
}

/**
 * @brief Deinitialize Modem UART interface
 * @return Operation result, BK_OK indicates success
 *        This function cleans up all resources allocated during initialization,
 *        including threads, queues, buffers, and UART configuration.
 */
bk_err_t bk_modem_uart_deinit(void)
{
    bk_err_t ret = BK_OK;
    
    BK_MODEM_LOGE("bk_modem_uart_deinit\r\n");
    
    /* Disable UART receive interrupt */
    bk_uart_disable_rx_interrupt(BK_MODEM_UART_ID);
    
    /* Unregister UART receive interrupt callback function */
    bk_uart_register_rx_isr(BK_MODEM_UART_ID, NULL, NULL);
    
    /* Delete created threads */
    if (bk_modem_uart_rx_thread != NULL) {
        rtos_delete_thread(&bk_modem_uart_rx_thread);
        bk_modem_uart_rx_thread = NULL;
    }
    
    if (bk_modem_uart_tx_thread != NULL) {
        rtos_delete_thread(&bk_modem_uart_tx_thread);
        bk_modem_uart_tx_thread = NULL;
    }
    
    /* Release receive buffer */
    if (g_uart_rx_buff != NULL) {
        os_free(g_uart_rx_buff);
        g_uart_rx_buff = NULL;
    }
    
    /* Deinitialize message queues */
    if (bk_modem_uart_rx_queue != NULL) {
        rtos_deinit_queue(&bk_modem_uart_rx_queue);
        bk_modem_uart_rx_queue = NULL;
    }
    if (bk_modem_uart_tx_queue != NULL) {
        rtos_deinit_queue(&bk_modem_uart_tx_queue);
        bk_modem_uart_tx_queue = NULL;
    }
    
    /* Deinitialize UART */
    ret = bk_uart_deinit(BK_MODEM_UART_ID);
    
    BK_MODEM_LOGI("[%s][%d] modem uart deinit success\r\n", __FUNCTION__, __LINE__);

    return ret;
}

/**
 * @brief Set UART baud rate at runtime
 * @param baud_rate Target baud rate (e.g. BK_MODEM_UART_2M_BAUD, BK_MODEM_UART_5M2_BAUD)
 * @return BK_OK on success
 */
bk_err_t bk_modem_uart_set_baud_rate(uint32_t baud_rate)
{
    bk_err_t ret;

    ret = bk_uart_set_baud_rate(BK_MODEM_UART_ID, baud_rate);
    if (ret == BK_OK)
    {
        s_modem_uart_baud_rate = baud_rate;
        BK_MODEM_LOGI("[%s][%d] modem uart baud rate set to %d\r\n", __FUNCTION__, __LINE__, baud_rate);
    }
    return ret;
}

/**
 * @brief Get current UART baud rate
 * @return Current baud rate
 */
uint32_t bk_modem_uart_get_baud_rate(void)
{
    return s_modem_uart_baud_rate;
}

//#endif

