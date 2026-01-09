#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <limits>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <ctime>

using namespace std;

// --- URGENCY LEVELS (NHS STANDARD) ---
enum UrgencyCode {
    BLUE_PHARMACY = 6,     // "Self Care / Pharmacy"
    GREEN_GP = 5,          // "Routine GP Appointment"
    YELLOW_WALK_IN = 4,    // "Urgent Care Centre / 111"
    AMBER_HOSPITAL = 3,    // "A&E (Non-Life Threatening)"
    ORANGE_MAJOR = 2,      // "A&E (Immediate / Resus)"
    RED_CRITICAL = 1       // "999 EMERGENCY"
};

struct Patient {
    string fullName;
    int patientID;
    int age;
    char gender;
    
    // Vitals
    int spO2 = -1;
    double temp = -1.0;
    int sysBP = -1;
    
    UrgencyCode code = BLUE_PHARMACY;
    string clinicalNotes;
    int symptomCategory = 0;
};

class NHSTriageSystem {
private:
    int currentIDCounter = 1001;

    // UTILITY: Helper for Yes/No inputs
    bool ask(string q) {
        char i;
        while(true) {
            cout << ">> " << q << " (y/n): ";
            cin >> i;
            if(tolower(i)=='y') return true;
            if(tolower(i)=='n') return false;
            cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }

    // UTILITY: Emergency Trigger
    void trigger999(string reason) {
        cout << "\n\n*******************************************************\n";
        cout << "   [!!!] CRITICAL EMERGENCY - CALLING 999 [!!!]        \n";
        cout << "*******************************************************\n";
        cout << "REASON: " << reason << endl;
        cout << "ACTION: Ambulance Dispatching...\n";
        exit(0);
    }

    // --- MODULE 1: REGISTRATION (FIXED IO) ---
    void registerPatient(Patient &p) {
        cout << "\n--- PATIENT REGISTRATION ---\n";
        
        // 1. ASK NAME FIRST (Clean Input)
        // We use ws to eat any leading whitespace if necessary, but standard getline is safer here.
        cout << "Enter Full Name: ";
        if (cin.peek() == '\n') cin.ignore(); // Safety check if buffer was dirty
        getline(cin, p.fullName);
        
        // 2. ASK AGE & GENDER
        p.patientID = currentIDCounter++;
        cout << "Enter Age: ";
        cin >> p.age;
        
        cout << "Enter Gender (M/F): ";
        cin >> p.gender;

        cout << "[SYSTEM] Profile Created. ID: #" << p.patientID << " for " << p.fullName << endl;
    }

    // --- MODULE 2: VITALS CHECK (UPDATED SpO2) ---
    void checkVitals(Patient &p) {
        cout << "\n[VITALS CHECK]\n";
        
        // 1. OXYGEN (Updated Cutoff: 90%)
        cout << ">> SpO2 % (0 to skip): "; cin >> p.spO2;
        
        if(p.spO2 > 0 && p.spO2 <= 90) {  // <-- CHANGED TO 90
            trigger999("Severe Hypoxia (SpO2 <= 90%)");
        }
        if(p.spO2 > 0 && p.spO2 <= 94) {
            p.code = min(p.code, AMBER_HOSPITAL);
            p.clinicalNotes += "Hypoxia (91-94%). ";
        }

        // 2. BLOOD PRESSURE (Adults Only)
        if (p.age > 12) {
            cout << ">> Systolic BP (0 to skip): "; cin >> p.sysBP;
            if(p.sysBP > 0 && p.sysBP < 90) trigger999("Hypotension / Shock (BP < 90)");
            if(p.sysBP > 220) {
                 p.code = min(p.code, ORANGE_MAJOR);
                 p.clinicalNotes += "Hypertensive Crisis Risk. ";
            }
        }
        
        // 3. TEMPERATURE
        cout << ">> Temp (C) (0 to skip): "; cin >> p.temp;
        if(p.temp > 38.0) {
            if(p.age == 0) trigger999("Infant Fever (Sepsis Risk)");
            if(p.age >= 75) {
                p.code = min(p.code, AMBER_HOSPITAL);
                p.clinicalNotes += "Elderly Fever Risk. ";
            }
        }
    }

    // --- MODULE 3: SYMPTOM CHARTS ---
    void assessChest(Patient &p) {
        p.symptomCategory = 1;
        cout << "\n[ASSESSING CHEST/HEART]\n";
        if(ask("Is the patient cold, clammy, or pale?")) trigger999("Cardiac Shock Signs");
        
        bool crushing = ask("Is there heavy pressure/crushing pain?");
        bool radiating = ask("Does pain radiate to jaw, neck or left arm?");
        
        if(crushing || radiating) {
            p.code = min(p.code, ORANGE_MAJOR);
            p.clinicalNotes += "Cardiac Chest Pain (STEMI Risk). ";
        } else if (ask("Is pain worse when breathing in?")) {
            p.code = min(p.code, YELLOW_WALK_IN);
            p.clinicalNotes += "Pleuritic Pain. ";
        }
    }

    void assessStomach(Patient &p) {
        p.symptomCategory = 2;
        cout << "\n[ASSESSING STOMACH]\n";
        if(p.age > 50 && ask("Is there tearing pain radiating to the back?")) trigger999("Suspected AAA Rupture");
        
        if(toupper(p.gender) == 'F' && p.age >= 12 && p.age <= 50) {
            if(ask("Is there any possibility of pregnancy?")) {
                 p.code = min(p.code, AMBER_HOSPITAL);
                 p.clinicalNotes += "Pregnancy Risk (Rule out Ectopic). ";
            }
        }
        if(ask("Is pain in the lower right side?")) {
            p.code = min(p.code, AMBER_HOSPITAL);
            p.clinicalNotes += "Possible Appendicitis. ";
        }
    }

    void assessHead(Patient &p) {
        p.symptomCategory = 3;
        cout << "\n[ASSESSING HEAD/NEURO]\n";
        if(p.age > 40) {
            if(ask("Any facial drooping, arm weakness, or slurred speech?")) trigger999("Stroke (FAST Positive)");
        }
        if(ask("Did they hit their head?")) {
            if(ask("Any loss of consciousness?")) {
                if(p.age >= 65) trigger999("Head Injury + Elderly (Bleed Risk)");
                if(ask("Are they on blood thinners?")) trigger999("Head Injury + Anticoagulants");
                p.code = min(p.code, ORANGE_MAJOR);
                p.clinicalNotes += "Concussion with LOC. ";
            }
            if(ask("Have they vomited >1 time?")) {
                 p.code = min(p.code, AMBER_HOSPITAL);
                 p.clinicalNotes += "Head Injury + Vomiting. ";
            }
        }
    }

    // --- MODULE 4: LOGISTICS & SAFETY ---
    void printEstimatedWait(UrgencyCode code) {
        srand(time(0));
        int busyFactor = rand() % 30;
        int minutes = 0;
        switch(code) {
            case RED_CRITICAL: minutes = 0; break;
            case ORANGE_MAJOR: minutes = 10; break;
            case AMBER_HOSPITAL: minutes = 60 + busyFactor; break;
            case YELLOW_WALK_IN: minutes = 120 + busyFactor; break;
            case GREEN_GP: minutes = 48 * 60;
            default: minutes = 0;
        }
        cout << "\n[LOGISTICS ESTIMATE]\n";
        if (code == RED_CRITICAL) cout << "WAIT TIME: 0 MINS (IMMEDIATE ENTRY)\n";
        else if (code >= GREEN_GP) cout << "TIMEFRAME: Book appointment within 48 hours.\n";
        else cout << "ESTIMATED A&E WAIT TIME: " << minutes << " minutes.\n";
    }

    void printSafetyNetting(const Patient &p) {
        if (p.code <= AMBER_HOSPITAL) return;
        cout << "\n--- SAFETY NETTING ADVICE (IF SYMPTOMS WORSEN) ---\n";
        cout << "Stable condition confirmed. However, return to A&E IMMEDIATELY if:\n";
        switch(p.symptomCategory) {
            case 1:
                cout << "[!] You become unable to speak full sentences.\n[!] Pain spreads to neck, jaw, or back.\n"; break;
            case 2:
                cout << "[!] You vomit blood.\n[!] You cannot pass urine for 12 hours.\n"; break;
            case 3:
                cout << "[!] You vomit more than twice.\n[!] You become confused or have a seizure.\n"; break;
            default:
                cout << "[!] You develop a high fever (>39C) or a rash that doesn't fade.\n";
        }
        cout << "--------------------------------------------------\n";
    }

public:
    void start() {
        Patient p;
        cout << "=== NHS DIGITAL TRIAGE KIOSK ===\n";
        
        // 1. REGISTRATION (FIXED)
        registerPatient(p);
        
        // 2. SAFETY CHECKS (Vitals)
        checkVitals(p);

        // 3. MAIN COMPLAINT
        cout << "\nSELECT SYMPTOM:\n1. Chest/Breathing\n2. Stomach Pain\n3. Head Injury/Stroke\n4. Other/Fever\n>> ";
        int choice; cin >> choice;

        switch(choice) {
            case 1: assessChest(p); break;
            case 2: assessStomach(p); break;
            case 3: assessHead(p); break;
            default: p.symptomCategory = 4; p.clinicalNotes += "General/Fever. "; break;
        }

        // 4. FINAL RESULT
        cout << "\n==========================================\n";
        cout << "PATIENT: " << p.fullName << " (Age: " << p.age << ")\n";
        cout << "------------------------------------------\n";
        cout << "TRIAGE RESULT: ";
        switch(p.code) {
            case RED_CRITICAL: cout << "RED (CRITICAL - 999)"; break;
            case ORANGE_MAJOR: cout << "ORANGE (A&E MAJORS)"; break;
            case AMBER_HOSPITAL: cout << "AMBER (A&E / URGENT CARE)"; break;
            case YELLOW_WALK_IN: cout << "YELLOW (WALK-IN CENTRE)"; break;
            case GREEN_GP: cout << "GREEN (GP APPOINTMENT)"; break;
            case BLUE_PHARMACY: cout << "BLUE (SELF CARE)"; break;
        }
        cout << "\nCLINICAL NOTES: " << p.clinicalNotes << endl;
        cout << "==========================================\n";

        // 5. LOGISTICS
        printEstimatedWait(p.code);
        printSafetyNetting(p);
    }
};

int main() {
    NHSTriageSystem bot;
    bot.start();
    return 0;
}
