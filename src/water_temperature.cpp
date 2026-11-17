#include "water_temperature.h"
#include "main.h"
#include <math.h>

namespace
{
    enum class eWaterTempSource : uint8_t
    {
        NONE = 0,
        ATLAS,
        DO
    };

    constexpr float WATER_TEMP_MIN_PLAUSIBLE_C = 5.0f;
    constexpr float WATER_TEMP_MAX_PLAUSIBLE_C = 60.0f;
    constexpr float ATLAS_MAX_JUMP_C = 2.0f;        // max change between two 1 s updates
    constexpr float ATLAS_DO_MAX_MISMATCH_C = 2.0f; // max difference Atlas / VisiFerm
    constexpr unsigned long DO_TEMP_MAX_AGE_MS = 5000UL;
    constexpr uint8_t ATLAS_GOOD_COUNT_TO_RETURN = 10; // 10 s of good readings before going back to Atlas

    eWaterTempSource source = eWaterTempSource::NONE;
    bool sourceInitialised = false;
    float waterTemperatureC = NAN;
    float lastAtlasTemperatureC = NAN;
    uint8_t atlasGoodCount = 0;

    const char *sourceName(eWaterTempSource s)
    {
        switch (s)
        {
        case eWaterTempSource::ATLAS:
            return "ATLAS";
        case eWaterTempSource::DO:
            return "DO";
        default:
            return "NONE";
        }
    }

    bool isPlausible(float t)
    {
        return isfinite(t) && t >= WATER_TEMP_MIN_PLAUSIBLE_C && t <= WATER_TEMP_MAX_PLAUSIBLE_C;
    }

    /** Atlas reading usable on its own (status, age, range, no jump). */
    bool readAtlas(float &t, const char *&reason)
    {
        t = tempSensor.getTemperatureC();
        const float previous = lastAtlasTemperatureC;
        lastAtlasTemperatureC = isPlausible(t) ? t : NAN;

        if (tempSensor.getStatus() != ATLAS_STATUS_OK ||
            tempSensor.getAgeMs() > 3 * TEMPERATURE_CONTROLLER_UPDATE_INTERVAL)
        {
            reason = "ATLAS_INVALID";
            return false;
        }
        if (!isPlausible(t))
        {
            reason = "ATLAS_OUT_OF_RANGE";
            return false;
        }
        if (isfinite(previous) && fabsf(t - previous) > ATLAS_MAX_JUMP_C)
        {
            reason = "ATLAS_JUMP";
            return false;
        }
        reason = "ATLAS_OK";
        return true;
    }

    /** VisiFerm temperature usable (last successful read < 5 s, plausible). */
    bool readDo(float &t)
    {
        t = dissolvedOxygenSensor.getTemperature();
        const uint32_t lastReadMs = dissolvedOxygenSensor.getLastReadMs();
        return lastReadMs != 0 &&
               millis() - lastReadMs <= DO_TEMP_MAX_AGE_MS &&
               isPlausible(t);
    }
}

void updateWaterTemperature()
{
    float atlasT = NAN;
    float doT = NAN;
    const char *atlasReason = "ATLAS_OK";

    bool atlasOk = readAtlas(atlasT, atlasReason);
    const bool doOk = readDo(doT);

    if (atlasOk && doOk && fabsf(atlasT - doT) > ATLAS_DO_MAX_MISMATCH_C)
    {
        atlasOk = false;
        atlasReason = "ATLAS_DO_MISMATCH";
    }

    // Hysteresis: once on DO, go back to Atlas only after ATLAS_GOOD_COUNT_TO_RETURN good readings.
    if (atlasOk)
    {
        if (atlasGoodCount < 255)
            atlasGoodCount++;
    }
    else
    {
        atlasGoodCount = 0;
    }
    const bool atlasAccepted =
        atlasOk && (source == eWaterTempSource::ATLAS ||
                    !sourceInitialised ||
                    atlasGoodCount >= ATLAS_GOOD_COUNT_TO_RETURN ||
                    !doOk);

    eWaterTempSource newSource;
    const char *reason;
    if (atlasAccepted)
    {
        newSource = eWaterTempSource::ATLAS;
        waterTemperatureC = atlasT;
        reason = doOk ? "ATLAS_OK" : "ATLAS_OK_DO_UNAVAILABLE";
    }
    else if (doOk)
    {
        newSource = eWaterTempSource::DO;
        waterTemperatureC = doT;
        reason = atlasOk ? "ATLAS_STABILISING" : atlasReason;
    }
    else
    {
        newSource = eWaterTempSource::NONE;
        waterTemperatureC = NAN;
        reason = "NO_VALID_SENSOR";
    }

    if (newSource != source || !sourceInitialised)
    {
        Serial.println(String("EVENT:TEMP:SOURCE=") + sourceName(newSource) + ";REASON=" + reason);
        if (newSource == eWaterTempSource::NONE)
            Serial.println("ERR:TEMP:NO_VALID_SENSOR");
    }
    source = newSource;
    sourceInitialised = true;
}

bool isWaterTemperatureValid()
{
    return source != eWaterTempSource::NONE && isfinite(waterTemperatureC);
}

float getWaterTemperature()
{
    return isWaterTemperatureValid() ? waterTemperatureC : NAN;
}

const char *getWaterTemperatureSourceName()
{
    return sourceName(source);
}