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

//**********************************************************************
//  Constant Declarations
//**********************************************************************
#define COUNTER_CHAR_HANDLE     (0x000E)

//**********************************************************************
//  Variable Declarations
//**********************************************************************
// 'connectionHandle' stores connection parameters
CYBLE_CONN_HANDLE_T  connectionHandle;

// This flag is used by application to know whether a Central 
// device has been connected. This is updated in BLE event callback 
// function
uint8 deviceConnected = 0;

void StackEventHandler(uint32 event, void *eventParam) {
    switch (event) {
        //======================================================
        // Mandatory events to be handled
        //======================================================
        case CYBLE_EVT_STACK_ON:
            // Start BLE advertisement for 30 seconds 
            CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
            break;

        //======================================================
        //  GAP Events
        //======================================================
        case CYBLE_EVT_GAP_DEVICE_CONNECTED:
            // BLE link is established
            break;

        //======================================================
        //  GATT Events
        //======================================================
        case CYBLE_EVT_GATT_CONNECT_IND:
            // This event is received when device is connected
            // over GATT level
			
            // Update attribute handle on GATT Connection
            connectionHandle = *(CYBLE_CONN_HANDLE_T  *)eventParam;
			
            // This flag is used in application
            // to check connection status
			deviceConnected = 1;
			
            break;
        
        case CYBLE_EVT_GATT_DISCONNECT_IND:
            // This event is received when device is disconnected
			
            // Update deviceConnected flag
            deviceConnected = 0;
			
            break;
            
        default:
            break;
    }
}

void updateCounter(uint32 data) {
    // Handle value to update the characteristic
    CYBLE_GATT_HANDLE_VALUE_PAIR_T handleValuePair;
    		
    // Update descriptor handle with data
    handleValuePair.attrHandle = COUNTER_CHAR_HANDLE;
    handleValuePair.value.val = (uint8*)&data;
    handleValuePair.value.len = sizeof data;

    // Report data to BLE component for sending data
    // when read by Central device
    CyBle_GattsWriteAttributeValue(
        &handleValuePair, 0,
        &connectionHandle, CYBLE_GATT_DB_LOCALLY_INITIATED
    );
}

int main() {
    CYBLE_API_RESULT_T apiResult;
    uint32 count = 0;

    // Enable global interrupts
    CyGlobalIntEnable;

    // Initialize the BLE device.
    apiResult = CyBle_Start(StackEventHandler);
    // Validate BLE stack initialization successed
    CYASSERT(apiResult == CYBLE_ERROR_OK);

    for (;;) {
        // Service all the BLE stack events.
        // Must be called at least once in a BLE connection interval
        CyBle_ProcessEvents();

        if (deviceConnected) {
            // Update counter value
            updateCounter(count);
        }
        count++;
    }
}

/* [] END OF FILE */
