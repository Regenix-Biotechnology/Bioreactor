#include "pressure_chamber_controller.h"
#include <math.h>

/** @brief Vrai si l'instant `target` est atteint ou dépassé (robuste au débordement de millis()). */
static inline bool timeReached(unsigned long now, unsigned long target)
{
    return static_cast<long>(now - target) >= 0;
}

static inline float clampValue(float value, float lo, float hi)
{
    if (value < lo)
        return lo;
    if (value > hi)
        return hi;
    return value;
}

/**
 * @brief Constructor to initialize the control loop parameters.
 */
PressureChamberController::PressureChamberController()
    : pressureChamberState(false),
      timeBeforeClosingO2Valve(0),
      timeBeforeClosingCO2Valve(0),
      timeBeforeClosingAirValve(0),
      o2ValveOpenDurationMs(0),
      co2ValveOpenDurationMs(0),
      airValveOpenDurationMs(0),
      settleUntilMs(0),
      o2BelowActive(false),
      o2BelowSinceMs(0),
      o2MinRef(O2_REF - O2_DEAD_ZONE),
      o2MaxRef(O2_REF + O2_DEAD_ZONE),
      co2MinRef(CO2_REF - CO2_DEAD_ZONE),
      co2MaxRef(CO2_REF + CO2_DEAD_ZONE),
      co2Ref(CO2_REF),
      o2Ref(O2_REF)
{
}

/** @brief Ferme toutes les vannes immédiatement. */
void PressureChamberController::closeAllValves()
{
    const unsigned long now = millis();
    this->timeBeforeClosingO2Valve = now;
    this->timeBeforeClosingCO2Valve = now;
    this->timeBeforeClosingAirValve = now;
    this->o2ValveOpenDurationMs = 0;
    this->co2ValveOpenDurationMs = 0;
    this->airValveOpenDurationMs = 0;
}

/** @brief Ferme les vannes et remet à zéro les temporisations. */
void PressureChamberController::resetState()
{
    this->closeAllValves();
    this->settleUntilMs = 0;
    this->o2BelowActive = false;
    this->o2BelowSinceMs = 0;
}

/**
 * @brief Régulation de la chambre. À appeler à intervalle régulier.
 * @param o2Concentration Lecture BRUTE de l'O2 (%), proportionnelle à la pression partielle.
 * @param co2Concentration Lecture BRUTE du CO2 (ppm).
 * @param pressure Pression de la chambre (Pa).
 *
 * Règles :
 * 1. Pas de nouvelle décision pendant une impulsion ou pendant le temps mort qui la suit.
 * 2. Le CO2 est prioritaire. L'O2 passe si le CO2 est dans sa bande, ou s'il reste sous sa
 *    borne basse depuis plus de O2_MAX_WAIT_MS.
 * 3. Une lecture O2 >= O2_SENSOR_CLIP est considérée comme saturée : pas d'injection d'O2.
 * 4. Purge à l'air seulement si aucun gaz n'est trop bas ; le CO2 trop haut est traité avant l'O2.
 */
