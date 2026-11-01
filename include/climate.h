#pragma once

#include "raylib.h"
#include "raymath.h"

#include "terrain.h"

#include <cmath>

static constexpr float CLIMATE_PI = 3.14159265358979323846f;
static constexpr float CLIMATE_DEG2RAD = CLIMATE_PI / 180.0f;

enum PlanetClimateTextureMode
{
    CLIMATE_TEXTURE_BIOME = 0,
    CLIMATE_TEXTURE_TEMPERATURE,
    CLIMATE_TEXTURE_PRECIPITATION
};

struct PlanetClimateSettings
{

    float axialTiltDegrees = 23.5f;

    float season = 0.0f;

    float sunStrength = 1.0f;

    float latitudeFalloffPower = 1.35f;

    float poleTemperature = 0.05f;
    float equatorTemperature = 1.0f;

    float temperatureNoiseFrequency = 3.0f;
    int temperatureNoiseOctaves = 4;
    float temperatureNoiseStrength = 0.10f;

    float altitudeCoolingStart = 0.05f;
    float altitudeCoolingEnd = 0.38f;
    float altitudeCoolingStrength = 0.50f;

    float basePrecipitation = 0.10f;

    float equatorRainStrength = 0.95f;
    float equatorRainWidth = 0.23f;

    float dryBeltLatitude = 0.42f;
    float dryBeltWidth = 0.16f;
    float dryBeltStrength = 0.75f;

    float midLatitudeRainLatitude = 0.72f;
    float midLatitudeRainWidth = 0.20f;
    float midLatitudeRainStrength = 0.75f;

    float polarDryStart = 0.88f;
    float polarDryStrength = 0.35f;

    float precipitationNoiseFrequency = 4.5f;
    int precipitationNoiseOctaves = 5;
    float precipitationNoiseStrength = 0.22f;

    float oceanPrecipitationBonus = 0.10f;

    float persistence = 0.5f;
    float lacunarity = 2.0f;
};

struct PlanetClimateSample
{
    bool valid = false;

    float latitude01 = 0.0f;

    float elevation = 0.0f;

    float temperature01 = 0.0f;
    float precipitation01 = 0.0f;
};

