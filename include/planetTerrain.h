#pragma once

#include "raylib.h"
#include "raymath.h"

#include "helperFunc.h"
#include "planetSettings.h"
#include "planetPlates.h"

#include "perlinnoise.h"

#include <vector>
#include <cmath>

struct PlanetSurfaceMap
{
    bool valid = false;
    bool hasClimate = false;

    int faceResolution = 0;
    int width = 0;
    int height = 0;

    std::vector<float> elevation;
    std::vector<int> plateIds;

    std::vector<float> temperature01;
    std::vector<float> moisture01;

    float minElevation = 0.0f;
    float maxElevation = 0.0f;
};

struct PlanetTerrainSampler
{
    bool valid = false;

    float pointRadius = 1.0f;

    PlanetTerrainStageSettings settings;

    PerlinNoise noise;

    std::vector<PlanetPlatePointGroup> plateGroups;
    std::vector<Vector3> plateNoiseOffsets;
    std::vector<float> plateCrustBias;
};

inline Vector3 PlanetTerrainPlateNoiseOffset(
    int plateId,
    unsigned int terrainSeed,
    float offsetDistance
)
{
    PlanetRandom random(
        PlanetHashSeed(
            terrainSeed,
            0x9E3779B9u * (unsigned int)(plateId + 1)
        )
    );

    Vector3 direction = {
        random.RangeFloat(-1.0f, 1.0f),
        random.RangeFloat(-1.0f, 1.0f),
        random.RangeFloat(-1.0f, 1.0f)
    };

    direction = PlanetSafeNormalize(
        direction,
        Vector3{ 1.0f, 0.0f, 0.0f }
    );

    return Vector3Scale(direction, offsetDistance);
}

inline PlanetTerrainSampler BuildPlanetTerrainSampler(
    const PlanetPlateMap& plateMap,
    const PlanetGenerationSettings& settings
)
{
    PlanetTerrainSampler sampler;

    sampler.settings = settings.terrain;
    sampler.noise = PerlinNoise(settings.terrainSeed);
    sampler.pointRadius = plateMap.pointRadius;

    if (sampler.settings.maxBlendPlates < 1)
    {
        sampler.settings.maxBlendPlates = 1;
    }

    if (
        !plateMap.valid ||
        plateMap.points.empty() ||
        plateMap.finalPlates.empty() ||
        plateMap.finalPlateIds.size() != plateMap.points.size()
        )
    {
        return sampler;
    }

    int plateCount = (int)plateMap.finalPlates.size();

    sampler.plateGroups = BuildPlanetPlatePointGroups(
        plateMap.points,
        plateMap.finalPlateIds,
        plateCount
    );

    sampler.plateNoiseOffsets.resize(plateCount);
    sampler.plateCrustBias.resize(plateCount);

    for (int plateId = 0; plateId < plateCount; plateId++)
    {
        sampler.plateNoiseOffsets[plateId] = PlanetTerrainPlateNoiseOffset(
            plateId,
            settings.terrainSeed,
            sampler.settings.plateNoiseOffsetDistance
        );

        bool continental =
            plateMap.finalPlates[plateId].crustType == PLANET_CRUST_CONTINENTAL;

        sampler.plateCrustBias[plateId] = continental
            ? sampler.settings.continentalCrustBias
            : sampler.settings.oceanicCrustBias;
    }

    sampler.valid = true;

    return sampler;
}

inline float SamplePlanetTerrainPlateValue01(
    const PlanetTerrainSampler& sampler,
    Vector3 normal,
    int plateId
)
{
    const PlanetTerrainStageSettings& s = sampler.settings;
    Vector3 offset = sampler.plateNoiseOffsets[plateId];

    float continent = sampler.noise.fbm01(
        normal.x * s.continentFrequency + offset.x,
        normal.y * s.continentFrequency + offset.y,
        normal.z * s.continentFrequency + offset.z,
        s.continentOctaves,
        s.persistence,
        s.lacunarity
    );

    continent = PlanetClamp01(continent + sampler.plateCrustBias[plateId]);

    float detail = sampler.noise.fbm01(
        normal.x * s.detailFrequency + offset.x,
        normal.y * s.detailFrequency + offset.y,
        normal.z * s.detailFrequency + offset.z,
        s.detailOctaves,
        s.persistence,
        s.lacunarity
    ) - 0.5f;

    float landMask = PlanetSmoothStep01(
        PlanetInverseLerp01(s.oceanLevel, 1.0f, continent)
    );

    float ridge = 0.0f;

    if (landMask > 0.0f)
    {
        ridge = sampler.noise.ridged01(
            normal.x * s.ridgeFrequency + offset.x,
            normal.y * s.ridgeFrequency + offset.y,
            normal.z * s.ridgeFrequency + offset.z,
            s.ridgeOctaves,
            s.persistence,
            s.lacunarity
        );
    }

    float value =
        continent +
        detail * s.detailStrength +
        ridge * s.ridgeStrength * landMask;

    return PlanetClamp01(value);
}

