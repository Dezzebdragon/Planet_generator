#pragma once

#include "raylib.h"
#include "raymath.h"

#include "cubesphere.h"
#include "plates.h"
#include "perlinnoise.h"
#include "tectonics.h"

#include <vector>
#include <cmath>
#include <algorithm>

inline float TerrainElevationFromNoiseValue(
    float value,
    const PlanetNoiseSettings& settings
)
{
    value = PlanetNoiseClamp01(value);

    if (value < settings.oceanLevel)
    {
        float ocean01 =
            (settings.oceanLevel - value) /
            settings.oceanLevel;

        ocean01 = PlanetNoiseClamp01(ocean01);

        return -settings.oceanDepth * ocean01;
    }

    float land01 =
        (value - settings.oceanLevel) /
        (1.0f - settings.oceanLevel);

    land01 = PlanetNoiseClamp01(land01);
    land01 = powf(land01, settings.landPower);

    return settings.landHeight * land01;
}

struct PlanetTerrainSampler
{
    bool valid = false;

    float baseRadius = 1.0f;
    float plateSampleRadius = 1.0f;

    PlanetNoiseSettings noiseSettings;

    PerlinNoise noise;

    std::vector<int> finalPlateIds;
    std::vector<PlatePointGroup> plateGroups;
    std::vector<Vector3> plateNoiseOffsets;

    int finalPlateCount = 0;
    int maxBlendPlates = 1;

    bool useTectonicHeightLayer = false;

    PlanetTectonicSettings tectonicSettings;
    TectonicPlatePairStressMap tectonicPairStressMap;
};

inline PlanetTerrainSampler BuildPlanetTerrainSampler(
    float baseRadius,
    const PlanetPlateData& plateData,
    const PlanetNoiseSettings& noiseSettings,
    unsigned int seed
)
{
    PlanetTerrainSampler sampler;

    sampler.baseRadius = baseRadius;
    sampler.noiseSettings = noiseSettings;
    sampler.noise = PerlinNoise(seed);

    sampler.plateSampleRadius = plateData.pointRadiusFromCenter;

    if (sampler.plateSampleRadius <= 0.0001f)
    {
        sampler.plateSampleRadius = baseRadius;
    }

    if (plateData.points.empty())
    {
        return sampler;
    }

    sampler.finalPlateIds = BuildFinalPointPlateIds(
        plateData.points,
        plateData.largeSeeds,
        plateData.largePlateIndices,
        plateData.smallSeeds,
        plateData.smallPlateIndices
    );

    sampler.finalPlateCount =
        (int)plateData.largeSeeds.size() +
        (int)plateData.smallSeeds.size();

    if (sampler.finalPlateCount <= 0)
    {
        return sampler;
    }

    sampler.plateGroups = BuildPlatePointGroups(
        plateData.points,
        sampler.finalPlateIds,
        sampler.finalPlateCount
    );

    sampler.plateNoiseOffsets = BuildPlateNoiseOffsets(
        sampler.finalPlateCount,
        seed,
        noiseSettings.plateNoiseOffsetDistance
    );

    sampler.maxBlendPlates = noiseSettings.maxNoiseBlendPlates;

    if (sampler.maxBlendPlates < 1)
    {
        sampler.maxBlendPlates = 1;
    }

    sampler.valid = true;

    return sampler;
}

inline PlanetTerrainSampler BuildPlanetTerrainSamplerWithTectonics(
    float baseRadius,
    const PlanetPlateData& plateData,
    const PlanetNoiseSettings& noiseSettings,
    const PlanetTectonicData& tectonicData,
    const PlanetTectonicSettings& tectonicSettings,
    unsigned int seed
)
{
    PlanetTerrainSampler sampler = BuildPlanetTerrainSampler(
        baseRadius,
        plateData,
        noiseSettings,
        seed
    );

    if (!sampler.valid)
    {
        return sampler;
    }

    if (!tectonicData.valid)
    {
        return sampler;
    }

    sampler.useTectonicHeightLayer = true;
    sampler.tectonicSettings = tectonicSettings;

    sampler.tectonicPairStressMap = BuildTectonicPlatePairStressMap(
        tectonicData,
        tectonicSettings
    );

    return sampler;
}

