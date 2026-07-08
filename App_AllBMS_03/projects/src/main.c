

/**
 * @file main.c
 * @author Lb
 * @version V1.0.0
 *
 * @copyright BMS24S
 */
#include "main.h"
#include <stdio.h>
#include <stdint.h>
#include "spi_flash.h"
#include "TimeModule.h"

typedef struct {
    uint64_t last_exec_time;  // 上次执行时间
    uint32_t interval;        // 执行间隔（毫秒）
    void (*task_func)(void);  // 任务函数指针
} TimerTask;

// 全局变量
uint64_t l_u64TickMs;
TimerTask g_tasks[10];  // 假设最多10个任务
uint8_t g_task_count = 0;


void register_timer_task(uint32_t interval, void (*task_func)(void)) {
    if (g_task_count >= 10) {
        return;
    }
    g_tasks[g_task_count].last_exec_time = GetTick_ms();
    g_tasks[g_task_count].interval = interval;
    g_tasks[g_task_count].task_func = task_func;
    g_task_count++;
    
}

void execute_timer_tasks(uint64_t current_time) {
    for (uint8_t i = 0; i < g_task_count; i++) {
        TimerTask* task = &g_tasks[i];
        
        if (current_time - task->last_exec_time >= task->interval) {
        
            uint64_t start_time = GetTick_ms();
            if (task->task_func != NULL) {
                task->task_func();
            }
            
            task->last_exec_time = current_time;
        }
    }
}

static void LogPeriodicRecord(void);

void init_all_tasks(void) {
    // 注册所有定时任务
    register_timer_task(10,  System_operation);
    register_timer_task(20,  AFE_Handle);         
    register_timer_task(20,  ExecMainMsgProcess);
    register_timer_task(50,  SocEstimation);
    register_timer_task(50,  ExecUartMsgProcess);
    register_timer_task(50,  ExecBleMsgProcess);
    register_timer_task(500,  SendSifData);
    register_timer_task(1000, LogPeriodicRecord);

}

/* BMS历史日志：每秒推进运行时间；SOC变化即记录，无变化则每30秒记录一次 */
static void LogPeriodicRecord(void)
{
    static uint16_t s_wLastSoc = 0xFFFF;
    static uint8_t  s_byNoChangeSec = 0;
    uint16_t wDelta;

    /* 每秒推进运行时间 1 秒 */
    DATE_UtcToDate(1,&g_tBatPackCtrl.tEERun.tRunTime);

    wDelta = (g_tBatPackInfo.wSocShow > s_wLastSoc)
           ? (g_tBatPackInfo.wSocShow - s_wLastSoc)
           : (s_wLastSoc - g_tBatPackInfo.wSocShow);

    if (wDelta >= 100)                 /* SOC 有变动：立即记录 */
    {
        s_wLastSoc = g_tBatPackInfo.wSocShow;
        s_byNoChangeSec = 0;
        ExecLogBmStRecord();
    }
    else if (++s_byNoChangeSec >= 30)  /* SOC 无变动：每 30 秒记录一次 */
    {
        s_byNoChangeSec = 0;
        ExecLogBmStRecord();
    }
}

void Init_Handle(void)
{
      
    Clk_Init();
    Port_Config();
    Usart485_Init();
    Uart_Init();
    ADC_Initial();
    AFE_Iicinit();
    Sif_Init();  
    AX24_SI2CInit();
    App_Init();
    FaultLevelInit();
    AFEParam_Config();
    CAN_User_Init();
    ble_init();
    CurDriftCalib_Init();
    Flash_MemoryInit();

}

/**
 * @brief  Main program.
 */

int main(void)
{

    Init_Handle();
    init_all_tasks();
    while (1)
    {

        l_u64TickMs = GetTick_ms();
        execute_timer_tasks(l_u64TickMs);
        MosCtrl();
        ble_loop();
        CanTxRx();
        Shallow_Sleep();
        Deep_Sleep();	
    }

}
/**
 * @}
 */
