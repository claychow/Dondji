/* Camp Mode helpers for field-oriented receive presets.
 *
 * The first MVP keeps the UI small: expose airband/marine scans and
 * one-touch save as assignable key actions.
 */

#ifndef APP_CAMP_H
#define APP_CAMP_H

#include <stdbool.h>
#include <stdint.h>

#include "radio.h"

typedef enum {
    CAMP_SERVICE_NONE = 0,
    CAMP_SERVICE_AIR,
    CAMP_SERVICE_MARINE
} CAMP_Service_t;

void CAMP_ActionAirScan(void);
void CAMP_ActionMarineScan(void);
void CAMP_ActionSaveHit(void);

bool CAMP_IsActive(void);
bool CAMP_IsActiveService(CAMP_Service_t service);
void CAMP_Stop(void);
void CAMP_UpdateScanSegment(void);

const char *CAMP_GetActiveLabel(void);
const char *CAMP_GetActiveServiceLabel(void);
ModulationMode_t CAMP_GetActiveModulation(void);

#endif