inline float SamplePlanetTerrainValue01(
    Vector3 sphereDirection,
    const PlanetTerrainSampler& sampler
)
{
    if (!sampler.valid)
    {
        return 0.0f;
    }

    if (Vector3Length(sphereDirection) < 0.0001f)
    {
        return 0.0f;
    }

    Vector3 normal = Vector3Normalize(sphereDirection);

    Vector3 plateSamplePoint = Vector3Scale(
        normal,
        sampler.plateSampleRadius
    );

    std::vector<PlateDistanceResult> closestPlates;
    closestPlates.reserve(sampler.maxBlendPlates);

    FindClosestFinalPlates(
        plateSamplePoint,
        sampler.plateGroups,
        sampler.maxBlendPlates,
        closestPlates
    );

    float baseValue = BlendPlateNoiseByDistance(
        plateSamplePoint,
        closestPlates,
        sampler.plateNoiseOffsets,
        sampler.noise,
        sampler.noiseSettings
    );

    baseValue = PlanetNoiseClamp01(baseValue);

    if (sampler.useTectonicHeightLayer)
    {
        TectonicHeightNoiseSample tectonicSample =
            SampleTectonicHeightNoise(
                plateSamplePoint,
                sampler.plateGroups,
                sampler.tectonicPairStressMap,
                sampler.tectonicSettings
            );

        if (tectonicSample.valid)
        {
            float influence =
                sampler.tectonicSettings.heightMapInfluence *
                tectonicSample.edgeMask;

            influence = TectonicClamp01(influence);

            baseValue = TectonicLerp(
                baseValue,
                tectonicSample.value01,
                influence
            );
        }
    }

    return PlanetNoiseClamp01(baseValue);
}

inline float SamplePlanetTerrainElevation(
    Vector3 sphereDirection,
    const PlanetTerrainSampler& sampler
)
{
    float value = SamplePlanetTerrainValue01(
        sphereDirection,
        sampler
    );

    return TerrainElevationFromNoiseValue(
        value,
        sampler.noiseSettings
    );
}

inline Vector3 DisplacePointByPlanetTerrain(
    Vector3 sphereDirection,
    const PlanetTerrainSampler& sampler,
    float elevationStrength
)
{
    if (Vector3Length(sphereDirection) < 0.0001f)
    {
        return Vector3{ 0.0f, 0.0f, 0.0f };
    }

    Vector3 normal = Vector3Normalize(sphereDirection);

    float elevation = SamplePlanetTerrainElevation(
        normal,
        sampler
    );

    elevation *= elevationStrength;

    float finalRadius = sampler.baseRadius + elevation;

    if (finalRadius < 0.0001f)
    {
        finalRadius = 0.0001f;
    }

    return Vector3Scale(normal, finalRadius);
}

inline bool ApplyPlanetTerrainToMesh(
    Mesh& mesh,
    const PlanetTerrainSampler& sampler,
    float elevationStrength = 1.0f
)
{
    if (mesh.vertices == nullptr)
    {
        return false;
    }

    if (mesh.vertexCount <= 0)
    {
        return false;
    }

    if (!sampler.valid)
    {
        return false;
    }

    for (int vertexIndex = 0; vertexIndex < mesh.vertexCount; vertexIndex++)
    {
        int vertexOffset = vertexIndex * 3;

        Vector3 oldPosition = {
            mesh.vertices[vertexOffset + 0],
            mesh.vertices[vertexOffset + 1],
            mesh.vertices[vertexOffset + 2]
        };

        if (Vector3Length(oldPosition) < 0.0001f)
        {
            continue;
        }

        Vector3 normal = Vector3Normalize(oldPosition);

        Vector3 newPosition = DisplacePointByPlanetTerrain(
            normal,
            sampler,
            elevationStrength
        );

        mesh.vertices[vertexOffset + 0] = newPosition.x;
        mesh.vertices[vertexOffset + 1] = newPosition.y;
        mesh.vertices[vertexOffset + 2] = newPosition.z;

        if (mesh.normals != nullptr)
        {
            mesh.normals[vertexOffset + 0] = normal.x;
            mesh.normals[vertexOffset + 1] = normal.y;
            mesh.normals[vertexOffset + 2] = normal.z;
        }
    }

    return true;
}

