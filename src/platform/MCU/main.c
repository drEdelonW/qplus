/* Includes ------------------------------------------------------------------*/
// #include "main.h"
#include "perepherial.h"
#include "string.h"
#include "cmsis_os.h"

osThreadId defaultTaskHandle;
void StartDefaultTask(void const* argument);

int main() {
    CoreClock_Init();
    Pereph_Init();

    osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 4096);  /* Create the thread(s) */
    defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);        /* definition and creation of defaultTask */

    osKernelStart();    /* Start scheduler */

    /* We should never get here as control is now taken by the scheduler */
    while (1) {}    /* Infinite loop */
}


#include <stdint.h>
#include <string.h>

/* Match your LTDC layer config (see MX_LTDC_Init): 200 x 480 RGB565 */
#define SCREEN_W       200u
#define SCREEN_H       480u
#define BYTES_PER_PIX  2u

/* Place framebuffer in AXI-SRAM (accessible by LTDC).
 * If your linker has a different name for AXI SRAM section, change ".RAM_D1" to your section
 * (e.g. ".sram1", ".AXI_SRAM"). If no section exists, temporarily remove the attribute
 * and ensure linker places this array into AXI-SRAM, NOT DTCM.
 */
#if defined(__GNUC__)
__attribute__((section(".RAM_D1"), aligned(32)))
#endif
static uint16_t s_fb[SCREEN_W * SCREEN_H];

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static inline void dcache_clean_range(void* addr, size_t size) {
    uintptr_t a = ((uintptr_t)addr) & ~31u;               // 32B line align
    size += ((uintptr_t)addr - a);
    size = (size + 31u) & ~31u;
    SCB_CleanDCache_by_Addr((uint32_t*)a, (int32_t)size);
}

void draw_gradient(uint16_t* fb) {
    for (uint32_t y = 0; y < SCREEN_H; ++y) {
        uint8_t t = (uint8_t)((y * 255u) / (SCREEN_H - 1u));
        uint16_t c = rgb565(0, t, 255);
        uint16_t* row = fb + y * SCREEN_W;
        for (uint32_t x = 0; x < SCREEN_W; ++x)
            row[x] = c;
    }
}

void fill_rect(uint16_t* fb, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c) {
    if (x >= SCREEN_W || y >= SCREEN_H) return;
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    for (uint32_t j = 0; j < h; ++j) {
        uint16_t* row = fb + (y + j) * SCREEN_W + x;
        for (uint32_t i = 0; i < w; ++i)
            row[i] = c;
    }
}
#if 1
#include "sys.h"
#include "errno.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "sys.h"
#include "host.h"
void StartDefaultTask(void const* argument) {
    /* Infinite loop */
    // for (;;) {
    //     osDelay(1);
    // }
        static quakeparms_t parms;
    int argc;
    cstring* argv;
    parms.memsize = 8 * 1024 * 1024;
    parms.membase = malloc(parms.memsize);
    parms.basedir = ".";

    COM_InitArgv(argc, argv);

    parms.argc = com_argc;
    parms.argv = com_argv;

    printf("Host_Init\n");
    Host_Init(&parms);
    while (1) {
        Host_Frame(0.1);
    }
}
#else
void StartDefaultTask(void const* argument) {
    /* Clear and draw initial background in internal RAM */
    draw_gradient(s_fb);
    dcache_clean_range(s_fb, SCREEN_W * SCREEN_H * BYTES_PER_PIX);
    HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_IMMEDIATE);

    /* Bouncing box state */
    int x = 20, y = 20, vx = 2, vy = 3;
    const int bw = 40, bh = 40;

    for (;;) {
        /* 1) Background */
        draw_gradient(s_fb);

        /* 2) Update position */
        x += vx; y += vy;
        if (x < 0) { x = 0; vx = -vx; }
        if (y < 0) { y = 0; vy = -vy; }
        if (x + bw > (int)SCREEN_W) { x = (int)SCREEN_W - bw; vx = -vx; }
        if (y + bh > (int)SCREEN_H) { y = (int)SCREEN_H - bh; vy = -vy; }

        /* 3) Draw box */
        fill_rect(s_fb, (uint32_t)x, (uint32_t)y, (uint32_t)bw, (uint32_t)bh, rgb565(255, 255, 0));

        /* 4) Make sure LTDC sees new pixels */
        dcache_clean_range(s_fb, SCREEN_W * SCREEN_H * BYTES_PER_PIX);

        /* 5) No address change needed (single-buffer), just ask LTDC to reload regs */
        HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_VERTICAL_BLANKING);

        osDelay(16); // ~60 FPS
    }
}
#endif
/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim) {
    if (htim->Instance == TIM6) {
        HAL_IncTick();
    }
}

void Error_Handler() {
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
    }
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line) {
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
}
#endif /* USE_FULL_ASSERT */
