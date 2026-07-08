#include"port.h"
#include"DVC1124.h"
#include"n32wb43x_pwr.h"
#include"User_Can_Config.h"
#include"app_io.h"
#include"Soc.h"
#include"app_proto_cmn.h"

bool shallow_sleeping = false;
bool deep_sleeping = false;
bool g_bSleepActive = false;
static bool deepsleep_started = false;
volatile bool g_bWakeCan  = false;
volatile bool g_bWakeRs485 = false;
volatile bool g_bWakeAfe  = false;
volatile bool g_bWakeUart = false;
/**
 * @brief  Configures JTAG as GPIO.
 */
void JTAGPortDisableInit(void)
{
    GPIO_InitType GPIO_InitStructure;

    GPIO_InitStruct(&GPIO_InitStructure);
    
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOA | RCC_APB2_PERIPH_GPIOB, ENABLE);
    
    // /* Configure PA.13 (JTMS/SWDIO) and PA.15 (JTDI) as gpio */
    // GPIO_InitStructure.Pin        = GPIO_PIN_13 | GPIO_PIN_15;
    // GPIO_InitStructure.GPIO_Pull = GPIO_No_Pull;
    // GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    // GPIO_InitPeripheral(GPIOA, &GPIO_InitStructure);

    /* Configure PB.04 (JTRST) as gpio */
    GPIO_InitStructure.Pin = GPIO_PIN_4;
    GPIO_InitPeripheral(GPIOB, &GPIO_InitStructure);

    // /* Configure  PA.14 (JTCK/SWCLK) as gpio */
    // GPIO_InitStructure.Pin        = GPIO_PIN_14;
    // GPIO_InitPeripheral(GPIOA, &GPIO_InitStructure);

    /* Configure PB.03 (JTDO) as gpio */
    GPIO_InitStructure.Pin        = GPIO_PIN_3;
    GPIO_InitPeripheral(GPIOB, &GPIO_InitStructure);

}

/**
 * @brief  Configures GPIO.
 * @param GPIOx x can be A to G to select the GPIO port.
 * @param Pin This parameter can be GPIO_PIN_0~GPIO_PIN_15.
 */
void PortInit(GPIO_Module* GPIOx, uint16_t Pin, GPIO_ModeType GPIO_Mode)
{
    GPIO_InitType GPIO_InitStructure;

    /* Check the parameters */
    assert_param(IS_GPIO_ALL_PERIPH(GPIOx));

    /* Enable the GPIO Clock */
    if (GPIOx == GPIOA)
    {
        RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOA, ENABLE);
    }
    else if (GPIOx == GPIOB)
    {
        RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOB, ENABLE);
    }
    else if (GPIOx == GPIOC)
    {
        RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOC, ENABLE);
    }
    else
    {
        if (GPIOx == GPIOD)
        {
            RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOD, ENABLE);
        }
    }

    /* Configure the GPIO pin */
    if (Pin <= GPIO_PIN_ALL)
    {
        GPIO_InitStruct(&GPIO_InitStructure);
        GPIO_InitStructure.Pin        = Pin;
		GPIO_InitStructure.GPIO_Current = GPIO_DC_4mA;
        GPIO_InitStructure.GPIO_Pull    = GPIO_No_Pull;
        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode;
        GPIO_InitPeripheral(GPIOx, &GPIO_InitStructure);
    }


}

void LedBlink(GPIO_Module* GPIOx, uint16_t Pin)
{
    GPIOx->POD ^= Pin;
}

void LedInit(GPIO_Module* GPIOx, uint16_t Pin)
{
    PortInit(GPIOx, Pin, GPIO_Mode_Out_PP);
}

