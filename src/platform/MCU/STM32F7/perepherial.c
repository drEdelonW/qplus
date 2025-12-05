#include "perepherial.h"
#include "terminal_tools.h"


#if defined ( __ICCARM__ ) /*!< IAR Compiler */
#pragma location=0x2007c000
ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
#pragma location=0x2007c0a0
ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */
#elif defined ( __CC_ARM )  /* MDK ARM Compiler */
__attribute__((at(0x2007c000))) ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
__attribute__((at(0x2007c0a0))) ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */
#elif defined ( __GNUC__ ) /* GNU Compiler */
ETH_DMADescTypeDef DMARxDscrTab[ETH_RX_DESC_CNT] __attribute__((section(".RxDecripSection"))); /* Ethernet Rx DMA Descriptors */
ETH_DMADescTypeDef DMATxDscrTab[ETH_TX_DESC_CNT] __attribute__((section(".TxDecripSection")));   /* Ethernet Tx DMA Descriptors */
#endif

#if 0
ETH_TxPacketConfig TxConfig = {
    .Attributes =
        ETH_TX_PACKETS_FEATURES_CSUM |
        ETH_TX_PACKETS_FEATURES_CRCPAD,
    .ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC,
    .CRCPadCtrl = ETH_CRC_PAD_INSERT,
};
#endif

#if 0
ADC_HandleTypeDef hadc1 = {
    .Instance = ADC1,
    .Init = {
        .ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4,
        .Resolution = ADC_RESOLUTION_12B,
        .ScanConvMode = ADC_SCAN_DISABLE,
        .ContinuousConvMode = DISABLE,
        .DiscontinuousConvMode = DISABLE,
        .ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE,
        .ExternalTrigConv = ADC_SOFTWARE_START,
        .DataAlign = ADC_DATAALIGN_RIGHT,
        .NbrOfConversion = 1,
        .DMAContinuousRequests = DISABLE,
        .EOCSelection = ADC_EOC_SINGLE_CONV
    }
};
void MX_ADC1_Init() {
    /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)  */
    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        Error_Handler();
    }

    /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.    */
    ADC_ChannelConfTypeDef sConfig = {
        .Channel = ADC_CHANNEL_12,
        .Rank = ADC_REGULAR_RANK_1,
        .SamplingTime = ADC_SAMPLETIME_3CYCLES,
    };

    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        Error_Handler();
    }
}
#endif

#if 0
ADC_HandleTypeDef hadc3 = {
    .Instance = ADC3,
    .Init = {
        .ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4,
        .Resolution = ADC_RESOLUTION_12B,
        .ScanConvMode = ADC_SCAN_DISABLE,
        .ContinuousConvMode = DISABLE,
        .DiscontinuousConvMode = DISABLE,
        .ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE,
        .ExternalTrigConv = ADC_SOFTWARE_START,
        .DataAlign = ADC_DATAALIGN_RIGHT,
        .NbrOfConversion = 1,
        .DMAContinuousRequests = DISABLE,
        .EOCSelection = ADC_EOC_SINGLE_CONV
    }
};
void MX_ADC3_Init() {
    /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)    */
    if (HAL_ADC_Init(&hadc3) != HAL_OK) {
        Error_Handler();
    }

    /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.     */
    ADC_ChannelConfTypeDef sConfig = {
        .Channel = ADC_CHANNEL_6,
        .Rank = ADC_REGULAR_RANK_1,
        .SamplingTime = ADC_SAMPLETIME_3CYCLES,
    };
    if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK) {
        Error_Handler();
    }
}
#endif

#if 0
CRC_HandleTypeDef hcrc = {
    .Instance = CRC,
    .Init = {
        .DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE,
        .DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE,
        .InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE,
        .OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE,
    },
    .InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES,
};
void MX_CRC_Init() {
    if (HAL_CRC_Init(&hcrc) != HAL_OK) {
        Error_Handler();
    }
}
#endif

DMA2D_HandleTypeDef hdma2d = {
    .Instance = DMA2D,
    .Init = {
        .Mode = DMA2D_M2M,
        .ColorMode = DMA2D_OUTPUT_ARGB8888,
        .OutputOffset = 0,
    },
    .LayerCfg = {
        {},
        {
            .InputOffset = 0,
            .InputColorMode = DMA2D_INPUT_ARGB8888,
            .AlphaMode = DMA2D_NO_MODIF_ALPHA,
            .InputAlpha = 0,
            .AlphaInverted = DMA2D_REGULAR_ALPHA,
            .RedBlueSwap = DMA2D_RB_REGULAR,
        }
    }
};
void MX_DMA2D_Init() {
    if ((HAL_DMA2D_Init(&hdma2d) != HAL_OK) ||
        (HAL_DMA2D_ConfigLayer(&hdma2d, 1) != HAL_OK)) {
        Error_Handler();
    }
}


