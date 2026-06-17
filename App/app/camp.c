#ifdef ENABLE_CAMP_MODE

#include "app/camp.h"

#include <stddef.h>
#include <string.h>

#include "app/chFrScanner.h"
#include "app/scanner.h"
#include "audio.h"
#include "frequencies.h"
#include "functions.h"
#include "misc.h"
#include "settings.h"
#include "ui/main.h"
#include "ui/ui.h"

typedef struct {
    const char      *label;
    uint32_t         start;
    uint32_t         stop;
    STEP_Setting_t   step;
    ModulationMode_t modulation;
    uint8_t          bandwidth;
} CAMP_Segment_t;

static const CAMP_Segment_t campAirSegments[] = {
    {"AIR GND", 12160000, 12195000, STEP_25kHz, MODULATION_AM, BANDWIDTH_NARROW},
    {"AIR LOW", 11800000, 12300000, STEP_25kHz, MODULATION_AM, BANDWIDTH_NARROW},
    {"AIR MID", 12300000, 12900000, STEP_25kHz, MODULATION_AM, BANDWIDTH_NARROW},
    {"AIR HI",  12900000, 13697500, STEP_25kHz, MODULATION_AM, BANDWIDTH_NARROW},
    {"AIR ALL", 11800000, 13697500, STEP_25kHz, MODULATION_AM, BANDWIDTH_NARROW},
    {"GUARD",   12150000, 12150000, STEP_25kHz, MODULATION_AM, BANDWIDTH_NARROW},
};

static const CAMP_Segment_t campMarineSegments[] = {
    {"SEA 16",  15680000, 15680000, STEP_25kHz, MODULATION_FM, BANDWIDTH_WIDE},
    {"SEA SHP", 15600000, 15742500, STEP_25kHz, MODULATION_FM, BANDWIDTH_WIDE},
    {"SEA CST", 16060000, 16195000, STEP_25kHz, MODULATION_FM, BANDWIDTH_WIDE},
    {"SEA ALL", 15600000, 16195000, STEP_25kHz, MODULATION_FM, BANDWIDTH_WIDE},
};

static CAMP_Service_t campActiveService = CAMP_SERVICE_NONE;
static uint8_t        campAirSegmentIndex;
static uint8_t        campMarineSegmentIndex;
static const CAMP_Segment_t *campActiveSegment;

static const CAMP_Segment_t *CAMP_GetSegment(CAMP_Service_t service, uint8_t index)
{
    if (service == CAMP_SERVICE_AIR)
        return &campAirSegments[index % ARRAY_SIZE(campAirSegments)];
    if (service == CAMP_SERVICE_MARINE)
        return &campMarineSegments[index % ARRAY_SIZE(campMarineSegments)];
    return NULL;
}

static uint8_t *CAMP_GetSegmentIndex(CAMP_Service_t service)
{
    if (service == CAMP_SERVICE_AIR)
        return &campAirSegmentIndex;
    if (service == CAMP_SERVICE_MARINE)
        return &campMarineSegmentIndex;
    return NULL;
}

static void CAMP_ApplySegmentToVfo(VFO_Info_t *vfo, const CAMP_Segment_t *segment)
{
    if (vfo == NULL || segment == NULL)
        return;

    RADIO_InitInfo(vfo, FREQ_CHANNEL_FIRST + FREQUENCY_GetBand(segment->start), segment->start);

    vfo->freq_config_RX.Frequency = segment->start;
    vfo->freq_config_TX.Frequency = segment->start;
    vfo->pRX                      = &vfo->freq_config_RX;
    vfo->pTX                      = &vfo->freq_config_TX;
    vfo->Modulation               = segment->modulation;
    vfo->CHANNEL_BANDWIDTH        = segment->bandwidth;
    vfo->STEP_SETTING             = segment->step;
    vfo->StepFrequency            = gStepFrequencyTable[segment->step];
    vfo->TX_LOCK                  = true;
    vfo->OUTPUT_POWER             = OUTPUT_POWER_LOW1;
    vfo->TX_OFFSET_FREQUENCY      = 0;
    vfo->TX_OFFSET_FREQUENCY_DIRECTION = TX_OFFSET_FREQUENCY_DIRECTION_OFF;
    vfo->freq_config_RX.CodeType  = CODE_TYPE_OFF;
    vfo->freq_config_TX.CodeType  = CODE_TYPE_OFF;
    vfo->freq_config_RX.Code      = 0;
    vfo->freq_config_TX.Code      = 0;
    vfo->SCANLIST_PARTICIPATION   = MR_CHANNELS_LIST + 1;

    RADIO_ApplyOffset(vfo);
    RADIO_ConfigureSquelchAndOutputPower(vfo);
}