void Port_Config(void)
{

  PortInit(DCDCEN_PORT, DCDCEN_PIN, GPIO_Mode_Out_PP);
  GPIO_SetBits(DCDCEN_PORT, DCDCEN_PIN);
  PortInit(M5V_CTRL_PORT, M5V_CTRL_PIN, GPIO_Mode_Out_PP);
  GPIO_SetBits(M5V_CTRL_PORT, M5V_CTRL_PIN);
  PortInit(M3V3_CTRL_PORT, M3V3_CTRL_PIN, GPIO_Mode_Out_PP);
  GPIO_ResetBits(M3V3_CTRL_PORT, M3V3_CTRL_PIN);
  PortInit(LED_PORT, LED_PIN, GPIO_Mode_Out_PP);
  PortInit(AFE_PORT, AFE_MCUIO_PIN, GPIO_Mode_Out_PP);
  PortInit(GPIOB, YDG, GPIO_Mode_Out_PP);  //hardware watchdog
  PortInit(HEAT_PORT, HEAT_HTCTRL_PIN ,GPIO_Mode_Out_PP);  //heating
	PortInit(FSEN_PORT, HEAT_FSEN_PIN  ,GPIO_Mode_Out_PP);  //heating
  PortInit(HTDET_PORT, HTDET_PIN, GPIO_Mode_Input);
  PortInit(WAKE_CHG_PORT, WAKE_CHG_PIN, GPIO_Mode_Input);
  PortInit(WAKE_LOAD_PORT, WAKE_LOAD_PIN, GPIO_Mode_Input);
  PortInit(WAKE_KEY_PORT, WAKE_KEY_PIN, GPIO_Mode_Input);  // Key button
	PortInit(WAKE_AFE_PORT, WAKE_AFE_PIN, GPIO_Mode_Input);  
	PortInit(WAKE_RS485_PORT, WAKE_RS485_PIN, GPIO_Mode_Input);
	PortInit(WAKE_CAN_PORT, WAKE_CAN_PIN, GPIO_Mode_Input);    
  GPIO_SetBits(AFE_PORT, AFE_MCUIO_PIN);


}

void System_operation(void)
{
    static uint16_t num = 0;
    static uint8_t YDG_num = 0;
    if (num++ >= 30) //300ms
    {
        num = 0;
        LedBlink(LED_PORT, LED_PIN);
      
   
        if (++YDG_num > 5) //1500ms 硬狗
        {
        //    IWDG_ReloadKey(); //软狗
        
           YDG_num = 0;
           GPIO_SetBits(GPIOB, YDG);
            Delay_us(300);
           GPIO_ResetBits(GPIOB, YDG);
        }
    }
}
static bool Sleep_WakeEventCheck(void)
{
    /* KEY (PB0) polled */
    if (GPIO_ReadInputDataBit(WAKE_KEY_PORT, WAKE_KEY_PIN) == RESET) {
        return true;
    }
    /* AFE (PC4) EXTI4 falling */
    if (g_bWakeAfe) {
        g_bWakeAfe = false;
        return true;
    }   
    /* RS485 wake (PA8) EXTI8 rising */
    if (g_bWakeRs485) {
        g_bWakeRs485 = false;
        return true;
    }
    /* CAN wake (PB7) EXTI7 rising */
    if (g_bWakeCan) {
        g_bWakeCan = false;
        return true;
    }
    /* UART5 debug RX - UART_IRQHandler sets g_bWakeUart */
    if (g_bWakeUart) {
        g_bWakeUart = false;
        return true;
    }
    /* BLE UART4 RX (PB15) LOW = start bit */
    if (GPIO_ReadInputDataBit(GPIOB, GPIO_PIN_15) == RESET) {
        return true;
    }
    /* Charger detect PA6 - active LOW = charger plugged in */
    if (GPIO_ReadInputDataBit(WAKE_CHG_PORT, WAKE_CHG_PIN) == RESET) {
        return true;
    }
    /* Load detect PA7 - active LOW = load connected */
    if (GPIO_ReadInputDataBit(WAKE_LOAD_PORT, WAKE_LOAD_PIN) == RESET) {
        return true;
    }
		 /* BLE (PC7) polled */
    if (GPIO_ReadInputDataBit(WAKE_BLE_PORT, WAKE_BLE_PIN) != RESET) {
        return true;
    }
    return false;
}