DSI_HandleTypeDef hdsi = {
    .Instance = DSI,
    .Init = {
        .AutomaticClockLaneControl = DSI_AUTO_CLK_LANE_CTRL_DISABLE,
        .TXEscapeCkdiv = 4,
        .NumberOfLanes = DSI_TWO_DATA_LANES,
    },
};
void MX_DSIHOST_DSI_Init() {
    DSI_PLLInitTypeDef PLLInit = {
        .PLLNDIV = 20,
        .PLLIDF = DSI_PLL_IN_DIV1,
        .PLLODF = DSI_PLL_OUT_DIV1,
    };
    DSI_HOST_TimeoutTypeDef HostTimeouts = {
        .TimeoutCkdiv = 1,
        .HighSpeedTransmissionTimeout = 0,
        .LowPowerReceptionTimeout = 0,
        .HighSpeedReadTimeout = 0,
        .LowPowerReadTimeout = 0,
        .HighSpeedWriteTimeout = 0,
        .HighSpeedWritePrespMode = DSI_HS_PM_DISABLE,
        .LowPowerWriteTimeout = 0,
        .BTATimeout = 0,
    };
    DSI_PHY_TimerTypeDef PhyTimings = {
        .ClockLaneHS2LPTime = 28,
        .ClockLaneLP2HSTime = 33,
        .DataLaneHS2LPTime = 15,
        .DataLaneLP2HSTime = 25,
        .DataLaneMaxReadTime = 0,
        .StopWaitTime = 10,
    };
    DSI_LPCmdTypeDef LPCmd = {
        .LPGenShortWriteNoP = DSI_LP_GSW0P_ENABLE,
        .LPGenShortWriteOneP = DSI_LP_GSW1P_ENABLE,
        .LPGenShortWriteTwoP = DSI_LP_GSW2P_ENABLE,
        .LPGenShortReadNoP = DSI_LP_GSR0P_ENABLE,
        .LPGenShortReadOneP = DSI_LP_GSR1P_ENABLE,
        .LPGenShortReadTwoP = DSI_LP_GSR2P_ENABLE,
        .LPGenLongWrite = DSI_LP_GLW_ENABLE,
        .LPDcsShortWriteNoP = DSI_LP_DSW0P_ENABLE,
        .LPDcsShortWriteOneP = DSI_LP_DSW1P_ENABLE,
        .LPDcsShortReadNoP = DSI_LP_DSR0P_ENABLE,
        .LPDcsLongWrite = DSI_LP_DLW_ENABLE,
        .LPMaxReadPacket = DSI_LP_MRDP_ENABLE,
        .AcknowledgeRequest = DSI_ACKNOWLEDGE_DISABLE,
    };
    DSI_CmdCfgTypeDef CmdCfg = {
        .VirtualChannelID = 0,
        .ColorCoding = DSI_RGB888,
        .CommandSize = 400,
        .TearingEffectSource = DSI_TE_EXTERNAL,
        .TearingEffectPolarity = DSI_TE_RISING_EDGE,
        .HSPolarity = DSI_HSYNC_ACTIVE_LOW,
        .VSPolarity = DSI_VSYNC_ACTIVE_LOW,
        .DEPolarity = DSI_DATA_ENABLE_ACTIVE_HIGH,
        .VSyncPol = DSI_VSYNC_FALLING,
        .AutomaticRefresh = DSI_AR_ENABLE,
        .TEAcknowledgeRequest = DSI_TE_ACKNOWLEDGE_ENABLE,
    };

    if ((HAL_DSI_Init(&hdsi, &PLLInit) != HAL_OK) ||
        (HAL_DSI_ConfigHostTimeouts(&hdsi, &HostTimeouts) != HAL_OK) ||
        (HAL_DSI_ConfigPhyTimer(&hdsi, &PhyTimings) != HAL_OK) ||
        (HAL_DSI_ConfigFlowControl(&hdsi, DSI_FLOW_CONTROL_BTA) != HAL_OK) ||
        (HAL_DSI_SetLowPowerRXFilter(&hdsi, 10000) != HAL_OK) ||
        (HAL_DSI_ConfigErrorMonitor(&hdsi, HAL_DSI_ERROR_NONE) != HAL_OK) ||
        (HAL_DSI_ConfigCommand(&hdsi, &LPCmd) != HAL_OK) ||
        (HAL_DSI_ConfigAdaptedCommandMode(&hdsi, &CmdCfg) != HAL_OK) ||
        (HAL_DSI_SetGenericVCID(&hdsi, 0) != HAL_OK)
        ) {
        Error_Handler();
    }
}


static uint8_t MACAddr[6] = {
    0x00, 0x80, 0xE1, 0x00, 0x00, 0x00
};
ETH_HandleTypeDef heth = {
    .Instance = ETH,
    .Init = {
        .MACAddr = &MACAddr[0],
        .MediaInterface = HAL_ETH_RMII_MODE,
        .TxDesc = DMATxDscrTab,
        .RxDesc = DMARxDscrTab,
        .RxBuffLen = 1524,
    }
};
void MX_ETH_Init() {
    if (HAL_ETH_Init(&heth) != HAL_OK) {
        Error_Handler();
    }
}


