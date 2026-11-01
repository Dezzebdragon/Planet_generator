#pragma once

#include "helperFunc.h"

struct PlanetPlateStageSettings
{
    int pointAmount = 1000;

    int largePlateAmount = 12;

    float pointRadiusOffset = 0.05f;
    float pointDisplacementStrength = 0.08f;

    int driftStepAmount = 30;

    int provisionalLargeEdgeNeighborCheckCount = 6;

    int finalEdgePointNeighborCheckCount = 6;

    int smallPlateAmount = 4;
    int smallPlateMinPoints = 6;
    int smallPlateMaxPoints = 18;

    float largePlateContinentalChance = 0.35f;
    float smallPlateContinentalChance = 0.25f;
};

struct PlanetTerrainStageSettings
{

    float continentFrequency = 1.25f;
    int continentOctaves = 5;

    float detailFrequency = 7.0f;
    int detailOctaves = 4;
    float detailStrength = 0.12f;

    float ridgeFrequency = 4.0f;
    int ridgeOctaves = 4;
    float ridgeStrength = 0.28f;

    float persistence = 0.5f;
    float lacunarity = 2.0f;

    float plateNoiseOffsetDistance = 100.0f;

    float plateBlendWidth = 0.25f;
    int maxBlendPlates = 4;

    float continentalCrustBias = 0.10f;
    float oceanicCrustBias = -0.08f;

    float oceanLevel = 0.46f;
    float oceanDepth = 0.12f;
    float landHeight = 0.38f;
    float landPower = 1.6f;

    float meshElevationScale = 0.35f;
    int meshSubdivisions = 128;
};

struct PlanetClimateStageSettings
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

    float baseMoisture = 0.10f;

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

    float moistureNoiseFrequency = 4.5f;
    int moistureNoiseOctaves = 5;
    float moistureNoiseStrength = 0.22f;

    float oceanMoistureBonus = 0.10f;
    float altitudeDryingStrength = 0.15f;

    float persistence = 0.5f;
    float lacunarity = 2.0f;
};

struct PlanetTextureStageSettings
{

    int faceResolution = 256;
};

struct PlanetGenerationSettings
{
    unsigned int rootSeed = 0;
    unsigned int plateSeed = 0;

    unsigned int terrainSeed = 0;
    unsigned int climateSeed = 0;

    float planetRadius = 2.0f;

    PlanetPlateStageSettings plates;

    PlanetTerrainStageSettings terrain;
    PlanetClimateStageSettings climate;
    PlanetTextureStageSettings textures;
};

inline PlanetGenerationSettings GeneratePlanetSettings(
    unsigned int rootSeed
)
{
    PlanetGenerationSettings settings;

    settings.rootSeed = rootSeed;

    settings.plateSeed =
        PlanetHashSeed(
            rootSeed,
            0xA341316Cu
        );

    PlanetRandom random(rootSeed);

    settings.planetRadius = 2.0f;

    settings.plates.pointAmount =
        random.RangeInt(900, 1300);

    settings.plates.largePlateAmount =
        random.RangeInt(9, 16);

    settings.plates.pointRadiusOffset =
        0.05f;

    settings.plates.pointDisplacementStrength =
        random.RangeFloat(0.04f, 0.09f);

    settings.plates.driftStepAmount =
        random.RangeInt(20, 42);

    settings.plates.provisionalLargeEdgeNeighborCheckCount =
        random.RangeInt(6, 9);

    settings.plates.finalEdgePointNeighborCheckCount =
        settings.plates.provisionalLargeEdgeNeighborCheckCount;

    settings.plates.smallPlateAmount =
        random.RangeInt(3, 7);

    settings.plates.smallPlateMinPoints =
        random.RangeInt(5, 8);

    settings.plates.smallPlateMaxPoints =
        random.RangeInt(14, 24);

    settings.plates.largePlateContinentalChance =
        random.RangeFloat(0.30f, 0.45f);

    settings.plates.smallPlateContinentalChance =
        random.RangeFloat(0.18f, 0.35f);

    settings.terrainSeed =
        PlanetHashSeed(
            rootSeed,
            0xC8013EA4u
        );

    settings.climateSeed =
        PlanetHashSeed(
            rootSeed,
            0xAD90777Du
        );

    settings.terrain.continentFrequency =
        random.RangeFloat(1.0f, 1.6f);

    settings.terrain.oceanLevel =
        random.RangeFloat(0.42f, 0.52f);

    settings.terrain.ridgeStrength =
        random.RangeFloat(0.20f, 0.34f);

    settings.climate.axialTiltDegrees =
        random.RangeFloat(0.0f, 35.0f);

    settings.climate.sunStrength =
        random.RangeFloat(0.85f, 1.15f);

    return settings;
}