void Sleep_WakeupConfig(void)
{
    EXTI_InitType EXTI_InitStructure;
    NVIC_InitType NVIC_InitStructure;

    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_AFIO | RCC_APB2_PERIPH_GPIOA
                          | RCC_APB2_PERIPH_GPIOB | RCC_APB2_PERIPH_GPIOC
                          | RCC_APB2_PERIPH_GPIOD, ENABLE);

    /* Clear any stale pending bits before re-mapping EXTI lines */
    EXTI_ClrITPendBit(WAKE_AFE_EXTI_LINE);
    EXTI_ClrITPendBit(WAKE_CAN_EXTI_LINE);
    EXTI_ClrITPendBit(WAKE_RS485_EXTI_LINE);

    /* AFE (PC4) -> EXTI4 falling edge */
    GPIO_ConfigEXTILine(WAKE_AFE_PORT_SRC, WAKE_AFE_PIN_SRC);
    EXTI_InitStructure.EXTI_Line    = WAKE_AFE_EXTI_LINE;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitPeripheral(&EXTI_InitStructure);

    /* CAN (PB7) -> EXTI7 rising edge */
    GPIO_ConfigEXTILine(WAKE_CAN_PORT_SRC, WAKE_CAN_PIN_SRC);
    EXTI_InitStructure.EXTI_Line    = WAKE_CAN_EXTI_LINE;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitPeripheral(&EXTI_InitStructure);
	
    /* RS485 (PA8) -> EXTI8 rising edge */
    GPIO_ConfigEXTILine(WAKE_RS485_PORT_SRC, WAKE_RS485_PIN_SRC);
    EXTI_InitStructure.EXTI_Line    = WAKE_RS485_EXTI_LINE;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitPeripheral(&EXTI_InitStructure);
		
    /* Enable NVIC for wake-up EXTI interrupts */
    NVIC_InitStructure.NVIC_IRQChannel                   = EXTI4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel                   = EXTI9_5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void Sleep_PeripheralDisable(void)
{
    /* Disable USART3 (RS485) */
    USART_Enable(USART485, DISABLE);
    RCC_EnableAPB1PeriphClk(USART485_CLK, DISABLE);

    /* NOTE: UART5 (debug/console) stays enabled for RX interrupt wake-up */

    /* Disable UART4 (BLE HCI) */
    USART_Enable(UART4, DISABLE);
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_UART4, DISABLE);

    /* Disable CAN */
    CAN_DeInit(CAN);
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_CAN, DISABLE);

    /* Disable ADC */
    ADC_Enable(ADC, DISABLE);
    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPH_ADC, DISABLE);

    /* Disable TIM1 */
    TIM_ConfigInt(TIM1, TIM_INT_UPDATE, DISABLE);
    TIM_Enable(TIM1, DISABLE);
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_TIM1, DISABLE);

    /* Disable TIM2 */
    TIM_Enable(TIM2, DISABLE);
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_TIM2, DISABLE);

    DVC1124_Sleep();

    /* Power off 3.3V and 5V rails */
    GPIO_SetBits(M3V3_CTRL_PORT, M3V3_CTRL_PIN);
    GPIO_ResetBits(M5V_CTRL_PORT, M5V_CTRL_PIN);
		Delay_ms(1);
}

void Sleep_PeripheralRestore(void)
{
    /* Clear all wake-up EXTI pending bits from the sleep wake event */
    EXTI_ClrITPendBit(WAKE_CAN_EXTI_LINE);
    EXTI_ClrITPendBit(WAKE_RS485_EXTI_LINE);

    /* Power up sequence: 5V first, then 3.3V */
    GPIO_SetBits(M5V_CTRL_PORT, M5V_CTRL_PIN);
    Delay_ms(5);
    GPIO_ResetBits(M3V3_CTRL_PORT, M3V3_CTRL_PIN);
    Delay_ms(5);

    /* Re-enable TIM1 */
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_TIM1, ENABLE);
    TIM_Enable(TIM1, ENABLE);
    //TIM_ConfigInt(TIM1, TIM_INT_UPDATE, ENABLE);

    /* Re-enable TIM2 */
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_TIM2, ENABLE);
    TIM_Enable(TIM2, ENABLE);

    /* Re-enable ADC */
    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPH_ADC, ENABLE);
    ADC_Enable(ADC, ENABLE);

    /* Re-enable CAN */
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_CAN, ENABLE);
    CAN_Config();

    /* Re-enable UART4 (BLE) */
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_UART4, ENABLE);
    USART_Enable(UART4, ENABLE);

    /* NOTE: UART5 was kept enabled during sleep for wake-up */

    /* Re-enable USART3 (RS485) */
    RCC_EnableAPB1PeriphClk(USART485_CLK, ENABLE);
    USART_Enable(USART485, ENABLE);

    /* Re-init AFE/BLE GPIO (restores PC7 EXTI7 for AFE interrupt + BLE IRQ) */
    app_io_init();

    /* Re-init AFE */
    AFE_Iicinit();
    AFEParam_Config();

    /* Recalibrate SOC from OCV after wake-up (coulomb counter was stopped) */
    CurDriftCalib_Init();
    SOC_WakeupCalibrate();

    g_bSleepActive = false;
}

