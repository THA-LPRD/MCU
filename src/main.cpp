#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    Serial.println("Hello, World!");
}

void loop() {
    static int counter = 0;
    Serial.print("Loop counter: ");
    Serial.println(counter++);
    delay(1000);
}
