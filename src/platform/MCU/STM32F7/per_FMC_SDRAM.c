#include "stm32f7xx_hal.h"

static void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef* hsdram);

SDRAM_HandleTypeDef hsdram1 = {
    .Instance   = FMC_SDRAM_DEVICE,
    .Init   = {
        .SDBank             = FMC_SDRAM_BANK1,
        .ColumnBitsNumber   = FMC_SDRAM_COLUMN_BITS_NUM_8,
        .RowBitsNumber      = FMC_SDRAM_ROW_BITS_NUM_12,
        .MemoryDataWidth    = FMC_SDRAM_MEM_BUS_WIDTH_32,
        .InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4,
        .CASLatency         = FMC_SDRAM_CAS_LATENCY_3,
        .WriteProtection    = FMC_SDRAM_WRITE_PROTECTION_DISABLE,
        .SDClockPeriod      = FMC_SDRAM_CLOCK_PERIOD_2, // HCLK/2
        .ReadBurst          = FMC_SDRAM_RBURST_ENABLE,
        // .ReadPipeDelay      = FMC_SDRAM_RPIPE_DELAY_1;  // 1 CK safe at 100 MHz
        .ReadPipeDelay      = FMC_SDRAM_RPIPE_DELAY_0,
    }
};
/*** Timing configuration for 100 MHz SDRAMCLK ***/
// ISSI IS42S32400F-6BL datasheet (tCK=10 ns at 100 MHz)
FMC_SDRAM_TimingTypeDef SdramTiming = {
    .LoadToActiveDelay      = 2, // tMRD ≥ 2
    .ExitSelfRefreshDelay   = 7, // tXSR ≥ 70 ns → 7 tCK
    .SelfRefreshTime        = 4, // tRAS ≥ 42 ns → 4 tCK
    .RowCycleDelay          = 7, // tRC ≥ 63 ns → 7 tCK
    .WriteRecoveryTime      = 3, // tWR ≥ 2 tCK
//   .RPDelay                = 2,
    .RPDelay                = 3, // tRP ≥ 18 ns → 3 tCK safer
//   .RCDDelay               = 2,
    .RCDDelay               = 3, // tRCD ≥ 18 ns → 3 tCK safe
};

void MPU_Config_SDRAM(void) {
    MPU_Region_InitTypeDef MPU_InitStruct;

    HAL_MPU_Disable();

    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress = 0xC0000000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_16MB; 
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    
    // make Normal Memory (Outer/Inner Non-cacheable)
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER0;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

void Error_Handler(void); // TODO: cleanup
void MX_FMC_Init() {
    // SCB->CCR &= ~SCB_CCR_UNALIGN_TRP_Msk; // TODO: disable it

    MPU_Config_SDRAM();
    /** Perform the SDRAM1 memory initialization sequence    */
    /*** GPIO clocks must be enabled ***/
    __HAL_RCC_FMC_CLK_ENABLE();

    /*** SDRAM device configuration ***/
    if (HAL_SDRAM_Init(&hsdram1, &SdramTiming) != HAL_OK) {
        Error_Handler();
    }

    /*** Run the JEDEC init sequence ***/
    SDRAM_Initialization_Sequence(&hsdram1);
}

static void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef* hsdram) {
    /* Step 1: Configure a clock enable command */
    FMC_SDRAM_CommandTypeDef Command = {
        .CommandMode            = FMC_SDRAM_CMD_CLK_ENABLE,
        .CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1,
        .AutoRefreshNumber      = 1,
        .ModeRegisterDefinition = 0,
    };

    HAL_SDRAM_SendCommand(hsdram, &Command, HAL_MAX_DELAY);

    HAL_Delay(1);   /* Step 2: Insert delay ≥100 µs */

    /* Step 3: Precharge all command */
    Command.CommandMode = FMC_SDRAM_CMD_PALL;
    HAL_SDRAM_SendCommand(hsdram, &Command, HAL_MAX_DELAY);

    /* Step 4: Auto-refresh command (8 refresh cycles) */
    Command.CommandMode       = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
    Command.AutoRefreshNumber = 8;
    HAL_SDRAM_SendCommand(hsdram, &Command, HAL_MAX_DELAY);

    /* Step 5: Load Mode Register */
    // Mode word = burst length 1, sequential, CAS=3, standard, write burst = single
    uint32_t mode_reg = 0
        | 0x0000               // burst length = 1
        | 0x0000               // burst type = sequential
        | 0x0030               // CAS latency = 3
        | 0x0000               // standard operation
        | 0x0200;              // write burst = single

    Command.CommandMode            = FMC_SDRAM_CMD_LOAD_MODE;
    Command.ModeRegisterDefinition = mode_reg;
    HAL_SDRAM_SendCommand(hsdram, &Command, HAL_MAX_DELAY);

    /* Step 6: Set the refresh rate counter */
    // tREFI = 7.81 µs @ 8192 rows → 64 ms / 8192 = 7.81 µs
    // RefreshCount = (SDRAM_CLK * 7.81e-6) - 20
    // For 100 MHz SDRAMCLK: 100e6 * 7.81e-6 = 781 → -20 ≈ 761
    HAL_SDRAM_ProgramRefreshRate(hsdram, 761);
}
