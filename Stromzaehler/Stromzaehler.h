// ==============================
// Stromzaehler.h
// ------------------------------
// Simulierter Stromzaehler (Server-Seite) fuer Tests mit dem Controller.
// Protokoll:
// Anfrage vom Controller: SOH "READ" STX <nr> ETX '\n'
// Antwort vom Stromzaehler: STX v1;v2;...;v24;P ETX '\n'
// - v1..v24 sind einstellig (0..9) und stellen Stundenwerte dar
// - P ist die Pruefziffer (25. Element = summe%10 nach Regel der Aufgabe)
//
// Hinweis: Die Klasse kann absichtlich Fehler injizieren (kein STX, kein ETX,
// falsche Pruefziffer), um die Fehlerbehandlung des Controllers zu testen.
// ==============================
#pragma once
#include <string>
#include "Serial.h"


using namespace std;


class Stromzaehler {
public:
	// Steuerzeichen gem. ASCII
	static const unsigned char SOH = 0x01; // Start of Heading
	static const unsigned char STX = 0x02; // Start of Text
	static const unsigned char ETX = 0x03; // End of Text


	// Konstruktor: bekommt eine bereits konfigurierte serielle Schnittstelle
	// uebergeben (wird in der main geoeffnet/geschlossen).
	Stromzaehler(Serial*);


	// Startet eine Schleife. Blockiert, liest Anfragen und sendet Antworten.
	// Erwartet oben beschriebenes Protokoll.
	void runLoop();


	// Hilfsfunktion: macht Steuerzeichen in einem String sichtbar, z. B.
	// "<1>READ<2>1<3>" statt unsichtbarer Bytes 0x01/0x02/0x03.
	string mitSteuerzeichen(const string& s);


private:
	Serial* stromzaehlerSerial; // nicht-besitzend; wird in der main verwaltet
};