inline float ClimateClamp01(float value)
{
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

inline float ClimateClamp(float value, float minValue, float maxValue)
{
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

inline float ClimateLerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

inline float ClimateInverseLerp(float a, float b, float value)
{
    float range = b - a;

    if (fabsf(range) < 0.0001f)
    {
        return 0.0f;
    }

    return ClimateClamp01((value - a) / range);
}

inline float ClimateSmoothStep01(float value)
{
    value = ClimateClamp01(value);
    return value * value * (3.0f - 2.0f * value);
}

inline Color ClimateLerpColor(Color a, Color b, float t)
{
    t = ClimateClamp01(t);

    Color result;
    result.r = (unsigned char)ClimateLerp((float)a.r, (float)b.r, t);
    result.g = (unsigned char)ClimateLerp((float)a.g, (float)b.g, t);
    result.b = (unsigned char)ClimateLerp((float)a.b, (float)b.b, t);
    result.a = 255;

    return result;
}

inline float ClimatePeak01(float x, float center, float width)
{
    if (width < 0.0001f)
    {
        width = 0.0001f;
    }

    float d = (x - center) / width;
    return expf(-d * d);
}

inline Vector3 ClimateGetTiltedNorthAxis(const PlanetClimateSettings& settings)
{
    float tiltRadians = settings.axialTiltDegrees * CLIMATE_DEG2RAD;

    Vector3 northAxis = {
        0.0f,
        cosf(tiltRadians),
        sinf(tiltRadians)
    };

    return Vector3Normalize(northAxis);
}

inline float ClimateNoise01(
    const PerlinNoise& climateNoise,
    Vector3 normal,
    float frequency,
    int octaves,
    float persistence,
    float lacunarity,
    Vector3 offset
)
{
    normal = Vector3Normalize(normal);

    return climateNoise.fbm01(
        normal.x * frequency + offset.x,
        normal.y * frequency + offset.y,
        normal.z * frequency + offset.z,
        octaves,
        persistence,
        lacunarity
    );
}

inline PlanetClimateSample SamplePlanetClimate(
    Vector3 spherePoint,
    const PlanetTerrainSampler& terrainSampler,
    const PerlinNoise& climateNoise,
    const PlanetClimateSettings& settings
)
{
    PlanetClimateSample sample;

    if (Vector3Length(spherePoint) < 0.0001f)
    {
        return sample;
    }

    Vector3 normal = Vector3Normalize(spherePoint);

    Vector3 northAxis = ClimateGetTiltedNorthAxis(settings);

    float latitudeDot = Vector3DotProduct(normal, northAxis);
    latitudeDot = ClimateClamp(latitudeDot, -1.0f, 1.0f);

    float latitudeRadians = asinf(latitudeDot);

    float season = ClimateClamp(settings.season, -1.0f, 1.0f);
    float seasonalOffsetRadians =
        settings.axialTiltDegrees * CLIMATE_DEG2RAD * season;

    float effectiveLatitudeRadians =
        latitudeRadians - seasonalOffsetRadians;

    float latitude01 = fabsf(effectiveLatitudeRadians) / (CLIMATE_PI * 0.5f);
    latitude01 = ClimateClamp01(latitude01);

    float elevation = 0.0f;

    if (terrainSampler.valid)
    {
        elevation = SamplePlanetTerrainElevation(
            normal,
            terrainSampler
        );
    }

    float equatorHeat = powf(
        1.0f - latitude01,
        settings.latitudeFalloffPower
    );

    float hotTemperature =
        settings.equatorTemperature * settings.sunStrength;

    float temperature = ClimateLerp(
        settings.poleTemperature,
        hotTemperature,
        equatorHeat
    );

    float temperatureNoise = ClimateNoise01(
        climateNoise,
        normal,
        settings.temperatureNoiseFrequency,
        settings.temperatureNoiseOctaves,
        settings.persistence,
        settings.lacunarity,
        Vector3{ 17.13f, 91.71f, 43.09f }
    );

    temperature +=
        (temperatureNoise - 0.5f) *
        settings.temperatureNoiseStrength;

    float altitudeCooling01 = ClimateSmoothStep01(
        ClimateInverseLerp(
            settings.altitudeCoolingStart,
            settings.altitudeCoolingEnd,
            elevation
        )
    );

    temperature -=
        altitudeCooling01 *
        settings.altitudeCoolingStrength;

    temperature = ClimateClamp01(temperature);

    float equatorRain =
        ClimatePeak01(latitude01, 0.0f, settings.equatorRainWidth) *
        settings.equatorRainStrength;

    float dryBelt =
        ClimatePeak01(latitude01, settings.dryBeltLatitude, settings.dryBeltWidth) *
        settings.dryBeltStrength;

    float midLatitudeRain =
        ClimatePeak01(latitude01, settings.midLatitudeRainLatitude, settings.midLatitudeRainWidth) *
        settings.midLatitudeRainStrength;

    float polarDry = ClimateSmoothStep01(
        ClimateInverseLerp(
            settings.polarDryStart,
            1.0f,
            latitude01
        )
    ) * settings.polarDryStrength;

    float precipitation =
        settings.basePrecipitation +
        equatorRain +
        midLatitudeRain -
        dryBelt -
        polarDry;

    float precipitationNoise = ClimateNoise01(
        climateNoise,
        normal,
        settings.precipitationNoiseFrequency,
        settings.precipitationNoiseOctaves,
        settings.persistence,
        settings.lacunarity,
        Vector3{ 113.51f, 7.33f, 59.77f }
    );

    precipitation +=
        (precipitationNoise - 0.5f) *
        settings.precipitationNoiseStrength;

    if (elevation < 0.0f)
    {
        precipitation += settings.oceanPrecipitationBonus;
    }

    precipitation = ClimateClamp01(precipitation);

    sample.valid = true;
    sample.latitude01 = latitude01;
    sample.elevation = elevation;
    sample.temperature01 = temperature;
    sample.precipitation01 = precipitation;

    return sample;
}

inline Color ClimateTemperatureColor(float temperature01)
{
    temperature01 = ClimateClamp01(temperature01);

    if (temperature01 < 0.50f)
    {
        float t = temperature01 / 0.50f;
        return ClimateLerpColor(
            Color{ 20, 35, 120, 255 },
            Color{ 70, 210, 230, 255 },
            t
        );
    }

    float t = (temperature01 - 0.50f) / 0.50f;
    return ClimateLerpColor(
        Color{ 240, 220, 75, 255 },
        Color{ 210, 45, 30, 255 },
        t
    );
}

inline Color ClimatePrecipitationColor(float precipitation01)
{
    precipitation01 = ClimateClamp01(precipitation01);

    if (precipitation01 < 0.50f)
    {
        float t = precipitation01 / 0.50f;
        return ClimateLerpColor(
            Color{ 210, 185, 95, 255 },
            Color{ 95, 165, 70, 255 },
            t
        );
    }

    float t = (precipitation01 - 0.50f) / 0.50f;
    return ClimateLerpColor(
        Color{ 95, 165, 70, 255 },
        Color{ 20, 90, 45, 255 },
        t
    );
}

inline Color ClimateBiomeColor(const PlanetClimateSample& sample)
{
    float temperature = ClimateClamp01(sample.temperature01);
    float precipitation = ClimateClamp01(sample.precipitation01);

    if (sample.elevation < 0.0f)
    {
        if (temperature < 0.18f)
        {
            return Color{ 185, 225, 235, 255 };
        }

        return ClimateLerpColor(
            Color{ 10, 55, 120, 255 },
            Color{ 25, 145, 190, 255 },
            temperature
        );
    }

    if (temperature < 0.12f)
    {
        return Color{ 235, 235, 230, 255 };
    }

    if (temperature < 0.25f)
    {
        if (precipitation < 0.35f)
        {
            return Color{ 160, 155, 135, 255 };
        }

        return Color{ 120, 145, 115, 255 };
    }

    if (precipitation < 0.20f)
    {
        return Color{ 215, 185, 100, 255 };
    }

    if (precipitation < 0.35f)
    {
        return Color{ 150, 170, 75, 255 };
    }

    if (precipitation < 0.62f)
    {
        if (temperature > 0.65f)
        {
            return Color{ 75, 145, 55, 255 };
        }

        return Color{ 70, 125, 65, 255 };
    }

    if (temperature > 0.65f)
    {
        return Color{ 20, 105, 45, 255 };
    }

    return Color{ 35, 105, 60, 255 };
}

inline Color ClimateTextureColor(
    const PlanetClimateSample& sample,
    PlanetClimateTextureMode mode
)
{
    if (!sample.valid)
    {
        return MAGENTA;
    }

    switch (mode)
    {
    case CLIMATE_TEXTURE_TEMPERATURE:
        return ClimateTemperatureColor(sample.temperature01);

    case CLIMATE_TEXTURE_PRECIPITATION:
        return ClimatePrecipitationColor(sample.precipitation01);

    case CLIMATE_TEXTURE_BIOME:
    default:
        return ClimateBiomeColor(sample);
    }
}

inline Texture2D GeneratePlanetClimateTexture(
    int faceResolution,
    float sphereRadius,
    float baseRadius,

    const PlanetPlateData& plateData,

    const PlanetNoiseSettings& noiseSettings,

    const PlanetTectonicData& tectonicData,
    const PlanetTectonicSettings& tectonicSettings,

    const PlanetClimateSettings& climateSettings,

    unsigned int terrainSeed,
    unsigned int climateSeed,

    PlanetClimateTextureMode textureMode = CLIMATE_TEXTURE_BIOME
)
{
    if (faceResolution < 2)
    {
        faceResolution = 2;
    }

    int atlasWidth = faceResolution * 3;
    int atlasHeight = faceResolution * 2;

    Image image = GenImageColor(
        atlasWidth,
        atlasHeight,
        MAGENTA
    );

    PlanetTerrainSampler terrainSampler = BuildPlanetTerrainSamplerWithTectonics(
        baseRadius,
        plateData,
        noiseSettings,
        tectonicData,
        tectonicSettings,
        terrainSeed
    );

    PerlinNoise climateNoise(climateSeed);

    for (int pixelY = 0; pixelY < atlasHeight; pixelY++)
    {
        for (int pixelX = 0; pixelX < atlasWidth; pixelX++)
        {
            Vector3 spherePoint = TectonicAtlasPixelToSpherePoint(
                pixelX,
                pixelY,
                faceResolution,
                sphereRadius
            );

            PlanetClimateSample sample = SamplePlanetClimate(
                spherePoint,
                terrainSampler,
                climateNoise,
                climateSettings
            );

            Color color = ClimateTextureColor(
                sample,
                textureMode
            );

            ImageDrawPixel(
                &image,
                pixelX,
                pixelY,
                color
            );
        }
    }

    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);

    return texture;
}