uint8_t cec_receive_buffer[16] = { 0 };
CEC_HandleTypeDef hcec = {
    .Instance = CEC,
    .Init = {
        .SignalFreeTime = CEC_DEFAULT_SFT,
        .Tolerance = CEC_STANDARD_TOLERANCE,
        .BRERxStop = CEC_RX_STOP_ON_BRE,
        .BREErrorBitGen = CEC_BRE_ERRORBIT_NO_GENERATION,
        .LBPEErrorBitGen = CEC_LBPE_ERRORBIT_NO_GENERATION,
        .BroadcastMsgNoErrorBitGen = CEC_BROADCASTERROR_ERRORBIT_GENERATION,
        .SignalFreeTimeOption = CEC_SFT_START_ON_TXSOM,
        .ListenMode = CEC_FULL_LISTENING_MODE,
        .OwnAddress = CEC_OWN_ADDRESS_NONE,
        .RxBuffer = cec_receive_buffer,
    }
};
void MX_HDMI_CEC_Init() {
    if (HAL_CEC_Init(&hcec) != HAL_OK) {
        Error_Handler();
    }
}


#if 0
I2C_HandleTypeDef hi2c1 = {
    .Instance = I2C1,
    .Init = {
        .Timing = 0x20404768,
        .OwnAddress1 = 0,
        .AddressingMode = I2C_ADDRESSINGMODE_7BIT,
        .DualAddressMode = I2C_DUALADDRESS_DISABLE,
        .OwnAddress2 = 0,
        .OwnAddress2Masks = I2C_OA2_NOMASK,
        .GeneralCallMode = I2C_GENERALCALL_DISABLE,
        .NoStretchMode = I2C_NOSTRETCH_DISABLE,
    }
};
void MX_I2C1_Init() {
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }
    /** Configure Analogue filter */
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        Error_Handler();
    }
    /** Configure Digital filter */
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) {
        Error_Handler();
    }
}
#endif


I2C_HandleTypeDef hi2c4 = {
    .Instance = I2C4,
    .Init = {
        .Timing = 0x20404768,
        .OwnAddress1 = 0,
        .AddressingMode = I2C_ADDRESSINGMODE_7BIT,
        .DualAddressMode = I2C_DUALADDRESS_DISABLE,
        .OwnAddress2 = 0,
        .OwnAddress2Masks = I2C_OA2_NOMASK,
        .GeneralCallMode = I2C_GENERALCALL_DISABLE,
        .NoStretchMode = I2C_NOSTRETCH_DISABLE,
    }
};
void MX_I2C4_Init() {
    if (HAL_I2C_Init(&hi2c4) != HAL_OK) {
        Error_Handler();
    }
    /** Configure Analogue filter */
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c4, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        Error_Handler();
    }
    /** Configure Digital filter */
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c4, 0) != HAL_OK) {
        Error_Handler();
    }
}


#if 0
IWDG_HandleTypeDef hiwdg = {
    .Instance = IWDG,
    .Init = {
        .Prescaler = IWDG_PRESCALER_4,
        .Window = 4095,
        .Reload = 4095,
    }
};
void MX_IWDG_Init() {
    if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
        Error_Handler();
    }
}
#endif

LTDC_HandleTypeDef hltdc = {
    .Instance = LTDC,
    .Init = {
        .HSPolarity = LTDC_HSPOLARITY_AL,
        .VSPolarity = LTDC_VSPOLARITY_AL,
        .DEPolarity = LTDC_DEPOLARITY_AL,
        .PCPolarity = LTDC_PCPOLARITY_IPC,
        .HorizontalSync = 0,
        .VerticalSync = 0,
        .AccumulatedHBP = 1,
        .AccumulatedVBP = 1,
        .AccumulatedActiveW = 201,
        .AccumulatedActiveH = 481,
        .TotalWidth = 202,
        .TotalHeigh = 482,
        .Backcolor = {
            .Blue = 0,
            .Green = 0,
            .Red = 0,
        }
    }
};
void MX_LTDC_Init() {
    if (HAL_LTDC_Init(&hltdc) != HAL_OK) {
        Error_Handler();
    }
    LTDC_LayerCfgTypeDef pLayerCfg = {
        .WindowX0 = 0,
        .WindowX1 = 200,
        .WindowY0 = 0,
        .WindowY1 = 480,
        .PixelFormat = LTDC_PIXEL_FORMAT_RGB565,
        .Alpha = 255,
        .Alpha0 = 0,
        .BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA,
        .BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA,
        .FBStartAdress = 0xC0000000,
        .ImageWidth = 200,
        .ImageHeight = 480,
        .Backcolor = {
            .Blue = 0,
            .Green = 0,
            .Red = 0,
        }
    };
    if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, 0) != HAL_OK) {
        Error_Handler();
    }
}


