#include <ArduinoBLE.h>
#include <LSM6DS3.h>

#include "watchdog.h"

#include "bluetooth_uuids.h"

#define WATCHDOG_TIMEOUT 5000

// Talk to the device's IMU over I2C.
LSM6DS3 imu(I2C_MODE, 0x6a);

typedef struct vec3 {
    float x;
    float y;
    float z;
} vec3_t;

// Make sure the vec3 is represented packed.
static_assert(sizeof(vec3_t) == 4 * 3);

// UUID for the KitBit BLE service.
BLEService ble_service(BLE_SERVICE_UUID);

// UUID for the KitBit AccelX characteristic.
BLECharacteristic ble_accel_characteristic(BLE_CHAR_ACCEL_UUID, BLERead | BLENotify,
                                           sizeof(vec3_t));
BLECharacteristic ble_gyro_characteristic(BLE_CHAR_GYRO_UUID, BLERead | BLENotify, sizeof(vec3_t));
BLECharacteristic ble_temp_characteristic(BLE_CHAR_TEMP_UUID, BLERead | BLENotify, sizeof(float));

vec3_t accel;
vec3_t gyro;
float temp;

unsigned long num_readings;

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.begin(9600);
    Serial.println("=== KitBit ===");

#ifdef WATCHDOG_TIMEOUT
    Watchdog.enable(WATCHDOG_TIMEOUT);
    Serial.println("[+] Watchdog timer set.");
    Serial.print("    Resetting after ");
    Serial.print(WATCHDOG_TIMEOUT);
    Serial.print("ms without activity.");
    Serial.println();
#endif

    // Initialize the IMU.
    if (imu.begin() == 0) {
        Serial.println("[+] IMU initialized.");
    } else {
        Serial.println("[!] Failed to initialize IMU.");
        while (true) {
        }
    }

    // Initialize bluetooth.
    if (BLE.begin()) {
        Serial.println("[+] Bluetooth LE initialized.");
        BLE.setLocalName("KitBit");
        BLE.setDeviceName("KitBit Device");
        BLE.setAdvertisedService(ble_service);

        ble_service.addCharacteristic(ble_accel_characteristic);
        ble_service.addCharacteristic(ble_gyro_characteristic);
        ble_service.addCharacteristic(ble_temp_characteristic);

        BLE.addService(ble_service);
        BLE.advertise();
        Serial.println("    Advertising KitBit and listening for connections.");
    } else {
        Serial.println("[!] Failed to initialize Bluetooth LE.");
    }

    digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
    static int tick = 0;
    int millis_at_start_of_tick = millis();
    tick++;

    // Update BLE devices.
    BLEDevice central = BLE.central();
    if (central) {
        digitalWrite(LEDB, HIGH);

        Serial.print("ble: connected to central mac=");
        Serial.print(central.address());
        Serial.println();

        int updates = 0;

        while (central.connected()) {
            // Reset the watchdog while a device is connected.
            Watchdog.reset();

            // Poll the sensor data and update characteristics.
            poll_sensors();

            // Pause briefly to allow requests from the central. If we don't do this, the central
            // will fail to make new subscriptions.
            delay(1);

            // Blink the LED periodically while we transmit data.
            digitalWrite(LEDG, millis() % 1000 < 20 ? LOW : HIGH);
        }

        // Turn off the LED.
        digitalWrite(LEDG, HIGH);

        Serial.println("ble: disconnected");
    }

    // Reset the watchdog between connections
    Watchdog.reset();

    // Blink the LED
    digitalWrite(LEDB, LOW);
    delay(10);
    digitalWrite(LEDB, HIGH);

    // Delay between Bluetooth connections.
    int delay_between_connections = 2000;
    int already_elapsed_this_tick = millis() - millis_at_start_of_tick;
    delay(delay_between_connections - already_elapsed_this_tick);

    // Reset the watchdog after we wake from sleep
    Watchdog.reset();
}

void poll_sensors() {
    // Read accelerometer
    vec3_t accel = {
        .x = imu.readFloatAccelX(),
        .y = imu.readFloatAccelY(),
        .z = imu.readFloatAccelZ(),
    };
    ble_accel_characteristic.writeValue((void*)&accel, sizeof(vec3_t), false);

    // Read gyroscope
    vec3_t gyro = {
        .x = imu.readFloatGyroX(),
        .y = imu.readFloatGyroY(),
        .z = imu.readFloatGyroZ(),
    };
    ble_gyro_characteristic.writeValue((void*)&gyro, sizeof(vec3_t), false);

    // Read temperature
    float temp = imu.readTempC();
    ble_temp_characteristic.writeValue((void*)&temp, sizeof(float), false);
}
