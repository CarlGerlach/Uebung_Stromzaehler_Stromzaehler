#include "Stromzaehler.h"
#include <iostream>
#include <ctime>
#include <cstdlib>

using namespace std;

Stromzaehler::Stromzaehler(Serial* com)
{
	// Merke Zeiger auf die serielle Schnittstelle (kein Besitz)
	stromzaehlerSerial = com;
}


// Erzeuge fuer jede Zaehler-Nr. ein einfaches Profil (0..9, einstellig)
// Idee: Nachts klein, morgens ansteigend, am Abend leicht erhoeht.
static void profil24(int nr, int out24[24]) {
	int base[24] = {
	1,1,1,1,1, // 00-04 Uhr
	2,3,4,5, // 05-08 Uhr
	4,3,3,3,3, // 09-13 Uhr
	4,6, // 14-15 Uhr
	9,8,7,5, // 16-19 Uhr (Abendspitze)
	4,3,2,1 // 20-23 Uhr
	};
	int offset = nr % 3; // kleine Variation je Zaehler, abhaengig von nr
	for (int i = 0; i < 24; ++i) 
	{
		int v = base[i];
		if (i >= 6 && i <= 8) {
			v = v + offset; // morgens minimal hoeher
		}
		if (i >= 16 && i <= 20) {
			v = v + 1 + offset; // abends etwas hoeher
		}
		if ((i + nr) % 5 == 0) {
			v = v + 1; // leichte periodische Schwankung
		}
		// Werte auf 0..9 begrenzen (einstellig fuer die Aufgabe)
		if (v < 0) v = 0;
		if (v > 9) v = 9;
		out24[i] = v;
	}
}



// Pruefziffer nach Aufgabenregel: summe%10, wobei die Indizes ab 0 zaehlen.
static int pruefziffer24_array(const int a[24]) 
{
	int summe = 0;

	for (int i = 0; i < 24; ++i) 
	{
		if (i % 2 == 0) 
		{
			summe = summe + a[i] * 2; // gerade Indizes: *2
		}
		else 
		{
			summe = summe + a[i] * 3; // ungerade Indizes: *3
		}
	}
	int p = summe % 10; // P ist einstellig (0..9)
	return p;
}


void Stromzaehler::runLoop() {
	cout << "Stromzaehler laeuft." << endl;


	// Zufall initialisieren (einmal genuegt)
	srand((unsigned)time(0));


	while (true) {
		// 1) Anfrage vom Controller lesen (endet mit '\n')
		string req = stromzaehlerSerial->readLine();
		if (req.size() == 0) {
			cout << "Ende." << endl;
			return; // Port geschlossen / keine Daten mehr
		}

		cout << "Empfange Anfrage vom Controller " << Stromzaehler::mitSteuerzeichen(req) << endl;

		// 2) STX/ETX in der Anfrage suchen und die Zaehlernummer extrahieren
		int posSTX = -1;
		int posETX = -1;
		for (int i = 0; i < (int)req.size(); ++i) {
			if (req[i] == (char)STX) { posSTX = i; break; }
		}
		if (posSTX == -1) continue; // ungueltig
		for (int i = posSTX + 1; i < (int)req.size(); ++i) {
			if (req[i] == (char)ETX) { posETX = i; break; }
		}
		if (posETX == -1) continue; // ungueltig

		cout << "ZaehlerNr wurde extrahiert" << endl;

		// Zeichen zwischen STX und ETX enthalten hier die Nummer (ASCII-Ziffern)
		string nrStr = req.substr(posSTX + 1, posETX - (posSTX + 1));
		int nr = 0;
		for (int i = 0; i < (int)nrStr.size(); ++i) {
			char c = nrStr[i];
			if (c >= '0' && c <= '9') { nr = nr * 10 + (c - '0'); }
		}

		cout << "ZaehlerNr ist: " << nrStr << endl;

		// 3) 24 Werte erzeugen und lokale Pruefziffer berechnen
		int arr[24];
		profil24(nr, arr);
		int p = pruefziffer24_array(arr);

		cout << "Pruefziffer:" << p << endl;


		// 4) Zufallsfehler waehlen (je ~10 %):
		// - kein ETX
		// - kein STX
		// - falsche Pruefziffer
		int r = rand() % 10; // 0..9
		bool fehlerKeinETX = false;
		bool fehlerKeinSTX = false;
		bool fehlerFalscheP = false;
		if (r == 0) fehlerKeinETX = true;
		if (r == 1) fehlerKeinSTX = true;
		if (r == 2) fehlerFalscheP = true;


		// 5) Payload bauen: "v1;...;v24;P" (P ist 25. Element)
		string payload = "";
		for (int i = 0; i < 24; ++i) 
		{
			payload += to_string(arr[i]);
			payload += ';';
		}

		int pSend = p;
		
		if (fehlerFalscheP) {
			pSend = (p + 7) % 10; // absichtlich falsche Zahl
			cout << "(Fehler injiziert: falsche Pruefziffer)" << endl;
		}
		payload += to_string(pSend);

		//cout << "Payload: " << payload << endl;


		// 6) Antwort-Frame zusammenbauen: STX + payload + ETX + '\n'
		string rsp = "";
		if (fehlerKeinSTX) {
			rsp += '@'; // falscher Start (kein STX)
			cout << "(Fehler injiziert: Kein STX)" << endl;
			rsp += "ERROR";

		}
		else 
		{
			rsp += (unsigned char)STX;
		}

		rsp += payload;
		
		if (fehlerKeinETX) 
		{
			cout << "(Fehler injiziert: Kein ETX)" << endl;
			rsp += "ERROR";
			
			
		}
		else 
		{
			rsp += (char)ETX;
		}

		rsp += '\n';

		cout << "Die antwort die Gesendet wird ist: " << rsp << endl;

		// 7) Senden
		stromzaehlerSerial->write(rsp);

		cout << "Antwort gesendet (nr=" << nr << ")" << endl;
	}
}


// Hilfsfunktion: druckt druckbare ASCII normal und Steuerzeichen als <Zahl>
string Stromzaehler::mitSteuerzeichen(const string& s) {
	string out;
	for (int i = 0; i < (int)s.size(); ++i) {
		unsigned char c = (unsigned char)s[i];
		if (c >= 32 && c < 127) {
			out += (char)c; // druckbar
		}
		else {
			out += '<';
			out += to_string((int)c); // z. B. 1, 2, 3 fuer SOH/STX/ETX
			out += '>';
		}
	}
	return out;
}