QSPI_HandleTypeDef hqspi = {
    .Instance = QUADSPI,
    .Init = {
        .ClockPrescaler = 1,
        .FifoThreshold = 16,
        .SampleShifting = QSPI_SAMPLE_SHIFTING_HALFCYCLE,
        .FlashSize = 26,
        .ChipSelectHighTime = QSPI_CS_HIGH_TIME_4_CYCLE,
        .ClockMode = QSPI_CLOCK_MODE_0,
        .FlashID = QSPI_FLASH_ID_1,
        .DualFlash = QSPI_DUALFLASH_DISABLE,
    }
};
void MX_QUADSPI_Init() {
    /* QUADSPI parameter configuration*/
    if (HAL_QSPI_Init(&hqspi) != HAL_OK) {
        Error_Handler();
    }
}

#if 0
RTC_HandleTypeDef hrtc = {
    .Instance = RTC,
    .Init = {
        .HourFormat = RTC_HOURFORMAT_24,
        .AsynchPrediv = 127,
        .SynchPrediv = 255,
        .OutPut = RTC_OUTPUT_ALARMA,
        .OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH,
        .OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN,
    },
};
void MX_RTC_Init() {
    /** Initialize RTC Only */
    if (HAL_RTC_Init(&hrtc) != HAL_OK) {
        Error_Handler();
    }
    /** Initialize RTC and set the Time and Date    */
    RTC_TimeTypeDef sTime = {
        .Hours = 0x0,
        .Minutes = 0x0,
        .Seconds = 0x0,
        .DayLightSaving = RTC_DAYLIGHTSAVING_NONE,
        .StoreOperation = RTC_STOREOPERATION_RESET,
    };
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK) {
        Error_Handler();
    }
    RTC_DateTypeDef sDate = {
        .WeekDay = RTC_WEEKDAY_MONDAY,
        .Month = RTC_MONTH_JANUARY,
        .Date = 0x1,
        .Year = 0x0,
    };

    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK) {
        Error_Handler();
    }

    /** Enable the Alarm A */
    RTC_AlarmTypeDef sAlarm = {
        .AlarmTime = {
            .Hours = 0x0,
            .Minutes = 0x0,
            .Seconds = 0x0,
            .SubSeconds = 0x0,
            .DayLightSaving = RTC_DAYLIGHTSAVING_NONE,
            .StoreOperation = RTC_STOREOPERATION_RESET,
        },
        .AlarmMask = RTC_ALARMMASK_NONE,
        .AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL,
        .AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE,
        .AlarmDateWeekDay = 0x1,
        .Alarm = RTC_ALARM_A,
    };
    if (HAL_RTC_SetAlarm(&hrtc, &sAlarm, RTC_FORMAT_BCD) != HAL_OK) {
        Error_Handler();
    }

    /** Enable the Alarm B */
    sAlarm.Alarm = RTC_ALARM_B;
    if (HAL_RTC_SetAlarm(&hrtc, &sAlarm, RTC_FORMAT_BCD) != HAL_OK) {
        Error_Handler();
    }
}
#endif


SAI_HandleTypeDef hsai_BlockA1 = {
    .Instance = SAI1_Block_A,
    .Init = {
        .Protocol = SAI_FREE_PROTOCOL,
        .AudioMode = SAI_MODEMASTER_TX,
        .DataSize = SAI_DATASIZE_8,
        .FirstBit = SAI_FIRSTBIT_MSB,
        .ClockStrobing = SAI_CLOCKSTROBING_FALLINGEDGE,
        .Synchro = SAI_ASYNCHRONOUS,
        .OutputDrive = SAI_OUTPUTDRIVE_DISABLE,
        .NoDivider = SAI_MASTERDIVIDER_ENABLE,
        .FIFOThreshold = SAI_FIFOTHRESHOLD_EMPTY,
        .AudioFrequency = SAI_AUDIO_FREQUENCY_192K,
        .SynchroExt = SAI_SYNCEXT_DISABLE,
        .MonoStereoMode = SAI_STEREOMODE,
        .CompandingMode = SAI_NOCOMPANDING,
        .TriState = SAI_OUTPUT_NOTRELEASED,
    },
    .FrameInit = {
        .FrameLength = 8,
        .ActiveFrameLength = 1,
        .FSDefinition = SAI_FS_STARTFRAME,
        .FSPolarity = SAI_FS_ACTIVE_LOW,
        .FSOffset = SAI_FS_FIRSTBIT,
    },
    .SlotInit = {
        .FirstBitOffset = 0,
        .SlotSize = SAI_SLOTSIZE_DATASIZE,
        .SlotNumber = 1,
        .SlotActive = 0x00000000,
    }
};
SAI_HandleTypeDef hsai_BlockB1 = {
    .Instance = SAI1_Block_B,
    .Init = {
        .Protocol = SAI_FREE_PROTOCOL,
        .AudioMode = SAI_MODESLAVE_RX,
        .DataSize = SAI_DATASIZE_8,
        .FirstBit = SAI_FIRSTBIT_MSB,
        .ClockStrobing = SAI_CLOCKSTROBING_FALLINGEDGE,
        .Synchro = SAI_SYNCHRONOUS,
        .OutputDrive = SAI_OUTPUTDRIVE_DISABLE,
        .FIFOThreshold = SAI_FIFOTHRESHOLD_EMPTY,
        .SynchroExt = SAI_SYNCEXT_DISABLE,
        .MonoStereoMode = SAI_STEREOMODE,
        .CompandingMode = SAI_NOCOMPANDING,
        .TriState = SAI_OUTPUT_NOTRELEASED,
    },
    .FrameInit = {
        .FrameLength = 8,
        .ActiveFrameLength = 1,
        .FSDefinition = SAI_FS_STARTFRAME,
        .FSPolarity = SAI_FS_ACTIVE_LOW,
        .FSOffset = SAI_FS_FIRSTBIT,
    },
    .SlotInit = {
        .FirstBitOffset = 0,
        .SlotSize = SAI_SLOTSIZE_DATASIZE,
        .SlotNumber = 1,
        .SlotActive = 0x00000000,
    }
};
void MX_SAI1_Init() {
    if (HAL_SAI_Init(&hsai_BlockA1) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_SAI_Init(&hsai_BlockB1) != HAL_OK) {
        Error_Handler();
    }
}