void PressureChamberController::update(float o2Concentration, float co2Concentration, float pressure)
{
    if (!this->pressureChamberState)
    {
        this->resetState();
        return;
    }

    if (co2Sensor.getStatus() != GMP_251_STATUS_OK ||
        !isfinite(o2Concentration) ||
        !isfinite(co2Concentration) ||
        !isfinite(pressure) ||
        o2Concentration <= 0.0f ||
        co2Concentration <= 0.0f ||
        this->o2Ref <= 0.0f ||
        this->co2Ref <= 0.0f)
    {
        // Données invalides : vannes fermées tout de suite, y compris en cours d'impulsion.
        this->closeAllValves();
        this->o2BelowActive = false;
        return;
    }

    const unsigned long now = millis();

    // Plafond de la carte O2 : on coupe l'injection d'O2 immédiatement.
    const bool o2Clipped = o2Concentration >= O2_SENSOR_CLIP;
    if (o2Clipped)
    {
        this->timeBeforeClosingO2Valve = now;
        this->o2ValveOpenDurationMs = 0;
    }

    // Les durées exposées sont celles décidées à cet appel (0 s'il n'y a pas de nouvelle impulsion).
    this->o2ValveOpenDurationMs = 0;
    this->co2ValveOpenDurationMs = 0;
    this->airValveOpenDurationMs = 0;

    // Suivi du temps passé par l'O2 sous sa borne basse (tour de garde).
    const bool o2Below = !o2Clipped && o2Concentration < this->o2MinRef;
    if (o2Below)
    {
        if (!this->o2BelowActive)
        {
            this->o2BelowActive = true;
            this->o2BelowSinceMs = now;
        }
    }
    else
    {
        this->o2BelowActive = false;
    }

    // Règle 1 : pas de nouvelle décision pendant une impulsion ou le temps mort.
    const bool pulseActive = !timeReached(now, this->timeBeforeClosingO2Valve) ||
                             !timeReached(now, this->timeBeforeClosingCO2Valve) ||
                             !timeReached(now, this->timeBeforeClosingAirValve);
    if (pulseActive || !timeReached(now, this->settleUntilMs))
        return;

    const bool co2Below = co2Concentration < this->co2MinRef;
    const bool o2Starving = o2Below && (now - this->o2BelowSinceMs >= O2_MAX_WAIT_MS);

    if (co2Below && !o2Starving)
    {
        // Règle 2 : le CO2 est prioritaire.
        const float co2Deficit = this->co2Ref - co2Concentration;
        this->startPulse(CO2, this->computePulseMs(CO2, co2Deficit));
    }
    else if (o2Below)
    {
        // CO2 dans sa bande, ou O2 en attente depuis trop longtemps.
        const float o2Deficit = this->o2Ref - o2Concentration;
        this->startPulse(O2, this->computePulseMs(O2, o2Deficit));
        this->o2BelowActive = false; // le chronomètre repart au prochain appel
    }
    else
    {
        // Aucun gaz trop bas : purge à l'air si nécessaire (CO2 prioritaire sur l'O2).
        float airValveTime = 0.0f;

        if (co2Concentration > this->co2MaxRef)
        {
            const float effectiveCO2Error = (co2Concentration - this->co2MaxRef) / co2Concentration;
            airValveTime = calculateTimeBeforeClosingValve(AIR, effectiveCO2Error) *
                           CORRECTION_FACTOR_CO2_REDUCTION;
        }
        else if (o2Concentration > this->o2MaxRef)
        {
            const float effectiveO2Error = (o2Concentration - this->o2MaxRef) / o2Concentration;
            airValveTime = calculateTimeBeforeClosingValve(AIR, effectiveO2Error) *
                           CORRECTION_FACTOR_O2_REDUCTION;
        }

        if (pressure < P_CHAMBER_MIN)
        {
            const float airMinTime = AIR_VALVE_OPEN_TIME;
            if (airValveTime < airMinTime)
                airValveTime = airMinTime;
        }

        if (airValveTime > 0.0f && isfinite(airValveTime))
        {
            const float airMaxPulse = MAX_AIR_PULSE_MS;
            const float airMinPulse = MIN_PULSE_MS;
            this->startPulse(AIR, clampValue(airValveTime, airMinPulse, airMaxPulse));
        }
    }
}

/**
 * @brief Calcule la durée d'impulsion pour corriger `error` (erreur positive, depuis la consigne).
 *        Utilise le gain mesuré s'il est renseigné, sinon le modèle de Poiseuille.
 * @return Durée en ms, bornée entre MIN_PULSE_MS et le maximum de la vanne, ou 0 si le calcul est invalide.
 */
float PressureChamberController::computePulseMs(eValves Valve, float error)
{
    if (!(error > 0.0f))
        return 0.0f;

    float gain = 0.0f;
    float deadTimeMs = 0.0f;
    float factor = 1.0f;
    float maxMs = 0.0f;
    switch (Valve)
    {
    case O2:
        gain = O2_GAIN_PTS_PER_S;
        deadTimeMs = O2_DEAD_TIME_MS;
        factor = CORRECTION_FACTOR_O2;
        maxMs = MAX_O2_PULSE_MS;
        break;
    case CO2:
        gain = CO2_GAIN_PPM_PER_S;
        deadTimeMs = CO2_DEAD_TIME_MS;
        factor = CORRECTION_FACTOR_CO2;
        maxMs = MAX_CO2_PULSE_MS;
        break;
    default:
        return 0.0f;
    }

    float pulseMs = 0.0f;
    if (gain > 0.0f)
        pulseMs = deadTimeMs + error / gain * SECONDS_TO_MILLIS; // délai mort + temps utile
    else
        pulseMs = calculateTimeBeforeClosingValve(Valve, error) * factor;

    if (!isfinite(pulseMs) || !(pulseMs > 0.0f))
        return 0.0f;

    const float minMs = MIN_PULSE_MS;
    return clampValue(pulseMs, minMs, maxMs);
}

/**
 * @brief Ouvre une vanne pour `pulseMs` et démarre le temps mort qui suit.
 */
