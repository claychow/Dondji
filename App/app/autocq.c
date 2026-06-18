#ifdef ENABLE_AUTO_CQ

#include "app/autocq.h"

#include <stdbool.h>

#include "app/app.h"
#include "app/chFrScanner.h"
#include "app/fm.h"
#include "app/scanner.h"
#include "audio.h"
#include "driver/bk4819.h"
#include "driver/system.h"
#include "functions.h"
#include "misc.h"
#include "radio.h"
#include "settings.h"
#include "ui/ui.h"

#define AUTOCQ_TONE_HZ      700u
#define AUTOCQ_DIT_MS       70u
#define AUTOCQ_TONE_LEVEL   66u

static const char *AUTOCQ_GetPattern(char c)
{
    switch (c)
    {
        case 'A': return ".-";
        case 'B': return "-...";
        case 'C': return "-.-.";
        case 'D': return "-..";
        case 'E': return ".";
        case 'F': return "..-.";
        case 'G': return "--.";
        case 'H': return "....";
        case 'I': return "..";
        case 'J': return ".---";
        case 'K': return "-.-";
        case 'L': return ".-..";
        case 'M': return "--";
        case 'N': return "-.";
        case 'O': return "---";
        case 'P': return ".--.";
        case 'Q': return "--.-";
        case 'R': return ".-.";
        case 'S': return "...";
        case 'T': return "-";
        case 'U': return "..-";
        case 'V': return "...-";
        case 'W': return ".--";
        case 'X': return "-..-";
        case 'Y': return "-.--";
        case 'Z': return "--..";
        case '0': return "-----";
        case '1': return ".----";
        case '2': return "..---";
        case '3': return "...--";
        case '4': return "....-";
        case '5': return ".....";
        case '6': return "-....";
        case '7': return "--...";
        case '8': return "---..";
        case '9': return "----.";
        default:  return "";
    }
}

static char AUTOCQ_ToUpper(char c)
{
    if (c >= 'a' && c <= 'z')
        return (char)(c - ('a' - 'A'));
    return c;
}

static bool AUTOCQ_LoadCallsign(char *callsign)
{
    bool hasLetter = false;
    bool hasDigit  = false;
    uint8_t len    = 0;

    if (!IS_MR_CHANNEL(gTxVfo->CHANNEL_SAVE))
        return false;

    SETTINGS_FetchChannelName(callsign, gTxVfo->CHANNEL_SAVE);

    for (uint8_t i = 0; callsign[i] != '\0' && i < CHANNEL_NAME_MAX_BYTES; ++i)
    {
        char c = AUTOCQ_ToUpper(callsign[i]);
        if (c >= 'A' && c <= 'Z')
            hasLetter = true;
        else if (c >= '0' && c <= '9')
            hasDigit = true;
        else
            return false;

        callsign[len++] = c;
    }

    callsign[len] = '\0';
    return len >= 3 && len <= 10 && hasLetter && hasDigit &&
        callsign[len - 1] >= 'A' && callsign[len - 1] <= 'Z';
}

static void AUTOCQ_Tone(bool on)
{
    if (on)
        BK4819_ExitTxMute();
    else
        BK4819_EnterTxMute();
}

static void AUTOCQ_SendElement(char element)
{
    AUTOCQ_Tone(true);
    SYSTEM_DelayMs((element == '-') ? (AUTOCQ_DIT_MS * 3u) : AUTOCQ_DIT_MS);
    AUTOCQ_Tone(false);
    SYSTEM_DelayMs(AUTOCQ_DIT_MS);
}

static void AUTOCQ_SendCharacter(char c)
{
    const char *pattern = AUTOCQ_GetPattern(c);
    while (*pattern != '\0')
        AUTOCQ_SendElement(*pattern++);

    SYSTEM_DelayMs(AUTOCQ_DIT_MS * 2u);
}

static bool AUTOCQ_IsBlocked(void)
{
    if (SCANNER_IsScanning() || gScanStateDir != SCAN_OFF)
        return true;

#ifdef ENABLE_FMRADIO
    if (gFmRadioMode || gFM_ScanState != FM_SCAN_OFF)
        return true;
#endif

    return gCurrentFunction == FUNCTION_TRANSMIT || gScreenToDisplay == DISPLAY_MENU;
}

static void AUTOCQ_BeepFail(void)
{
    gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
    gUpdateDisplay = true;
    gUpdateStatus = true;
}

static void AUTOCQ_SendWord(const char *word)
{
    while (*word != '\0')
        AUTOCQ_SendCharacter(*word++);
    SYSTEM_DelayMs(AUTOCQ_DIT_MS * 4u);
}

void AUTOCQ_Action(void)
{
    char callsign[CHANNEL_NAME_MAX_BYTES + 1];

    if (AUTOCQ_IsBlocked())
    {
        AUTOCQ_BeepFail();
        return;
    }

    if (!AUTOCQ_LoadCallsign(callsign))
    {
        AUTOCQ_BeepFail();
        return;
    }

    RADIO_PrepareTX();

    if (gCurrentFunction != FUNCTION_TRANSMIT)
        return;

    BK4819_WriteRegister(
        BK4819_REG_70,
        BK4819_REG_70_MASK_ENABLE_TONE1 |
            (AUTOCQ_TONE_LEVEL << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN));
    BK4819_WriteRegister(BK4819_REG_71, ((uint32_t)AUTOCQ_TONE_HZ * 103244u + 5000u) / 10000u);
    BK4819_SetAF(BK4819_AF_MUTE);
    BK4819_EnableTXLink();
    SYSTEM_DelayMs(50);
    AUTOCQ_Tone(false);

    AUTOCQ_SendWord("CQ");
    AUTOCQ_SendWord("CQ");
    AUTOCQ_SendWord("CQ");
    AUTOCQ_SendWord("DE");
    AUTOCQ_SendWord(callsign);
    AUTOCQ_SendWord(callsign);
    AUTOCQ_SendCharacter('K');

    BK4819_WriteRegister(BK4819_REG_70, 0);
    AUTOCQ_Tone(false);

    APP_EndTransmission();

    if (gEeprom.REPEATER_TAIL_TONE_ELIMINATION == 0)
        FUNCTION_Select(FUNCTION_FOREGROUND);
    else
        gRTTECountdown_10ms = gEeprom.REPEATER_TAIL_TONE_ELIMINATION * 10;

    gFlagEndTransmission = false;
    RADIO_SetVfoState(VFO_STATE_NORMAL);
    gRequestDisplayScreen = DISPLAY_MAIN;
    gUpdateDisplay = true;
    gUpdateStatus = true;
}

#endif