#if 0
SAI_HandleTypeDef hsai_BlockA2 = {
    .Instance = SAI2_Block_A,
    .Init = {
        .Protocol = SAI_SPDIF_PROTOCOL,
        .AudioMode = SAI_MODEMASTER_TX,
        .Synchro = SAI_ASYNCHRONOUS,
        .OutputDrive = SAI_OUTPUTDRIVE_DISABLE,
        .FIFOThreshold = SAI_FIFOTHRESHOLD_EMPTY,
        .AudioFrequency = SAI_AUDIO_FREQUENCY_48K,
        .MonoStereoMode = SAI_STEREOMODE,
        .CompandingMode = SAI_NOCOMPANDING,
    }
};
void MX_SAI2_Init() {
    if (HAL_SAI_Init(&hsai_BlockA2) != HAL_OK) {
        Error_Handler();
    }
}
#endif




#if 0
SPDIFRX_HandleTypeDef hspdif = {
    .Instance = SPDIFRX,
    .Init = {
        .InputSelection = SPDIFRX_INPUT_IN1,
        .Retries = SPDIFRX_MAXRETRIES_NONE,
        .WaitForActivity = SPDIFRX_WAITFORACTIVITY_OFF,
        .ChannelSelection = SPDIFRX_CHANNEL_A,
        .DataFormat = SPDIFRX_DATAFORMAT_LSB,
        .StereoMode = SPDIFRX_STEREOMODE_DISABLE,
        .PreambleTypeMask = SPDIFRX_PREAMBLETYPEMASK_OFF,
        .ChannelStatusMask = SPDIFRX_CHANNELSTATUS_OFF,
        .ValidityBitMask = SPDIFRX_VALIDITYMASK_OFF,
        .ParityErrorMask = SPDIFRX_PARITYERRORMASK_OFF,
    }
};
void MX_SPDIFRX_Init() {
    if (HAL_SPDIFRX_Init(&hspdif) != HAL_OK) {
        Error_Handler();
    }
}
#endif


#if 0
SPI_HandleTypeDef hspi2 = {
    .Instance = SPI2,
    .Init = {
        .Mode = SPI_MODE_MASTER,
        .Direction = SPI_DIRECTION_2LINES,
        .DataSize = SPI_DATASIZE_4BIT,
        .CLKPolarity = SPI_POLARITY_LOW,
        .CLKPhase = SPI_PHASE_1EDGE,
        .NSS = SPI_NSS_HARD_INPUT,
        .BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2,
        .FirstBit = SPI_FIRSTBIT_MSB,
        .TIMode = SPI_TIMODE_DISABLE,
        .CRCCalculation = SPI_CRCCALCULATION_DISABLE,
        .CRCPolynomial = 7,
        .CRCLength = SPI_CRC_LENGTH_DATASIZE,
        .NSSPMode = SPI_NSS_PULSE_ENABLE,
    }
};
void MX_SPI2_Init() {
    /* SPI2 parameter configuration*/
    if (HAL_SPI_Init(&hspi2) != HAL_OK) {
        Error_Handler();
    }
}
#endif



TIM_HandleTypeDef htim1 = {
    .Instance = TIM1,
    .Init = {
        .Prescaler = 0,
        .CounterMode = TIM_COUNTERMODE_UP,
        .Period = 65535,
        .ClockDivision = TIM_CLOCKDIVISION_DIV1,
        .RepetitionCounter = 0,
        .AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE,
    }
};
void MX_TIM1_Init() {
    if (HAL_TIM_Base_Init(&htim1) != HAL_OK) {
        Error_Handler();
    }
    TIM_ClockConfigTypeDef sClockSourceConfig = {
        .ClockSource = TIM_CLOCKSOURCE_INTERNAL
    };
    if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }
    TIM_MasterConfigTypeDef sMasterConfig = {
        .MasterOutputTrigger = TIM_TRGO_RESET,
        .MasterOutputTrigger2 = TIM_TRGO2_RESET,
        .MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE,
    };
    if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
}