inline bool ApplyPlanetTerrainToMesh(
    Mesh& mesh,
    float baseRadius,
    const PlanetPlateData& plateData,
    const PlanetNoiseSettings& noiseSettings,
    unsigned int seed,
    float elevationStrength = 1.0f
)
{
    PlanetTerrainSampler sampler = BuildPlanetTerrainSampler(
        baseRadius,
        plateData,
        noiseSettings,
        seed
    );

    return ApplyPlanetTerrainToMesh(
        mesh,
        sampler,
        elevationStrength
    );
}

inline Mesh GenMeshTerrainCubeSphere(
    float baseRadius,
    int subdivisions,
    const PlanetPlateData& plateData,
    const PlanetNoiseSettings& noiseSettings,
    unsigned int seed,
    float elevationStrength = 1.0f
)
{
    Mesh mesh = GenMeshCubeSphere(
        baseRadius,
        subdivisions
    );

    ApplyPlanetTerrainToMesh(
        mesh,
        baseRadius,
        plateData,
        noiseSettings,
        seed,
        elevationStrength
    );

    return mesh;
}

inline bool ApplyPlanetTerrainToMeshWithTectonics(
    Mesh& mesh,
    float baseRadius,
    const PlanetPlateData& plateData,
    const PlanetNoiseSettings& noiseSettings,
    const PlanetTectonicData& tectonicData,
    const PlanetTectonicSettings& tectonicSettings,
    unsigned int seed,
    float elevationStrength = 1.0f
)
{
    PlanetTerrainSampler sampler = BuildPlanetTerrainSamplerWithTectonics(
        baseRadius,
        plateData,
        noiseSettings,
        tectonicData,
        tectonicSettings,
        seed
    );

    return ApplyPlanetTerrainToMesh(
        mesh,
        sampler,
        elevationStrength
    );
}

inline Mesh GenMeshTectonicTerrainCubeSphere(
    float baseRadius,
    int subdivisions,
    const PlanetPlateData& plateData,
    const PlanetNoiseSettings& noiseSettings,
    const PlanetTectonicData& tectonicData,
    const PlanetTectonicSettings& tectonicSettings,
    unsigned int seed,
    float elevationStrength = 1.0f
)
{
    Mesh mesh = GenMeshCubeSphere(
        baseRadius,
        subdivisions
    );

    ApplyPlanetTerrainToMeshWithTectonics(
        mesh,
        baseRadius,
        plateData,
        noiseSettings,
        tectonicData,
        tectonicSettings,
        seed,
        elevationStrength
    );

    return mesh;
}

struct TerrainTextureColorBand
{

    float minHeight01 = 0.0f;
    float maxHeight01 = 1.0f;

    Color colorA = WHITE;
    Color colorB = WHITE;

    bool smooth = true;
};

struct TerrainTextureSettings
{

    float lowestElevation = -0.12f;
    float highestElevation = 0.38f;

    bool clampHeight = true;

    Color fallbackColor = MAGENTA;

    std::vector<TerrainTextureColorBand> colorBands;
};

