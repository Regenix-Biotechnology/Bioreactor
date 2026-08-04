#include "co2_controller.h"
#include <Wire.h>
#include "ledI2C.h"

CO2Controller::CO2Controller()
    : co2Ref(800.0f),
      co2MinRef(800.0f - 50.0f),
      co2MaxRef(800.0f + 50.0f)
{
}

/**
 * @brief State machine of the CO2 regulation.
 * @param co2Concentration The concentration of CO2 in the bioreactor in %.
 */
void CO2Controller::update(float co2Concentration)
{
    switch (currentState)
    {
    case CONCENTRATION_0_to_4:
        if (co2Concentration >= 4.0f)
        {
            currentState = CONCENTRATION_4_to_4_45;
        }
        break;

    case CONCENTRATION_4_to_4_45:
        if (co2Concentration >= 4.45f && co2Concentration <= 4.55f)
        {
            currentState = CONCENTRATION_4_45_to_4_55;
        }
        else if (co2Concentration < 4.0f)
        {
            currentState = CONCENTRATION_0_to_4;
        }
        break;

    case CONCENTRATION_4_45_to_4_55:
        if (co2Concentration > 4.55f)
        {
            currentState = CONCENTRATION_OVER_4_55;
        }
        else if (co2Concentration < 4.45f)
        {
            currentState = CONCENTRATION_UDER_4_45;
        }
        break;

    case CONCENTRATION_OVER_4_55:
        if (co2Concentration <= 4.55f)
        {
            currentState = CONCENTRATION_4_55_to_4_45;
        }
        break;

    case CONCENTRATION_4_55_to_4_45:
        if (co2Concentration < 4.45f)
        {
            currentState = CONCENTRATION_UDER_4_45;
        }
        else if (co2Concentration > 4.55f)
        {
            currentState = CONCENTRATION_OVER_4_55;
        }
        break;

    case CONCENTRATION_UDER_4_45:
        if (co2Concentration >= 4.45f && co2Concentration <= 4.55f)
        {
            currentState = CONCENTRATION_4_45_to_4_55;
        }
        else if (co2Concentration < 4.0f)
        {
            currentState = CONCENTRATION_0_to_4;
        }
        break;
    }

    // 2. Exécution des actions associées à l'état courant
    switch (currentState)
    {
    case CONCENTRATION_0_to_4:
        // Impulsion forte pour remplir le volume rapidement
        IMPULSION_TIME = 500;
        break;

    case CONCENTRATION_4_to_4_45:
        // Réduction de l'impulsion pour approcher sans dépasser
        this->isCO2ValveOpen = false;
        IMPULSION_TIME = 0;
        break;

    case CONCENTRATION_4_45_to_4_55:
        this->isCO2ValveOpen = false;
        IMPULSION_TIME = 150;
        break;

    case CONCENTRATION_OVER_4_55:
        this->isCO2ValveOpen = false;
        IMPULSION_TIME = 0;
        CYCLE_TIME = 16000;
        break;

    case CONCENTRATION_4_55_to_4_45:
        IMPULSION_TIME = 200;
        CYCLE_TIME = 10000;
        break;

    case CONCENTRATION_UDER_4_45:
        IMPULSION_TIME = 300;
        break;
    }
    regulation();
}

/**
 * @brief Manage the state of the admission valve with the impulsion and cycle time.
 */
void CO2Controller::regulation()
{
    if (this->isCO2ValveOpen && millis() - LAST_CYCLE > IMPULSION_TIME)
    {
        this->isCO2ValveOpen = false;
    }
    if (!this->isCO2ValveOpen && millis() - LAST_CYCLE > CYCLE_TIME)
    {
        this->isCO2ValveOpen = true;
        LAST_CYCLE = millis();
    }
}

/**
 * @brief Sets the reference level for the specified valve.
 * @param ReferenceLevel The reference level to set.
 */
void CO2Controller::setReferenceLevel(float ReferenceLevel)
{
    this->co2Ref = ReferenceLevel;
    this->co2MinRef = ReferenceLevel - CO2_DEAD_ZONE;
    this->co2MaxRef = ReferenceLevel;
}
/**
 * @brief Gets the state of the CO2 admision valve.
 * @return return the state of the CO2 admision valve.
 */
bool CO2Controller::getValveState()
{
    return this->isCO2ValveOpen;
}