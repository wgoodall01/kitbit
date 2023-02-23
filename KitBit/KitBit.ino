#include <Arduino_LSM6DSOX.h>

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    // Wait for serial port availability.
    while (!Serial) {
    }
    Serial.println("=== KitBit ===");

    // Wait for IMU availability.
    if (!IMU.begin()) {
        Serial.println("[!] Failed to initialize IMU!");
        while (true) {
        }
    } else {
        Serial.println("[+] IMU initialized.");
        Serial.print("    Accelerometer Sample rate = ");
        Serial.print(IMU.accelerationSampleRate());
        Serial.println(" Hz");
        Serial.print("    Gyroscope Sample rate = ");
        Serial.print(IMU.gyroscopeSampleRate());
        Serial.println(" Hz");
    }
}

void loop() {
    static int tick = 0;
    tick++;

    static int accel_readings = 0;
    static int gyro_readings = 0;

    static float accel_x = 0, accel_y = 0, accel_z = 0;
    if (IMU.accelerationAvailable()) {
        float x, y, z;
        IMU.readAcceleration(x, y, z);
        accel_x += x;
        accel_y += y;
        accel_z += z;

        accel_readings++;
    }

    static float gyro_x = 0, gyro_y = 0, gyro_z = 0;
    if (IMU.gyroscopeAvailable()) {
        float x, y, z;
        IMU.readGyroscope(x, y, z);
        gyro_x += x;
        gyro_y += y;
        gyro_z += z;

        gyro_readings++;
    }

    static float temperature;
    if (IMU.temperatureAvailable()) {
        IMU.readTemperatureFloat(temperature);
    }

    if (tick % 64 == 0) {
        Serial.print("Accel\t\t");
        Serial.print("x:");
        Serial.print(accel_x / accel_readings);
        Serial.print("\ty:");
        Serial.print(accel_y / accel_readings);
        Serial.print("\tz:");
        Serial.print(accel_z / accel_readings);
        Serial.print("\t\t\tGyro \t");
        Serial.print("x:");
        Serial.print(gyro_x / gyro_readings);
        Serial.print("\ty:");
        Serial.print(gyro_y / gyro_readings);
        Serial.print("\tz:");
        Serial.print(gyro_z / gyro_readings);
        Serial.print("\t\t\tTemp \t");
        Serial.print(temperature);
        Serial.println();

        gyro_x = 0;
        gyro_y = 0;
        gyro_z = 0;
        gyro_readings = 0;
        accel_x = 0;
        accel_y = 0;
        accel_z = 0;
        accel_readings = 0;

        static bool led = false;
        led = !led;
        digitalWrite(LED_BUILTIN, led ? HIGH : LOW);
    }
}