inline float TerrainTextureClamp01(float value)
{
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

inline float TerrainTextureLerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

inline Color TerrainTextureLerpColor(Color a, Color b, float t)
{
    t = TerrainTextureClamp01(t);

    Color result;
    result.r = (unsigned char)TerrainTextureLerp((float)a.r, (float)b.r, t);
    result.g = (unsigned char)TerrainTextureLerp((float)a.g, (float)b.g, t);
    result.b = (unsigned char)TerrainTextureLerp((float)a.b, (float)b.b, t);
    result.a = 255;

    return result;
}

inline float NormalizeElevationForTerrainTexture(
    float elevation,
    const TerrainTextureSettings& textureSettings
)
{
    float range =
        textureSettings.highestElevation -
        textureSettings.lowestElevation;

    if (fabsf(range) < 0.0001f)
    {
        return 0.0f;
    }

    float value01 =
        (elevation - textureSettings.lowestElevation) /
        range;

    if (textureSettings.clampHeight)
    {
        value01 = TerrainTextureClamp01(value01);
    }

    return value01;
}

inline Color GetTerrainTextureColorFromBands(
    float height01,
    const TerrainTextureSettings& textureSettings
)
{
    height01 = TerrainTextureClamp01(height01);

    for (const TerrainTextureColorBand& band : textureSettings.colorBands)
    {
        if (height01 < band.minHeight01)
        {
            continue;
        }

        if (height01 > band.maxHeight01)
        {
            continue;
        }

        if (!band.smooth)
        {
            return band.colorA;
        }

        float bandRange = band.maxHeight01 - band.minHeight01;

        if (fabsf(bandRange) < 0.0001f)
        {
            return band.colorA;
        }

        float t =
            (height01 - band.minHeight01) /
            bandRange;

        return TerrainTextureLerpColor(
            band.colorA,
            band.colorB,
            t
        );
    }

    return textureSettings.fallbackColor;
}

inline TerrainTextureSettings CreateDefaultTerrainTextureSettings(
    const PlanetNoiseSettings& noiseSettings
)
{
    TerrainTextureSettings settings;

    settings.lowestElevation = -noiseSettings.oceanDepth;
    settings.highestElevation = noiseSettings.landHeight;
    settings.clampHeight = true;
    settings.fallbackColor = MAGENTA;

    settings.colorBands.clear();

    settings.colorBands.push_back(
        TerrainTextureColorBand{
            0.00f,
            0.22f,
            Color{ 5, 20, 80, 255 },
            Color{ 15, 60, 150, 255 },
            true
        }
    );

    settings.colorBands.push_back(
        TerrainTextureColorBand{
            0.22f,
            0.42f,
            Color{ 15, 60, 150, 255 },
            Color{ 35, 130, 200, 255 },
            true
        }
    );

    settings.colorBands.push_back(
        TerrainTextureColorBand{
            0.42f,
            0.50f,
            Color{ 190, 170, 95, 255 },
            Color{ 220, 205, 140, 255 },
            true
        }
    );

    settings.colorBands.push_back(
        TerrainTextureColorBand{
            0.50f,
            0.68f,
            Color{ 45, 130, 55, 255 },
            Color{ 80, 165, 70, 255 },
            true
        }
    );

    settings.colorBands.push_back(
        TerrainTextureColorBand{
            0.68f,
            0.82f,
            Color{ 120, 105, 65, 255 },
            Color{ 145, 120, 80, 255 },
            true
        }
    );

    settings.colorBands.push_back(
        TerrainTextureColorBand{
            0.82f,
            0.93f,
            Color{ 120, 120, 120, 255 },
            Color{ 190, 190, 190, 255 },
            true
        }
    );

    settings.colorBands.push_back(
        TerrainTextureColorBand{
            0.93f,
            1.00f,
            Color{ 220, 220, 220, 255 },
            Color{ 255, 255, 255, 255 },
            true
        }
    );

    return settings;
}

inline void SortTerrainTextureColorBands(
    TerrainTextureSettings& settings
)
{
    std::sort(
        settings.colorBands.begin(),
        settings.colorBands.end(),
        [](const TerrainTextureColorBand& a, const TerrainTextureColorBand& b)
        {
            return a.minHeight01 < b.minHeight01;
        }
    );
}

inline Texture2D GenerateCombinedTerrainHeightTexture(
    int faceResolution,
    float sphereRadius,
    float baseRadius,

    const PlanetPlateData& plateData,

    const PlanetNoiseSettings& noiseSettings,

    const PlanetTectonicData& tectonicData,
    const PlanetTectonicSettings& tectonicSettings,

    const TerrainTextureSettings& inputTextureSettings,

    unsigned int seed
)
{
    if (faceResolution < 2)
    {
        faceResolution = 2;
    }

    int atlasWidth = faceResolution * 3;
    int atlasHeight = faceResolution * 2;

    TerrainTextureSettings textureSettings = inputTextureSettings;
    SortTerrainTextureColorBands(textureSettings);

    Image image = GenImageColor(
        atlasWidth,
        atlasHeight,
        textureSettings.fallbackColor
    );

    PlanetTerrainSampler sampler = BuildPlanetTerrainSamplerWithTectonics(
        baseRadius,
        plateData,
        noiseSettings,
        tectonicData,
        tectonicSettings,
        seed
    );

    if (!sampler.valid)
    {
        Texture2D texture = LoadTextureFromImage(image);
        UnloadImage(image);
        return texture;
    }

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

            float terrainValue01 = SamplePlanetTerrainValue01(
                spherePoint,
                sampler
            );

            float elevation = TerrainElevationFromNoiseValue(
                terrainValue01,
                noiseSettings
            );

            float height01 = NormalizeElevationForTerrainTexture(
                elevation,
                textureSettings
            );

            Color color = GetTerrainTextureColorFromBands(
                height01,
                textureSettings
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
