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
#define COUNTER_CCC_HANDLE      (0x0010)

//**********************************************************************
//  Variable Declarations
//**********************************************************************
// 'connectionHandle' stores connection parameters
CYBLE_CONN_HANDLE_T  connectionHandle;

// This flag is used by application to know whether a Central 
// device has been connected. This is updated in BLE event callback 
// function
uint8 deviceConnected = 0;

// Variable to store the present Counter CCC data.
struct GattAttribute {
    uint8               dirty;
    CYBLE_GATT_VALUE_T  value;
}   counterCccDescriptor;

// This flag is set when the Central device writes to
// CCC (Client Characteristic Configuration) of the Counter
// Characteristic to enable notifications
uint8 enableCounterNotification = 0;

void initializeCounterCccDescriptor(void) {
    static uint8        cccValue[2];
    
    cccValue[0] = 0;
    cccValue[1] = 0;
    counterCccDescriptor.value.len = 2;
    counterCccDescriptor.value.val = cccValue;
    counterCccDescriptor.dirty = 1;
}

void queueCounterCccDescriptor(CYBLE_GATT_VALUE_T *value) {
    counterCccDescriptor.value.len = value->len;
    memcpy(counterCccDescriptor.value.val, value->val, value->len);
    counterCccDescriptor.dirty = 1;
}

void updateCounterCccDescriptor(void) {
    // Handle value to update the CCCD
    CYBLE_GATT_HANDLE_VALUE_PAIR_T  handleValuePair;

    // Update CCCD handle with notification status data
    handleValuePair.attrHandle = COUNTER_CCC_HANDLE;
    handleValuePair.value = counterCccDescriptor.value;
    
    // Send updated RGB control handle as attribute for read
    // by central device
    CyBle_GattsWriteAttributeValue(
        &handleValuePair, 0,
        &connectionHandle, CYBLE_GATT_DB_LOCALLY_INITIATED
    );
    
    // Get notification flag
    enableCounterNotification = counterCccDescriptor.value.val[0] & 1;
    
    // Clear dirty flag
    counterCccDescriptor.dirty = 0;
}

void StackEventHandler(uint32 event, void *eventParam) {
    switch (event) {
        //======================================================
        // Mandatory events to be handled
        //======================================================
        case CYBLE_EVT_STACK_ON:
            // Disable CCCD notification
            initializeCounterCccDescriptor();

            // Start BLE advertisement for 30 seconds 
            CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
            break;

        //======================================================
        //  GAP Events
        //======================================================
        case CYBLE_EVT_GAP_DEVICE_CONNECTED:
            // BLE link is established
            break;

        case CYBLE_EVT_GAP_DEVICE_DISCONNECTED:
            // Disable CCCD notification
            initializeCounterCccDescriptor();
        
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
                // Counter CCCD Write Attribute
                if (writeReqParam->handleValPair.attrHandle
                    == COUNTER_CCC_HANDLE
                ) {
                    queueCounterCccDescriptor(
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

void sendCounterNotification(uint32 data) {
	// 'notificationHandle' stores Counter notification data parameters
	CYBLE_GATTS_HANDLE_VALUE_NTF_T  handleValueNotify;	
	
	// Update notification handle with Counter  data
	handleValueNotify.attrHandle = COUNTER_CHAR_HANDLE;
	handleValueNotify.value.val = (uint8*)&data;
	handleValueNotify.value.len = sizeof data;
	
	// Send the updated handle as part of attribute for notifications
	CyBle_GattsNotification(connectionHandle, &handleValueNotify);
}

int main() {
    CYBLE_API_RESULT_T apiResult;
    uint32 count = 0;
    uint8   triggerNotification = 0;

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
            if (counterCccDescriptor.dirty) {
                // Update Counter CCCD
                updateCounterCccDescriptor();
            } else if (triggerNotification) {
                // Send notification if required
                if (enableCounterNotification) {
                    sendCounterNotification(count);
                }
                triggerNotification = 0;
            } else {
                // Update counter value
                updateCounter(count);
                count++;
                triggerNotification = ((count & 0x000000FF) == 0);
            }
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
