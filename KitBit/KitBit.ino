#include <Adafruit_SleepyDog.h>
#include <ArduinoBLE.h>
#include <Arduino_LSM6DSOX.h>
#include <Scheduler.h>
#include <WiFiNINA.h>

#include "bluetooth_uuids.h"

#define WATCHDOG_TIMEOUT 2000

#define WITHIN_EPSILON(a, b) (fabs((a) - (b)) < 0.001)

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
    digitalWrite(LED_BUILTIN, HIGH);

    /*
    // Wait for serial port availability.
    while (!Serial) {
    }
    Serial.begin(9600);
    Serial.println("=== KitBit ===");
    */

    // Set up the watchdog timer.
#ifdef WATCHDOG_TIMEOUT
    Watchdog.enable(2000);
    Serial.println("[+] Watchdog timer set.");
    Serial.print("    Resetting after ");
    Serial.print(WATCHDOG_TIMEOUT);
    Serial.print("ms with no sensor readings.");
    Serial.println();
#endif

    // Initialize the IMU.
    if (IMU.begin()) {
        Serial.println("[+] IMU initialized.");
        Serial.print("    Accelerometer Sample rate = ");
        Serial.print(IMU.accelerationSampleRate());
        Serial.println(" Hz");
        Serial.print("    Gyroscope Sample rate = ");
        Serial.print(IMU.gyroscopeSampleRate());
        Serial.println(" Hz");
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

    // Start the sensor-polling task.
    Scheduler.startLoop(handle_ble_connection);
}

void loop() {
    poll_sensors();
}

void handle_ble_connection() {
    static int tick = 0;
    tick++;

    // Update BLE devices.
    BLEDevice central = BLE.central();
    if (central) {
        // Serial.print("ble: connected to central mac=");
        // Serial.print(central.address());
        // Serial.println();

        // Track the last reading we've sent to the central.
        unsigned int last_reading_number = 0;

        while (central.connected()) {

            // Don't update characteristics if there is no new data.
            if (num_readings == last_reading_number) {
                delay(1);
                yield();
            }

            // Update sensor characteristics.
            ble_accel_characteristic.writeValue((void*)&accel, sizeof(vec3_t), false);
            ble_gyro_characteristic.writeValue((void*)&gyro, sizeof(vec3_t), false);
            ble_temp_characteristic.writeValue((void*)&temp, sizeof(float), false);

            last_reading_number = num_readings;
        }

        // Serial.println("ble: disconnected");
    }

    yield();
}

void poll_sensors() {
    bool did_read = false;

    while (IMU.accelerationAvailable()) {
        IMU.readAcceleration(accel.x, accel.y, accel.z);
        did_read = true;
    }

    while (IMU.gyroscopeAvailable()) {
        IMU.readGyroscope(gyro.x, gyro.y, gyro.z);
        did_read = true;
    }

    while (IMU.temperatureAvailable()) {
        IMU.readTemperatureFloat(temp);
        did_read = true;
    }

    // Reset the watchdog timer.
    if (did_read) {
        Watchdog.reset();
        num_readings++;
    }

    // Blink the LED as readings are taken.
    digitalWrite(LED_BUILTIN, num_readings % 64 < 32 ? HIGH : LOW);

    delay(2);
    yield();
}
