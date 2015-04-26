/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include <project.h>

void StackEventHandler(uint32 event, void *eventParam) {
    switch (event) {
        // Mandatory events to be handled
        case CYBLE_EVT_STACK_ON:
        case CYBLE_EVT_GAP_DEVICE_DISCONNECTED:
            // Start BLE advertisement for 30 seconds 
            CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
            break;

        default:
            break;
    }
}

int main() {
    CYBLE_API_RESULT_T apiResult;

    // Enable global interrupts
    CyGlobalIntEnable;

    // Initialize the BLE device.
    apiResult = CyBle_Start(StackEventHandler);
    // Validate BLE stack initialization successed
    CYASSERT(apiResult == CYBLE_ERROR_OK);

    for(;;){
        // Service all the BLE stack events.
        // Must be called at least once in a BLE connection interval
        CyBle_ProcessEvents();
    }
}

/* [] END OF FILE */