void Shallow_Sleep(void)
{
    static uint64_t Shallow_time = 0;

		 /* 电池欠压时只进入Deep_Sleep，不进入Shallow_Sleep */
    if (deepsleep_started)
    {
        /* 如果当前正在浅休眠，先退出（先唤醒AFE消除INT，再改状态防止ISR复位） */
        if (shallow_sleeping)
        {
            /* Real wake event: restore peripherals */
            Sleep_PeripheralRestore();
            shallow_sleeping = false;
        }
        return;
    }
    if (!bDSGING && !bCHGING && app_cmn_env.conn_stat != AT_STAT_CONN && GetTick_ms() - g_u64LastCommMs > 6000)
    {
        if (!shallow_sleeping)
        {
            Shallow_time = GetTick_ms();
            shallow_sleeping = true;
            g_bSleepActive = false;
        }
        else if (!g_bSleepActive)
        {
            if (GetTick_ms() >= Shallow_time + SHALLOW_SLEEP_TIMEOUT)
            {
                g_bSleepActive = true;
                byEEMemoryWrite(EE_RUN_ADDR, (uint8_t *)&g_tBatPackCtrl.tEERun, EE_RUN_LEN);

                Sleep_WakeupConfig();
                Sleep_PeripheralDisable();

                /* Ensure UART5 RX interrupt is enabled for sleep wake-up */
                USART_ConfigInt(UART, USART_INT_RXDNE, ENABLE);
                g_bWakeUart = false;
								g_bWakeCan  = false;
								g_bWakeRs485 = false;
								g_bWakeAfe  = false;
								
								 GPIO_SetBits(LED_PORT, LED_PIN);
                /* Sleep loop: WFI sleeps until any NVIC interrupt.
                 * SysTick fires every 1ms but we check for real wake events
                 * and go right back to sleep if only SysTick woke us. */
                while (g_bSleepActive)
                {
                    static uint16_t s_wSleepWdtCnt = 0;

                    __WFI();

                    if (Sleep_WakeEventCheck())
                    {
                        break;
                    }

                    /* Feed hardware watchdog (YDG) during sleep.
                     * SysTick wakes us every 1ms; feed every ~1500ms. */
                    if (++s_wSleepWdtCnt >= 1500)
                    {
                        s_wSleepWdtCnt = 0;
                        GPIO_SetBits(GPIOB, YDG);
                        Delay_us(300);
                        GPIO_ResetBits(GPIOB, YDG);
                    }
                }

                /* Real wake event: restore peripherals */
                Sleep_PeripheralRestore();
                shallow_sleeping = false;
            }
        }
    }
    else
    {
        shallow_sleeping = false;
        g_bSleepActive = false;
    }
}


void Deep_Sleep(void)
{
   static uint64_t sleep_time = 0;
  
    if ((!bDSGING && !bCHGING && g_tBatPackInfo.tCell.wVolMin < g_tBatPackCtrl.tEESpec.wCellVolMin)||g_tDiagnosticMsg.uRelaySt.tBits.bBm2) //
    {
        if (!deepsleep_started)
        {
            sleep_time = GetTick_ms();
            deepsleep_started = true;
        }
        else if(!deep_sleeping)
        {
            if ((GetTick_ms()  >= sleep_time+ 180 * TIME_1S) ||g_tDiagnosticMsg.uRelaySt.tBits.bBm2) 
            {
                deep_sleeping = true;
                g_tDiagnosticMsg.uRelaySt.tBits.bBm2=0;
                byEEMemoryWrite(EE_RUN_ADDR, (uint8_t *)& g_tBatPackCtrl.tEERun, EE_RUN_LEN);
                DVC1124_Offmos();
							 /* TIM1 enable update irq */
								TIM_ConfigInt(TIM1, TIM_INT_UPDATE, DISABLE);

								/* TIM1 enable counter */
								TIM_Enable(TIM1, DISABLE); 
							  Delay_ms(2);
                GPIO_ResetBits(DCDCEN_PORT, DCDCEN_PIN);
                GPIO_SetBits(M3V3_CTRL_PORT, M3V3_CTRL_PIN);
                GPIO_ResetBits(M5V_CTRL_PORT, M5V_CTRL_PIN);
            }
        }
    }
    else
    {
        deep_sleeping = false;
				deepsleep_started=false;
    }
}
