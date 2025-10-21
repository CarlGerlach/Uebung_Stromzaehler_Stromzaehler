#include <iostream>
#include <windows.h>
#include <ctime>
#include "Serial.h"
#include "Stromzaehler.h"

using namespace std;

int main() {
    // Beispiel-Setup: COM1 (Controller) <-> COM2 (Stromzaehler)
    Serial stromzaehlerSerial("COM2", 115200, 8, ONESTOPBIT, NOPARITY); 


    if (!stromzaehlerSerial.open()) {
        cout << "Konnte stromzaehlerSerial nicht oeffnen." << endl;
        return 1;
    }

    srand((unsigned)time(0));

    Stromzaehler meter(&stromzaehlerSerial);
    meter.runLoop();

    stromzaehlerSerial.close();
    return 0;
}