inline float SamplePlanetTerrainValue01(
    const PlanetTerrainSampler& sampler,
    Vector3 direction,
    std::vector<PlanetPlateDistanceResult>& closestScratch,
    int& outPlateId
)
{
    outPlateId = -1;

    if (!sampler.valid)
    {
        return 0.0f;
    }

    Vector3 normal = PlanetSafeNormalize(
        direction,
        Vector3{ 0.0f, 1.0f, 0.0f }
    );

    Vector3 plateSamplePoint = Vector3Scale(normal, sampler.pointRadius);

    FindClosestPlanetPlates(
        plateSamplePoint,
        sampler.plateGroups,
        sampler.settings.maxBlendPlates,
        closestScratch
    );

    if (closestScratch.empty())
    {
        return 0.0f;
    }

    outPlateId = closestScratch[0].plateId;

    float blendWidth = sampler.settings.plateBlendWidth;

    if (blendWidth <= PLANET_EPSILON || closestScratch.size() == 1)
    {
        return SamplePlanetTerrainPlateValue01(sampler, normal, outPlateId);
    }

    float closestDistance = closestScratch[0].distance;

    float blendedValue = 0.0f;
    float totalWeight = 0.0f;

    for (const PlanetPlateDistanceResult& result : closestScratch)
    {
        float distanceDifference = result.distance - closestDistance;

        if (distanceDifference > blendWidth)
        {
            break;
        }

        float weight = PlanetSmoothStep01(1.0f - distanceDifference / blendWidth);

        if (weight <= PLANET_EPSILON)
        {
            continue;
        }

        blendedValue +=
            SamplePlanetTerrainPlateValue01(sampler, normal, result.plateId) *
            weight;

        totalWeight += weight;
    }

    if (totalWeight <= PLANET_EPSILON)
    {
        return SamplePlanetTerrainPlateValue01(sampler, normal, outPlateId);
    }

    return PlanetClamp01(blendedValue / totalWeight);
}

inline float PlanetTerrainElevationFromValue(
    float value,
    const PlanetTerrainStageSettings& settings
)
{
    value = PlanetClamp01(value);

    if (value < settings.oceanLevel)
    {
        float ocean01 = PlanetInverseLerp01(settings.oceanLevel, 0.0f, value);
        return -settings.oceanDepth * ocean01;
    }

    float land01 = PlanetInverseLerp01(settings.oceanLevel, 1.0f, value);
    land01 = powf(land01, settings.landPower);

    return settings.landHeight * land01;
}

inline PlanetSurfaceMap GeneratePlanetTerrainSurface(
    const PlanetPlateMap& plateMap,
    const PlanetGenerationSettings& settings
)
{
    PlanetSurfaceMap surface;

    int faceResolution = settings.textures.faceResolution;

    if (faceResolution < 2)
    {
        faceResolution = 2;
    }

    surface.faceResolution = faceResolution;
    surface.width = faceResolution * 3;
    surface.height = faceResolution * 2;

    int texelCount = surface.width * surface.height;

    surface.elevation.assign(texelCount, 0.0f);
    surface.plateIds.assign(texelCount, -1);

    PlanetTerrainSampler sampler = BuildPlanetTerrainSampler(
        plateMap,
        settings
    );

    if (!sampler.valid)
    {
        return surface;
    }

    PlanetParallelForRows(
        surface.height,
        [&](int pixelY)
        {
            std::vector<PlanetPlateDistanceResult> closestScratch;
            closestScratch.reserve(sampler.settings.maxBlendPlates + 1);

            for (int pixelX = 0; pixelX < surface.width; pixelX++)
            {
                Vector3 direction = PlanetAtlasPixelToSpherePoint(
                    pixelX,
                    pixelY,
                    faceResolution,
                    1.0f
                );

                int plateId = -1;

                float value = SamplePlanetTerrainValue01(
                    sampler,
                    direction,
                    closestScratch,
                    plateId
                );

                int index = pixelY * surface.width + pixelX;

                surface.elevation[index] = PlanetTerrainElevationFromValue(
                    value,
                    sampler.settings
                );

                surface.plateIds[index] = plateId;
            }
        }
    );

    surface.minElevation = surface.elevation[0];
    surface.maxElevation = surface.elevation[0];

    for (float elevation : surface.elevation)
    {
        surface.minElevation = fminf(surface.minElevation, elevation);
        surface.maxElevation = fmaxf(surface.maxElevation, elevation);
    }

    surface.valid = true;

    return surface;
}

inline float SamplePlanetSurfaceElevation(
    const PlanetSurfaceMap& surface,
    Vector3 direction
)
{
    int face = 0;
    float localU = 0.0f;
    float localV = 0.0f;

    PlanetSpherePointToAtlasFaceUV(direction, face, localU, localV);

    return PlanetSampleAtlasGridBilinear(
        surface.elevation,
        surface.faceResolution,
        face,
        localU,
        localV
    );
}

inline bool ApplyPlanetSurfaceToMesh(
    Mesh& mesh,
    const PlanetSurfaceMap& surface,
    const PlanetGenerationSettings& settings
)
{
    if (!surface.valid || mesh.vertices == nullptr || mesh.vertexCount <= 0)
    {
        return false;
    }

    float baseRadius = settings.planetRadius;
    float elevationScale = settings.terrain.meshElevationScale;

    for (int vertexIndex = 0; vertexIndex < mesh.vertexCount; vertexIndex++)
    {
        int offset = vertexIndex * 3;

        Vector3 position = {
            mesh.vertices[offset + 0],
            mesh.vertices[offset + 1],
            mesh.vertices[offset + 2]
        };

        if (Vector3Length(position) < PLANET_EPSILON)
        {
            continue;
        }

        Vector3 normal = Vector3Normalize(position);

        float elevation = SamplePlanetSurfaceElevation(surface, normal);
        elevation = fmaxf(elevation, 0.0f);

        Vector3 displaced = Vector3Scale(
            normal,
            baseRadius + elevation * elevationScale
        );

        mesh.vertices[offset + 0] = displaced.x;
        mesh.vertices[offset + 1] = displaced.y;
        mesh.vertices[offset + 2] = displaced.z;

        if (mesh.normals != nullptr)
        {
            mesh.normals[offset + 0] = normal.x;
            mesh.normals[offset + 1] = normal.y;
            mesh.normals[offset + 2] = normal.z;
        }
    }

    return true;
}