static void CAMP_StartSegment(CAMP_Service_t service, const CAMP_Segment_t *segment)
{
    if (segment == NULL)
        return;

    if (SCANNER_IsScanning())
        SCANNER_Stop();

    if (gScanStateDir != SCAN_OFF)
        CHFRSCANNER_Stop();

    campActiveService = service;
    campActiveSegment = segment;

    gMonitor = false;
    gEeprom.CROSS_BAND_RX_TX = CROSS_BAND_OFF;
    gEeprom.RX_VFO = gEeprom.TX_VFO;
    gRxVfoIsActive = true;
    RADIO_SelectVfos();
    CAMP_ApplySegmentToVfo(gRxVfo, segment);

    gScanRangeStart = 0;
    gScanRangeStop  = 0;

    if (segment->start == segment->stop) {
        RADIO_SetupRegisters(true);
        gScanKeepResult = true;
        GUI_SelectNextDisplay(DISPLAY_MAIN);
    } else {
        gScanRangeStart = segment->start;
        gScanRangeStop  = segment->stop;
        gRxVfo->freq_config_RX.Frequency = segment->stop;
        RADIO_ApplyOffset(gRxVfo);
        CHFRSCANNER_Start(true, SCAN_FWD);
        initialFrqOrChan = segment->start;
        lastFoundFrqOrChan = segment->start;
    }

#ifdef ENABLE_FEAT_F4HWN_RESUME_STATE
    gEeprom.CURRENT_STATE = gScanRangeStart ? 2 : 0;
    SETTINGS_WriteCurrentState();
#endif

#ifdef ENABLE_VOICE
    AUDIO_SetVoiceID(0, VOICE_ID_SCANNING_BEGIN);
    AUDIO_PlaySingleVoice(true);
#endif

    gVFO_RSSI_bar_level[(gEeprom.RX_VFO + 1) & 1U] = 0;
    gDualWatchActive = false;
    gUpdateDisplay   = true;
    gUpdateStatus    = true;
}

static void CAMP_ActionScan(CAMP_Service_t service)
{
    uint8_t *index = CAMP_GetSegmentIndex(service);
    if (index == NULL)
        return;

    CAMP_StartSegment(service, CAMP_GetSegment(service, *index));

    if (campActiveService != service)
        return;

    *index = (uint8_t)(*index + 1);
}

static uint16_t CAMP_FindFirstFreeChannel(void)
{
    for (uint16_t channel = MR_CHANNEL_FIRST; IS_MR_CHANNEL(channel); channel++)
        if (!RADIO_CheckValidChannel(channel, false, 0))
            return channel;

    return 0xFFFFu;
}

static void CAMP_CopyCurrentHitToVfo(VFO_Info_t *dst)
{
    const uint32_t frequency = (gScanStateDir != SCAN_OFF && gScanKeepResult)
        ? lastFoundFrqOrChan
        : gRxVfo->pRX->Frequency;

    const CAMP_Segment_t *segment = campActiveSegment;

    if (segment == NULL)
        segment = (campActiveService == CAMP_SERVICE_MARINE) ? &campMarineSegments[0] : &campAirSegments[0];

    CAMP_ApplySegmentToVfo(dst, segment);

    dst->freq_config_RX.Frequency = frequency;
    dst->freq_config_TX.Frequency = frequency;
    dst->Band                     = FREQUENCY_GetBand(frequency);
    dst->CHANNEL_SAVE             = FREQ_CHANNEL_FIRST + dst->Band;
    dst->pRX                      = &dst->freq_config_RX;
    dst->pTX                      = &dst->freq_config_TX;
    RADIO_ApplyOffset(dst);
    RADIO_ConfigureSquelchAndOutputPower(dst);

    if (segment->label[0] != '\0')
        strncpy(dst->Name, segment->label, sizeof(dst->Name) - 1);
    else
        strncpy(dst->Name, "CAMP", sizeof(dst->Name) - 1);
    dst->Name[sizeof(dst->Name) - 1] = '\0';
}

void CAMP_ActionAirScan(void)
{
    CAMP_ActionScan(CAMP_SERVICE_AIR);
}

void CAMP_ActionMarineScan(void)
{
    CAMP_ActionScan(CAMP_SERVICE_MARINE);
}

void CAMP_ActionSaveHit(void)
{
    if (!CAMP_IsActive())
    {
        gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
        return;
    }

    if (gScanStateDir != SCAN_OFF && !gScanKeepResult)
    {
        gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
        return;
    }

    const uint16_t channel = CAMP_FindFirstFreeChannel();
    if (!IS_MR_CHANNEL(channel))
    {
        gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
        return;
    }

    VFO_Info_t vfo;
    CAMP_CopyCurrentHitToVfo(&vfo);

    vfo.CHANNEL_SAVE = channel;
    SETTINGS_SaveChannel(channel, gEeprom.RX_VFO, &vfo, 3);

    gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;
    gUpdateDisplay = true;
    gUpdateStatus = true;
}

bool CAMP_IsActive(void)
{
    return campActiveService != CAMP_SERVICE_NONE;
}

void CAMP_Stop(void)
{
    if (campActiveService != CAMP_SERVICE_NONE)
    {
        gScanRangeStart = 0;
        gScanRangeStop  = 0;
    }

    campActiveService = CAMP_SERVICE_NONE;
    campActiveSegment = NULL;
}

void CAMP_UpdateScanSegment(void)
{
    if (campActiveSegment == NULL || gScanStateDir == SCAN_OFF || gRxVfo == NULL)
        return;

    gRxVfo->Modulation        = campActiveSegment->modulation;
    gRxVfo->CHANNEL_BANDWIDTH = campActiveSegment->bandwidth;
    gRxVfo->STEP_SETTING      = campActiveSegment->step;
    gRxVfo->StepFrequency     = gStepFrequencyTable[campActiveSegment->step];
    gRxVfo->TX_LOCK           = true;
}

const char *CAMP_GetActiveLabel(void)
{
    return campActiveSegment ? campActiveSegment->label : "";
}

#endif