#if 0
TIM_HandleTypeDef htim3 = {
    .Instance = TIM3,
    .Init = {
        .Prescaler = 0,
        .CounterMode = TIM_COUNTERMODE_UP,
        .Period = 65535,
        .ClockDivision = TIM_CLOCKDIVISION_DIV1,
        .AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE,
    }
};
void MX_TIM3_Init() {
    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK) {
        Error_Handler();
    }
    TIM_MasterConfigTypeDef sMasterConfig = {
        .MasterOutputTrigger = TIM_TRGO_RESET,
        .MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE,
    };
    if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
    TIM_OC_InitTypeDef sConfigOC = {
        .OCMode = TIM_OCMODE_PWM1,
        .Pulse = 0,
        .OCPolarity = TIM_OCPOLARITY_HIGH,
        .OCFastMode = TIM_OCFAST_DISABLE,
    };
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK) {
        Error_Handler();
    }
    HAL_TIM_MspPostInit(&htim3);
}
#endif


#if 0
TIM_HandleTypeDef htim10 = {
    .Instance = TIM10,
    .Init = {
        .Prescaler = 0,
        .CounterMode = TIM_COUNTERMODE_UP,
        .Period = 65535,
        .ClockDivision = TIM_CLOCKDIVISION_DIV1,
        .AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE,
    }
};
void MX_TIM10_Init() {
    if (HAL_TIM_Base_Init(&htim10) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_TIM_PWM_Init(&htim10) != HAL_OK) {
        Error_Handler();
    }
    TIM_OC_InitTypeDef sConfigOC = {
        .OCMode = TIM_OCMODE_PWM1,
        .Pulse = 0,
        .OCPolarity = TIM_OCPOLARITY_HIGH,
        .OCFastMode = TIM_OCFAST_DISABLE,
    };
    if (HAL_TIM_PWM_ConfigChannel(&htim10, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }
    HAL_TIM_MspPostInit(&htim10);
}
#endif


#if 0
TIM_HandleTypeDef htim11 = {
    .Instance = TIM11,
    .Init = {
        .Prescaler = 0,
        .CounterMode = TIM_COUNTERMODE_UP,
        .Period = 65535,
        .ClockDivision = TIM_CLOCKDIVISION_DIV1,
        .AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE,
    }
};
void MX_TIM11_Init() {
    if (HAL_TIM_Base_Init(&htim11) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_TIM_PWM_Init(&htim11) != HAL_OK) {
        Error_Handler();
    }
    TIM_OC_InitTypeDef sConfigOC = {
        .OCMode = TIM_OCMODE_PWM1,
        .Pulse = 0,
        .OCPolarity = TIM_OCPOLARITY_HIGH,
        .OCFastMode = TIM_OCFAST_DISABLE,
    };
    if (HAL_TIM_PWM_ConfigChannel(&htim11, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }
    HAL_TIM_MspPostInit(&htim11);
}
#endif


#if 0
TIM_HandleTypeDef htim12 = {
    .Instance = TIM12,
    .Init = {
        .Prescaler = 0,
        .CounterMode = TIM_COUNTERMODE_UP,
        .Period = 65535,
        .ClockDivision = TIM_CLOCKDIVISION_DIV1,
        .AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE,
    }
};
void MX_TIM12_Init() {
    if (HAL_TIM_PWM_Init(&htim12) != HAL_OK) {
        Error_Handler();
    }
    TIM_OC_InitTypeDef sConfigOC = {
        .OCMode = TIM_OCMODE_PWM1,
        .Pulse = 0,
        .OCPolarity = TIM_OCPOLARITY_HIGH,
        .OCFastMode = TIM_OCFAST_DISABLE,
    };
    if (HAL_TIM_PWM_ConfigChannel(&htim12, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }
    HAL_TIM_MspPostInit(&htim12);
}
#endif



UART_HandleTypeDef huart5 = {
    .Instance = UART5,
    .Init = {
        .BaudRate = 115200,
        .WordLength = UART_WORDLENGTH_8B,
        .StopBits = UART_STOPBITS_1,
        .Parity = UART_PARITY_NONE,
        .Mode = UART_MODE_TX_RX,
        .HwFlowCtl = UART_HWCONTROL_NONE,
        .OverSampling = UART_OVERSAMPLING_16,
        .OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE,
    },
    .AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT,
};
void MX_UART5_Init() {
    if (HAL_UART_Init(&huart5) != HAL_OK) {
        Error_Handler();
    }
}



UART_HandleTypeDef huart1 = {
    .Instance = USART1,
    .Init = {
        .BaudRate = 115200,
        .WordLength = UART_WORDLENGTH_8B,
        .StopBits = UART_STOPBITS_1,
        .Parity = UART_PARITY_NONE,
        .Mode = UART_MODE_TX_RX,
        .HwFlowCtl = UART_HWCONTROL_NONE,
        .OverSampling = UART_OVERSAMPLING_16,
        .OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE,
    },
    .AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT,
};
void MX_USART1_UART_Init() {
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }
}



UART_HandleTypeDef huart6 = {
    .Instance = USART6,
    .Init = {
        .BaudRate = 115200,
        .WordLength = UART_WORDLENGTH_8B,
        .StopBits = UART_STOPBITS_1,
        .Parity = UART_PARITY_NONE,
        .Mode = UART_MODE_TX_RX,
        .HwFlowCtl = UART_HWCONTROL_NONE,
        .OverSampling = UART_OVERSAMPLING_16,
        .OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE,
    },
    .AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT,
};
void MX_USART6_UART_Init() {
    if (HAL_UART_Init(&huart6) != HAL_OK) {
        Error_Handler();
    }
}




PCD_HandleTypeDef hpcd_USB_OTG_HS = {
    .Instance = USB_OTG_HS,
    .Init = {
        .dev_endpoints = 9,
        .speed = PCD_SPEED_HIGH,
        .dma_enable = DISABLE,
        .phy_itface = USB_OTG_ULPI_PHY,
        .Sof_enable = DISABLE,
        .low_power_enable = DISABLE,
        .lpm_enable = DISABLE,
        .vbus_sensing_enable = DISABLE,
        .use_dedicated_ep1 = DISABLE,
        .use_external_vbus = DISABLE,
    }
};
void MX_USB_OTG_HS_PCD_Init() {
    if (HAL_PCD_Init(&hpcd_USB_OTG_HS) != HAL_OK) {
        Error_Handler();
    }
}


#if 0
WWDG_HandleTypeDef hwwdg = {
    .Instance = WWDG,
    .Init = {
        .Prescaler = WWDG_PRESCALER_1,
        .Window = 64,
        .Counter = 128,
        .EWIMode = WWDG_EWI_DISABLE,
    }
};
void MX_WWDG_Init() {
    if (HAL_WWDG_Init(&hwwdg) != HAL_OK) {
        Error_Handler();
    }
}
#endif


void SystemClock_Config() {
    HAL_PWR_EnableBkUpAccess(); /** Configure LSE Drive Capability  */

    /** Configure the main internal regulator output voltage */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /** Initializes the RCC Oscillators according to the specified parameters
    * in the RCC_OscInitTypeDef structure.    */
    RCC_OscInitTypeDef RCC_OscInitStruct = {
        .OscillatorType =
            RCC_OSCILLATORTYPE_HSI |
            RCC_OSCILLATORTYPE_LSI |
            RCC_OSCILLATORTYPE_HSE,
        .HSEState = RCC_HSE_ON,
        .HSIState = RCC_HSI_ON,
        .HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT,
        .LSIState = RCC_LSI_ON,
        .PLL = {
            .PLLState = RCC_PLL_ON,
            .PLLSource = RCC_PLLSOURCE_HSE,
            .PLLM = 25,
            .PLLN = 432,
            .PLLP = RCC_PLLP_DIV2,
            .PLLQ = 4,
            .PLLR = 2
        }
    };
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Activate the Over-Drive mode    */
    if (HAL_PWREx_EnableOverDrive() != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks    */
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {
        .ClockType =
            RCC_CLOCKTYPE_HCLK |
            RCC_CLOCKTYPE_SYSCLK |
            RCC_CLOCKTYPE_PCLK1 |
            RCC_CLOCKTYPE_PCLK2,
        .SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK,
        .AHBCLKDivider = RCC_SYSCLK_DIV1,
        .APB1CLKDivider = RCC_HCLK_DIV4,
        .APB2CLKDivider = RCC_HCLK_DIV2,
    };

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK) {
        Error_Handler();
    }
    HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSI, RCC_MCODIV_1);
}

void PeriphCommonClock_Config() {
    /** Initializes the peripherals clock */
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {
        .PeriphClockSelection =
            RCC_PERIPHCLK_LTDC |
            RCC_PERIPHCLK_SAI1 |
            RCC_PERIPHCLK_SAI2 |
            RCC_PERIPHCLK_SDMMC2 |
            RCC_PERIPHCLK_CLK48,
        .PLLSAI = {
            .PLLSAIN = 192,
            .PLLSAIR = 2,
            .PLLSAIQ = 3,
            .PLLSAIP = RCC_PLLSAIP_DIV4
        },
        .PLLSAIDivQ = 1,
        .PLLSAIDivR = RCC_PLLSAIDIVR_2,
        .Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLLSAI,
        .Sai2ClockSelection = RCC_SAI2CLKSOURCE_PLLSAI,
        .Clk48ClockSelection = RCC_CLK48SOURCE_PLLSAIP,
        .Sdmmc2ClockSelection = RCC_SDMMC2CLKSOURCE_CLK48,
    };
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
        Error_Handler();
    }
}