void PressureChamberController::startPulse(eValves Valve, float pulseMs)
{
    if (!(pulseMs > 0.0f) || !isfinite(pulseMs))
        return;

    const unsigned long now = millis();
    const unsigned long duration = static_cast<unsigned long>(pulseMs);
    unsigned long settle = 0;

    switch (Valve)
    {
    case O2:
        this->o2ValveOpenDurationMs = duration;
        this->timeBeforeClosingO2Valve = now + duration;
        settle = O2_SETTLE_MS;
        break;
    case CO2:
        this->co2ValveOpenDurationMs = duration;
        this->timeBeforeClosingCO2Valve = now + duration;
        settle = CO2_SETTLE_MS;
        break;
    case AIR:
        this->airValveOpenDurationMs = duration;
        this->timeBeforeClosingAirValve = now + duration;
        settle = AIR_SETTLE_MS;
        break;
    default:
        return;
    }

    this->settleUntilMs = now + duration + settle;
}

/**
 * @brief Calculates the time before closing the valve based on the error.
 * @param Valve The valve to be closed.
 * @param error The error value.
 * @return The time before closing the valve in milliseconds.
 * @note Modèle simplifié (à valider par un étalonnage de gain) :
 * - Le débit est supposé constant alors que la pression monte pendant l'ouverture.
 * - Aucune perte de gaz n'est supposée pendant l'ajout.
 */
float PressureChamberController::calculateTimeBeforeClosingValve(eValves Valve, float error)
{
    float timeBeforeClosing = 0;
    float q = 0.0f;           // Flow rate in liters per second
    float volumeToAdd = 0.0f; // Volume to add in liters
    switch (Valve)
    {
    case O2:
        q = (PI * pow(R, 4)) / (8 * MU_O2 * L) * (P_APPROV - P_CHAMBER); // Poiseuille
        volumeToAdd = error * PERCENT_TO_LITERS;
        break;
    case CO2:
        q = (PI * pow(R, 4)) / (8 * MU_CO2 * L) * (P_APPROV - P_CHAMBER);
        volumeToAdd = error * PPM_TO_LITERS;
        break;
    case AIR:
        q = (PI * pow(R, 4)) / (8 * MU_AIR * L) * (P_APPROV - P_CHAMBER);
        volumeToAdd = error * PERCENT_TO_LITERS;
        break;
    default:
        return 0;
    }

    if (!(q > 0.0f))
        return 0;

    timeBeforeClosing = volumeToAdd / q * SECONDS_TO_MILLIS; // t = V / Q, en ms
    return timeBeforeClosing;
}

/**
 * @brief Returns the state of the specified valve.
 * @param Valve The valve to check.
 * @return The state of the valve (true for open, false for closed).
 */
bool PressureChamberController::getValveState(eValves Valve) const
{
    if (!this->pressureChamberState)
        return false;

    const unsigned long now = millis();
    switch (Valve)
    {
    case O2:
        return !timeReached(now, timeBeforeClosingO2Valve);
    case CO2:
        return !timeReached(now, timeBeforeClosingCO2Valve);
    case AIR:
        return !timeReached(now, timeBeforeClosingAirValve);
    case SAFETY:
        return false;
    default:
        return false;
    }
}

unsigned long PressureChamberController::getValveOpenDurationMs(eValves Valve) const
{
    switch (Valve)
    {
    case O2:
        return this->o2ValveOpenDurationMs;
    case CO2:
        return this->co2ValveOpenDurationMs;
    case AIR:
        return this->airValveOpenDurationMs;
    default:
        return 0;
    }
}

float PressureChamberController::getO2Reference() const
{
    return this->o2Ref;
}

float PressureChamberController::getCO2Reference() const
{
    return this->co2Ref;
}

/**
 * @brief Sets the reference level for the specified valve.
 * @param Valve The valve to set the reference level for.
 * @param ReferenceLevel The reference level to set.
 */
void PressureChamberController::setReferenceLevel(eValves Valve, float ReferenceLevel)
{
    switch (Valve)
    {
    case O2:
        this->o2Ref = ReferenceLevel;
        this->o2MinRef = ReferenceLevel - O2_DEAD_ZONE;
        this->o2MaxRef = ReferenceLevel + O2_DEAD_ZONE;
        break;
    case CO2:
        this->co2Ref = ReferenceLevel;
        this->co2MinRef = ReferenceLevel - CO2_DEAD_ZONE;
        this->co2MaxRef = ReferenceLevel + CO2_DEAD_ZONE;
        break;
    default:
        break;
    }
}