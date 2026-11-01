#pragma once

#include "raylib.h"
#include "raymath.h"

#include "helperFunc.h"
#include "planetSettings.h"
#include "planetTerrain.h"

#include <cmath>

static constexpr float PLANET_CLIMATE_DEG2RAD = 3.14159265358979323846f / 180.0f;
static constexpr float PLANET_CLIMATE_HALF_PI = 3.14159265358979323846f * 0.5f;

inline Vector3 PlanetClimateNorthAxis(
    const PlanetClimateStageSettings& settings
)
{
    float tilt = settings.axialTiltDegrees * PLANET_CLIMATE_DEG2RAD;

    return Vector3Normalize(
        Vector3{ 0.0f, cosf(tilt), sinf(tilt) }
    );
}

inline float PlanetClimateLatitude01(
    Vector3 normal,
    Vector3 northAxis,
    const PlanetClimateStageSettings& settings
)
{
    float latitudeDot = PlanetClamp(
        Vector3DotProduct(normal, northAxis),
        -1.0f,
        1.0f
    );

    float latitude = asinf(latitudeDot);

    float seasonOffset =
        settings.axialTiltDegrees *
        PLANET_CLIMATE_DEG2RAD *
        PlanetClamp(settings.season, -1.0f, 1.0f);

    return PlanetClamp01(
        fabsf(latitude - seasonOffset) / PLANET_CLIMATE_HALF_PI
    );
}

inline float PlanetClimateTemperature01(
    Vector3 normal,
    float latitude01,
    float elevation,
    const PerlinNoise& noise,
    const PlanetClimateStageSettings& s
)
{
    float equatorHeat = powf(1.0f - latitude01, s.latitudeFalloffPower);

    float temperature = PlanetLerp(
        s.poleTemperature,
        s.equatorTemperature * s.sunStrength,
        equatorHeat
    );

    float temperatureNoise = noise.fbm01(
        normal.x * s.temperatureNoiseFrequency + 17.13f,
        normal.y * s.temperatureNoiseFrequency + 91.71f,
        normal.z * s.temperatureNoiseFrequency + 43.09f,
        s.temperatureNoiseOctaves,
        s.persistence,
        s.lacunarity
    );

    temperature += (temperatureNoise - 0.5f) * s.temperatureNoiseStrength;

    float altitudeCooling = PlanetSmoothStep01(
        PlanetInverseLerp01(
            s.altitudeCoolingStart,
            s.altitudeCoolingEnd,
            elevation
        )
    );

    temperature -= altitudeCooling * s.altitudeCoolingStrength;

    return PlanetClamp01(temperature);
}

inline float PlanetClimateMoisture01(
    Vector3 normal,
    float latitude01,
    float elevation,
    float landHeight,
    const PerlinNoise& noise,
    const PlanetClimateStageSettings& s
)
{
    float equatorRain =
        PlanetPeak01(latitude01, 0.0f, s.equatorRainWidth) *
        s.equatorRainStrength;

    float dryBelt =
        PlanetPeak01(latitude01, s.dryBeltLatitude, s.dryBeltWidth) *
        s.dryBeltStrength;

    float midLatitudeRain =
        PlanetPeak01(latitude01, s.midLatitudeRainLatitude, s.midLatitudeRainWidth) *
        s.midLatitudeRainStrength;

    float polarDry =
        PlanetSmoothStep01(PlanetInverseLerp01(s.polarDryStart, 1.0f, latitude01)) *
        s.polarDryStrength;

    float moisture =
        s.baseMoisture +
        equatorRain +
        midLatitudeRain -
        dryBelt -
        polarDry;

    float moistureNoise = noise.fbm01(
        normal.x * s.moistureNoiseFrequency + 113.51f,
        normal.y * s.moistureNoiseFrequency + 7.33f,
        normal.z * s.moistureNoiseFrequency + 59.77f,
        s.moistureNoiseOctaves,
        s.persistence,
        s.lacunarity
    );

    moisture += (moistureNoise - 0.5f) * s.moistureNoiseStrength;

    if (elevation < 0.0f)
    {
        moisture += s.oceanMoistureBonus;
    }
    else
    {
        moisture -=
            PlanetInverseLerp01(0.0f, landHeight, elevation) *
            s.altitudeDryingStrength;
    }

    return PlanetClamp01(moisture);
}

inline bool ApplyPlanetClimateToSurface(
    PlanetSurfaceMap& surface,
    const PlanetGenerationSettings& settings
)
{
    if (!surface.valid)
    {
        return false;
    }

    const PlanetClimateStageSettings& climate = settings.climate;

    int texelCount = surface.width * surface.height;

    surface.temperature01.assign(texelCount, 0.0f);
    surface.moisture01.assign(texelCount, 0.0f);

    PerlinNoise climateNoise(settings.climateSeed);
    Vector3 northAxis = PlanetClimateNorthAxis(climate);
    float landHeight = settings.terrain.landHeight;

    PlanetParallelForRows(
        surface.height,
        [&](int pixelY)
        {
            for (int pixelX = 0; pixelX < surface.width; pixelX++)
            {
                int index = pixelY * surface.width + pixelX;

                Vector3 normal = PlanetAtlasPixelToSpherePoint(
                    pixelX,
                    pixelY,
                    surface.faceResolution,
                    1.0f
                );

                float elevation = surface.elevation[index];

                float latitude01 = PlanetClimateLatitude01(
                    normal,
                    northAxis,
                    climate
                );

                surface.temperature01[index] = PlanetClimateTemperature01(
                    normal,
                    latitude01,
                    elevation,
                    climateNoise,
                    climate
                );

                surface.moisture01[index] = PlanetClimateMoisture01(
                    normal,
                    latitude01,
                    elevation,
                    landHeight,
                    climateNoise,
                    climate
                );
            }
        }
    );

    surface.hasClimate = true;

    return true;
}
