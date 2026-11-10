#ifndef PRESSURE_CHAMBER_CONTROLLER_H
#define PRESSURE_CHAMBER_CONTROLLER_H

#include <Arduino.h>
#include "gmp251.h"

typedef enum
{
    O2 = 0,
    CO2,
    AIR,
    SAFETY,
    MAX_VALVES
} eValves;

extern GMP251 co2Sensor;

/**
 * @class PressureChamberController
 * @brief Régulation de la chambre sous pression (O2, CO2, air).
 *
 * Principe : impulsions courtes suivies d'un temps mort.
 * - Le CO2 est prioritaire (il pilote ensuite le pH).
 * - L'O2 est corrigé quand le CO2 est dans sa bande, ou quand il reste sous sa
 *   borne basse depuis plus de O2_MAX_WAIT_MS (tour de garde).
 * - Une seule vanne à la fois, et aucune nouvelle décision avant la fin du temps mort.
 * - Les lectures O2 et CO2 sont les valeurs BRUTES (proportionnelles à la pression partielle).
 *   Les consignes sont donc exprimées dans la même unité que les lectures brutes.
 */
class PressureChamberController
{
public:
    PressureChamberController();
    void update(float o2Concentration, float co2Concentration, float pressure);
    bool getValveState(eValves Valve) const;
    unsigned long getValveOpenDurationMs(eValves Valve) const;
    float getO2Reference() const;
    float getCO2Reference() const;
    void setReferenceLevel(eValves Valve, float ReferenceLevel);
    void setPressureChamberState(bool state) { this->pressureChamberState = state; }

private:
    float calculateTimeBeforeClosingValve(eValves Valve, float error);
    float computePulseMs(eValves Valve, float error);
    void startPulse(eValves Valve, float pulseMs);
    void closeAllValves();
    void resetState();

    // État de la régulation.
    bool pressureChamberState;
    unsigned long timeBeforeClosingO2Valve;
    unsigned long timeBeforeClosingCO2Valve;
    unsigned long timeBeforeClosingAirValve;
    unsigned long o2ValveOpenDurationMs; // durée décidée à la dernière décision (0 si aucune)
    unsigned long co2ValveOpenDurationMs;
    unsigned long airValveOpenDurationMs;
    unsigned long settleUntilMs;  // pas de nouvelle décision avant cet instant
    bool o2BelowActive;           // l'O2 est sous sa borne basse
    unsigned long o2BelowSinceMs; // depuis quand

    // Consignes et bornes.
    float o2MinRef;
    float o2MaxRef;
    float co2MinRef;
    float co2MaxRef;
    float co2Ref;
    float o2Ref;

    // Modèle de débit (loi de Poiseuille) : à valider par un étalonnage de gain (voir *_GAIN_* plus bas).
    static constexpr float R = 0.0043;                           // Rayon intérieur du tube (m)
    static constexpr float L = 2;                                // Longueur du tube (m)
    static constexpr float MU_AIR = 0.000018;                    // Viscosité de l'air
    static constexpr float MU_O2 = 0.00002;                      // Viscosité de l'O2
    static constexpr float MU_CO2 = 0.000015;                    // Viscosité du CO2
    static constexpr float P_APPROV = 30 * 6895;                 // Pression des bouteilles (Pa) : À VÉRIFIER (psi relatifs)
    static constexpr float P_CHAMBER = 7.5f * 6895;              // Pression de la chambre (Pa), 7,5 psi relatifs
    static constexpr float P_CHAMBER_MIN = P_CHAMBER - 2 * 6895; // Pression minimale : 2 psi sous la consigne
    static constexpr float V = 1.296;                            // Volume de la chambre (L)

    // Consignes par défaut et zones mortes (unités des lectures brutes).
    static constexpr float O2_REF = 85.0f;
    static constexpr float CO2_REF = 50000.0f; // ppm
    // Zones mortes = au moins la moitié du plus petit pas possible (impulsion minimale), sinon la
    // régulation dépasse sa cible à chaque impulsion. Pas minimal mesuré (FLOW_TEST, 250 ms) :
    // O2 ~ +10 points bruts, CO2 ~ +40 000 ppm bruts.
    // Impulsion minimale 200 ms (FLOW_TEST 175/200/225 ms) : pas O2 ~ +8,4 points, pas CO2 ~ +28 000 ppm.
    static constexpr float O2_DEAD_ZONE = 5.0f;      // points d'O2 (lecture brute)
    static constexpr float CO2_DEAD_ZONE = 15000.0f; // ppm (lecture brute) ; À TESTER : 175 ms n'a aucun effet, la marge est mince

    // Conversions.
    static constexpr float PERCENT_TO_LITERS = 0.01 * V;
    static constexpr float PPM_TO_LITERS = 0.000001 * V;
    static constexpr float SECONDS_TO_MILLIS = 1000.0f;

    // Impulsions.
    static constexpr float MIN_PULSE_MS = 200.0f; // mesuré : 150 ms (O2) et 175 ms (CO2) n'ont aucun effet ; si le CO2 rate, remonter à 225-250
    static constexpr float MAX_O2_PULSE_MS = 1000.0f;
    static constexpr float MAX_CO2_PULSE_MS = 2000.0f;
    static constexpr float MAX_AIR_PULSE_MS = 3000.0f;
    static constexpr float AIR_VALVE_OPEN_TIME = 1000.0f; // ouverture d'air si la pression est trop basse (ms)

    // Temps morts après une impulsion (le CO2 répond lentement : ~50 s observées).
    static constexpr unsigned long O2_SETTLE_MS = 15000UL;
    static constexpr unsigned long CO2_SETTLE_MS = 150000UL; // mesuré : pic atteint entre +111 s et +136 s (FLOW_TEST 2026-11-10)
    static constexpr unsigned long AIR_SETTLE_MS = 20000UL;
    static constexpr unsigned long O2_MAX_WAIT_MS = 180000UL; // tour de garde de l'O2 sous la priorité du CO2

    // Plafond de la carte O2 (échelle 0-100) : au-delà, la lecture n'est plus fiable.
    static constexpr float O2_SENSOR_CLIP = 99.0f;

    // Gains mesurés (FLOW_TEST du 2026-11-09, lectures brutes). 0 = utiliser le modèle de Poiseuille.
    // Durée d'ouverture = délai mort + erreur / gain.
    static constexpr float O2_GAIN_PTS_PER_S = 58.0f; // points bruts d'O2 par seconde d'ouverture (250/300/500 ms : +11,6 / +15,4 / +26)
    static constexpr float O2_DEAD_TIME_MS = 50.0f;
    static constexpr float CO2_GAIN_PPM_PER_S = 250000.0f; // ppm bruts par seconde d'ouverture
    static constexpr float CO2_DEAD_TIME_MS = 105.0f;

    // Facteurs de correction.
    static constexpr float CORRECTION_FACTOR_O2 = 1.0f;
    static constexpr float CORRECTION_FACTOR_CO2 = 1.0f;
    static constexpr float CORRECTION_FACTOR_O2_REDUCTION = 40.0f;  // purge à l'air (O2 trop haut)
    static constexpr float CORRECTION_FACTOR_CO2_REDUCTION = 30.0f; // purge à l'air (CO2 trop haut)
};

#endif // PRESSURE_CHAMBER_CONTROLLER_H