void MX_GPIO_Init() {
    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOJ_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();
    __HAL_RCC_GPIOK_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOJ, LD_USER1_Pin | DSI_RESET_Pin | LD_USER2_Pin, GPIO_PIN_RESET);

    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    /*Configure GPIO pins : LD_USER1_Pin DSI_RESET_Pin LD_USER2_Pin */
    GPIO_InitStruct.Pin = LD_USER1_Pin | DSI_RESET_Pin | LD_USER2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOJ, &GPIO_InitStruct);

    /*Configure GPIO pins : Audio_INT_Pin WIFI_RST_Pin ARD_D8_Pin ARD_D7_Pin ARD_D4_Pin ARD_D2_Pin */
    GPIO_InitStruct.Pin = Audio_INT_Pin | WIFI_RST_Pin | ARD_D8_Pin | ARD_D7_Pin | ARD_D4_Pin | ARD_D2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOJ, &GPIO_InitStruct);

    /*Configure GPIO pin : DFSDM_DATIN5_Pin */
    GPIO_InitStruct.Pin = DFSDM_DATIN5_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF3_DFSDM1;
    HAL_GPIO_Init(DFSDM_DATIN5_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pins : NC4_Pin NC5_Pin uSD_Detect_Pin LCD_BL_CTRL_Pin */
    GPIO_InitStruct.Pin = NC4_Pin | NC5_Pin | uSD_Detect_Pin | LCD_BL_CTRL_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

    /*Configure GPIO pins : NC3_Pin NC2_Pin NC1_Pin NC8_Pin NC7_Pin */
    GPIO_InitStruct.Pin = NC3_Pin | NC2_Pin | NC1_Pin | NC8_Pin | NC7_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOK, &GPIO_InitStruct);

    /*Configure GPIO pins : RMII_RXER_Pin OTG_FS_OverCurrent_Pin */
    GPIO_InitStruct.Pin = RMII_RXER_Pin | OTG_FS_OverCurrent_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /*Configure GPIO pin : DFSDM_CKOUT_Pin */
    GPIO_InitStruct.Pin = DFSDM_CKOUT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF3_DFSDM1;
    HAL_GPIO_Init(DFSDM_CKOUT_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : CEC_CLK_Pin */
    GPIO_InitStruct.Pin = CEC_CLK_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF0_MCO;
    HAL_GPIO_Init(CEC_CLK_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : LCD_INT_Pin */
    GPIO_InitStruct.Pin = LCD_INT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(LCD_INT_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pins : EXT_SDA_Pin EXT_SCL_Pin */
    GPIO_InitStruct.Pin = EXT_SDA_Pin | EXT_SCL_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    /*Configure GPIO pin : B_USER_Pin */
    GPIO_InitStruct.Pin = B_USER_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(B_USER_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : PH7 */
    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
}

void MX_FMC_Init();
void CoreClock_Init() {
    HAL_Init();                 /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    SystemClock_Config();       /* Configure the system clock */
    PeriphCommonClock_Config(); /* Configure the peripherals common clocks */
}

void PrintCpuClock() {
    SystemCoreClockUpdate();

    uint32_t hclk = HAL_RCC_GetHCLKFreq();
    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
    uint32_t pclk2 = HAL_RCC_GetPCLK2Freq();

    printf("CPU(HCLK): %.f MHz\r\n", (double)hclk / 1000000.0);
    printf("PCLK1:     %.f MHz\r\n", (double)pclk1 / 1000000.0);
    printf("PCLK2:     %.f MHz\r\n", (double)pclk2 / 1000000.0);
}

void LCD_Init(); // TODO move to h-file
void Pereph_Init() {
    /* Initialize all configured peripherals */
    MX_GPIO_Init();

    MX_FMC_Init();	        // SDRAM 16MB   [V]

    MX_USART1_UART_Init();  // DEBUG UART   [V]
    printf(CLEAR_SCREEN "\nDEBUG UART " TEXT_GREEN "started" TEXT_RESET "\n");
    PrintCpuClock();

    LCD_Init();
    MX_SDMMC2_SD_Init();	// FileSystem   [V] TODO: rework FAT32/PartTable

    // MX_ADC1_Init();
    // MX_ADC3_Init();
    // MX_CRC_Init();

    PrintCpuClock();
    // MX_LTDC_Init();         // display framebufer
    // MX_DMA2D_Init();        // 2D accelerator
    // MX_DSIHOST_DSI_Init();  // Display
    // MX_HDMI_CEC_Init();

    // MX_ETH_Init();	        // Ethernet
    // MX_I2C1_Init();
    // MX_I2C4_Init();	        // Sound bus
    // MX_IWDG_Init();
    // MX_QUADSPI_Init();
    // MX_RTC_Init();
    // MX_SAI2_Init();
    // MX_SPDIFRX_Init();
    // MX_SPI2_Init();
    // MX_TIM1_Init();
    // MX_TIM3_Init();
    // MX_TIM10_Init();
    // MX_TIM11_Init();
    // MX_TIM12_Init();
    // MX_UART5_Init();
    // MX_USART6_UART_Init();
    // MX_USB_OTG_HS_PCD_Init();	// USB host - input
    //  MX_WWDG_Init();

    // printf("Pereph_Init done\n");
}