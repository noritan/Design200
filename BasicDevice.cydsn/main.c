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
#define RGB_CHAR_HANDLE         (0x000E)

//**********************************************************************
//  Variable Declarations
//**********************************************************************
// 'connectionHandle' stores connection parameters
CYBLE_CONN_HANDLE_T  connectionHandle;

// This flag is used by application to know whether a Central 
// device has been connected. This is updated in BLE event callback 
// function
uint8 deviceConnected = 0;

// Variable to store the present RGB LED control data.
struct GattAttribute {
    uint8               dirty;
    CYBLE_GATT_VALUE_T  value;
}   rgbDescriptor;

void initializeRgbDescriptor(void) {
    static uint8        rgbValue[1];
    
    rgbValue[0] = 0;
    rgbDescriptor.value.len = 1;
    rgbDescriptor.value.val = rgbValue;
    rgbDescriptor.dirty = 1;
}

void queueRgbDescriptor(CYBLE_GATT_VALUE_T *value) {
    rgbDescriptor.value.len = value->len;
    memcpy(rgbDescriptor.value.val, value->val, value->len);
    rgbDescriptor.dirty = 1;
}

void updateRgbDescriptor(void) {
    // Handle value to update the descriptor
    CYBLE_GATT_HANDLE_VALUE_PAIR_T  handleValuePair;

    // Update descriptor handle with data
    handleValuePair.attrHandle = RGB_CHAR_HANDLE;
    handleValuePair.value = rgbDescriptor.value;
    
    // Send updated RGB control handle as attribute for read
    // by central device
    CyBle_GattsWriteAttributeValue(
        &handleValuePair, 0,
        &connectionHandle, CYBLE_GATT_DB_LOCALLY_INITIATED
    );
    
    // Update the LED color
    LED_Green_Write(!(rgbDescriptor.value.val[0] & 4));
    LED_Red_Write(  !(rgbDescriptor.value.val[0] & 2));
    LED_Blue_Write( !(rgbDescriptor.value.val[0] & 1));
    
    // Clear dirty flag
    rgbDescriptor.dirty = 0;
}

void StackEventHandler(uint32 event, void *eventParam) {
    switch (event) {
        //======================================================
        // Mandatory events to be handled
        //======================================================
        case CYBLE_EVT_STACK_ON:
            // Start BLE advertisement for 30 seconds 
            CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);

            // Initialize descriptor
            initializeRgbDescriptor();

            break;

        //======================================================
        //  GAP Events
        //======================================================
        case CYBLE_EVT_GAP_DEVICE_CONNECTED:
            // BLE link is established
            break;

        case CYBLE_EVT_GAP_DEVICE_DISCONNECTED:
            // Start BLE advertisement for 30 seconds 
            CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
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
        
        case CYBLE_EVT_GATTS_WRITE_REQ:
            // This event is received when Central device sends
            {
                CYBLE_GATTS_WRITE_REQ_PARAM_T *writeReqParam;

                // a Write command on an Attribute
                writeReqParam =
                    (CYBLE_GATTS_WRITE_REQ_PARAM_T *) eventParam;

                // When this event is triggered, the peripheral has
                // received a write command on the custom characteristic
                // Check if the returned handle is matching to
                // RGB Control Write Attribute
                if (writeReqParam->handleValPair.attrHandle
                    == RGB_CHAR_HANDLE
                ) {
                    queueRgbDescriptor(
                        &(writeReqParam->handleValPair.value)
                    );
                }

                // Send the response to the write request received.
                CyBle_GattsWriteRsp(connectionHandle);
            }
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
        
        // Scan update queue
        if (rgbDescriptor.dirty) {
            // Update RGB Descriptor
            updateRgbDescriptor();
        }

        // Enter to deep sleep mode
        {
            CYBLE_LP_MODE_T state;

            state = CyBle_EnterLPM(CYBLE_BLESS_DEEPSLEEP);
            if (state == CYBLE_BLESS_DEEPSLEEP) {
                CySysPmDeepSleep();
            }
        }
    }
}

/* [] END OF FILE */
