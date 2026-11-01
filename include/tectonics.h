#pragma once

#include "raylib.h"
#include "raymath.h"

#include "cubesphere.h"
#include "plates.h"

#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>

enum PlateBoundaryType
{
    PLATE_BOUNDARY_UNKNOWN = 0,
    PLATE_BOUNDARY_CONVERGENT,
    PLATE_BOUNDARY_DIVERGENT,
    PLATE_BOUNDARY_TRANSFORM
};

struct PlateMotion
{

    Vector3 rotationAxis = { 0.0f, 1.0f, 0.0f };

    float speed = 1.0f;

    Vector3 center = { 0.0f, 1.0f, 0.0f };
};

struct PlateBoundaryPoint
{
    bool valid = false;

    int pointIndex = -1;
    int otherPointIndex = -1;

    int plateA = -1;
    int plateB = -1;

    Vector3 position = { 0.0f, 0.0f, 0.0f };
    Vector3 otherPosition = { 0.0f, 0.0f, 0.0f };

    Vector3 velocityA = { 0.0f, 0.0f, 0.0f };
    Vector3 velocityB = { 0.0f, 0.0f, 0.0f };

    PlateBoundaryType type = PLATE_BOUNDARY_UNKNOWN;

    float convergentStress = 0.0f;
    float divergentStress = 0.0f;
    float transformStress = 0.0f;

    float finalStress = 0.0f;
};

struct PlanetTectonicSettings
{

    int edgeNeighborCheckCount = 8;

    float minPlateSpeed = 0.45f;
    float maxPlateSpeed = 1.0f;

    float minClassificationStress = 0.08f;

    int smoothingIterations = 2;
    int smoothingNeighborCheckCount = 10;
    float smoothingDistance = 0.35f;

    float drawRadiusOffset = 0.08f;
    float minDrawPointRadius = 0.015f;
    float maxDrawPointRadius = 0.055f;

    bool drawNeighborLines = true;

    float textureBorderWidth = 0.4f;

    int textureBoundarySampleCount = 12;

    Color textureBackgroundColor = { 8, 8, 8, 255 };

    float heightMapBorderWidth = 0.16f;

    int heightMapClosestPlateCount = 4;

    float heightMapInfluence = 0.85f;

    float heightMapNeutralValue = 0.5f;
    float heightMapConvergentValue = 1.0f;
    float heightMapDivergentValue = 0.0f;
    float heightMapTransformValue = 0.5f;
};

struct PlanetTectonicData
{
    bool valid = false;

    std::vector<int> finalPlateIds;
    std::vector<bool> edgePoints;

    std::vector<PlateMotion> plateMotions;
    std::vector<PlateBoundaryPoint> boundaryPoints;

    std::vector<int> boundaryPointIndexByPointIndex;

    int finalPlateCount = 0;
};

inline float TectonicClamp01(float value)
{
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

inline float TectonicLerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

inline float TectonicSmoothStep01(float value)
{
    value = TectonicClamp01(value);
    return value * value * (3.0f - 2.0f * value);
}

inline Color TectonicLerpColor(Color a, Color b, float t)
{
    t = TectonicClamp01(t);

    Color result;
    result.r = (unsigned char)TectonicLerp((float)a.r, (float)b.r, t);
    result.g = (unsigned char)TectonicLerp((float)a.g, (float)b.g, t);
    result.b = (unsigned char)TectonicLerp((float)a.b, (float)b.b, t);
    result.a = 255;

    return result;
}

inline std::uint32_t TectonicHash3D(
    int x,
    int y,
    int z,
    unsigned int seed
)
{
    std::uint32_t h = 2166136261u;

    h = (h ^ ((std::uint32_t)x * 374761393u)) * 16777619u;
    h = (h ^ ((std::uint32_t)y * 668265263u)) * 16777619u;
    h = (h ^ ((std::uint32_t)z * 2147483647u)) * 16777619u;
    h = (h ^ ((std::uint32_t)seed * 60493u)) * 16777619u;

    h ^= h >> 13;
    h *= 1274126177u;
    h ^= h >> 16;

    return h;
}

inline float TectonicHashToFloat01(std::uint32_t hash)
{
    return (float)(hash & 0xFFFFFFu) / (float)0xFFFFFFu;
}

inline Vector3 TectonicProjectOntoTangentPlane(
    Vector3 vector,
    Vector3 surfaceNormal
)
{
    surfaceNormal = Vector3Normalize(surfaceNormal);

    float dot = Vector3DotProduct(vector, surfaceNormal);

    Vector3 projected = Vector3Subtract(
        vector,
        Vector3Scale(surfaceNormal, dot)
    );

    return projected;
}

inline Vector3 TectonicSafeNormalize(Vector3 vector, Vector3 fallback)
{
    if (Vector3Length(vector) < 0.0001f)
    {
        return Vector3Normalize(fallback);
    }

    return Vector3Normalize(vector);
}

inline int TectonicPairMin(int a, int b)
{
    return a < b ? a : b;
}

inline int TectonicPairMax(int a, int b)
{
    return a > b ? a : b;
}

inline bool TectonicSamePlatePair(
    int a0,
    int b0,
    int a1,
    int b1
)
{
    return
        TectonicPairMin(a0, b0) == TectonicPairMin(a1, b1) &&
        TectonicPairMax(a0, b0) == TectonicPairMax(a1, b1);
}

inline Vector3 TectonicRandomUnitVector(
    int id,
    unsigned int seed
)
{
    float x = TectonicHashToFloat01(
        TectonicHash3D(id, 11, 17, seed)
    ) * 2.0f - 1.0f;

    float y = TectonicHashToFloat01(
        TectonicHash3D(id, 23, 31, seed)
    ) * 2.0f - 1.0f;

    float z = TectonicHashToFloat01(
        TectonicHash3D(id, 41, 47, seed)
    ) * 2.0f - 1.0f;

    Vector3 result = { x, y, z };

    if (Vector3Length(result) < 0.0001f)
    {
        result = { 0.0f, 1.0f, 0.0f };
    }

    return Vector3Normalize(result);
}

inline Vector3 GetPlateVelocityAtPoint(
    const PlateMotion& motion,
    Vector3 spherePoint
)
{
    Vector3 normal = Vector3Normalize(spherePoint);

    Vector3 velocity = Vector3CrossProduct(
        motion.rotationAxis,
        normal
    );

    if (Vector3Length(velocity) < 0.0001f)
    {
        Vector3 fallbackAxis = { 0.0f, 1.0f, 0.0f };

        if (fabsf(Vector3DotProduct(fallbackAxis, normal)) > 0.95f)
        {
            fallbackAxis = { 1.0f, 0.0f, 0.0f };
        }

        velocity = Vector3CrossProduct(
            fallbackAxis,
            normal
        );
    }

    velocity = Vector3Normalize(velocity);
    velocity = Vector3Scale(velocity, motion.speed);

    return velocity;
}

inline std::vector<Vector3> BuildPlateCenters(
    const std::vector<Vector3>& points,
    const std::vector<int>& finalPlateIds,
    int finalPlateCount
)
{
    std::vector<Vector3> centers;
    centers.resize(finalPlateCount, Vector3{ 0.0f, 0.0f, 0.0f });

    std::vector<int> counts;
    counts.resize(finalPlateCount, 0);

    for (int i = 0; i < (int)points.size(); i++)
    {
        int plateId = finalPlateIds[i];

        if (plateId < 0 || plateId >= finalPlateCount)
        {
            continue;
        }

        Vector3 normal = Vector3Normalize(points[i]);

        centers[plateId] = Vector3Add(
            centers[plateId],
            normal
        );

        counts[plateId]++;
    }

    for (int plateId = 0; plateId < finalPlateCount; plateId++)
    {
        if (counts[plateId] > 0)
        {
            centers[plateId] = Vector3Normalize(centers[plateId]);
        }
        else
        {
            centers[plateId] = { 0.0f, 1.0f, 0.0f };
        }
    }

    return centers;
}

inline std::vector<PlateMotion> GeneratePlateMotions(
    const std::vector<Vector3>& points,
    const std::vector<int>& finalPlateIds,
    int finalPlateCount,
    const PlanetTectonicSettings& settings,
    unsigned int seed
)
{
    std::vector<PlateMotion> motions;
    motions.resize(finalPlateCount);

    std::vector<Vector3> centers = BuildPlateCenters(
        points,
        finalPlateIds,
        finalPlateCount
    );

    for (int plateId = 0; plateId < finalPlateCount; plateId++)
    {
        float speedT = TectonicHashToFloat01(
            TectonicHash3D(plateId, 61, 73, seed)
        );

        PlateMotion motion;
        motion.rotationAxis = TectonicRandomUnitVector(plateId, seed);
        motion.speed = TectonicLerp(
            settings.minPlateSpeed,
            settings.maxPlateSpeed,
            speedT
        );
        motion.center = centers[plateId];

        motions[plateId] = motion;
    }

    return motions;
}

inline PlateBoundaryPoint BuildRawBoundaryPoint(
    int pointIndex,
    int otherPointIndex,
    const std::vector<Vector3>& points,
    const std::vector<int>& finalPlateIds,
    const std::vector<PlateMotion>& plateMotions,
    const PlanetTectonicSettings& settings
)
{
    PlateBoundaryPoint result;

    if (
        pointIndex < 0 ||
        pointIndex >= (int)points.size() ||
        otherPointIndex < 0 ||
        otherPointIndex >= (int)points.size()
        )
    {
        return result;
    }

    int plateA = finalPlateIds[pointIndex];
    int plateB = finalPlateIds[otherPointIndex];

    if (plateA < 0 || plateB < 0 || plateA == plateB)
    {
        return result;
    }

    if (
        plateA >= (int)plateMotions.size() ||
        plateB >= (int)plateMotions.size()
        )
    {
        return result;
    }

    Vector3 positionA = points[pointIndex];
    Vector3 positionB = points[otherPointIndex];

    Vector3 surfaceNormal = Vector3Normalize(
        Vector3Add(positionA, positionB)
    );

    if (Vector3Length(surfaceNormal) < 0.0001f)
    {
        surfaceNormal = Vector3Normalize(positionA);
    }

    Vector3 directionAToB = Vector3Subtract(
        positionB,
        positionA
    );

    directionAToB = TectonicProjectOntoTangentPlane(
        directionAToB,
        surfaceNormal
    );

    directionAToB = TectonicSafeNormalize(
        directionAToB,
        Vector3Subtract(Vector3Normalize(positionB), Vector3Normalize(positionA))
    );

    Vector3 boundaryTangent = Vector3CrossProduct(
        surfaceNormal,
        directionAToB
    );

    boundaryTangent = TectonicSafeNormalize(
        boundaryTangent,
        Vector3{ 1.0f, 0.0f, 0.0f }
    );

    Vector3 velocityA = GetPlateVelocityAtPoint(
        plateMotions[plateA],
        positionA
    );

    Vector3 velocityB = GetPlateVelocityAtPoint(
        plateMotions[plateB],
        positionB
    );

    Vector3 relativeVelocity = Vector3Subtract(
        velocityA,
        velocityB
    );

    float closing = Vector3DotProduct(
        relativeVelocity,
        directionAToB
    );

    float shear = fabsf(
        Vector3DotProduct(relativeVelocity, boundaryTangent)
    );

    float stressNormalizer = settings.maxPlateSpeed * 2.0f;

    if (stressNormalizer <= 0.0001f)
    {
        stressNormalizer = 1.0f;
    }

    float convergentStress = TectonicClamp01(
        closing / stressNormalizer
    );

    float divergentStress = TectonicClamp01(
        -closing / stressNormalizer
    );

    float transformStress = TectonicClamp01(
        shear / stressNormalizer
    );

    PlateBoundaryType type = PLATE_BOUNDARY_UNKNOWN;
    float finalStress = 0.0f;

    if (
        convergentStress >= divergentStress &&
        convergentStress >= transformStress &&
        convergentStress >= settings.minClassificationStress
        )
    {
        type = PLATE_BOUNDARY_CONVERGENT;
        finalStress = convergentStress;
    }
    else if (
        divergentStress >= convergentStress &&
        divergentStress >= transformStress &&
        divergentStress >= settings.minClassificationStress
        )
    {
        type = PLATE_BOUNDARY_DIVERGENT;
        finalStress = divergentStress;
    }
    else if (transformStress >= settings.minClassificationStress)
    {
        type = PLATE_BOUNDARY_TRANSFORM;
        finalStress = transformStress;
    }

    result.valid = true;

    result.pointIndex = pointIndex;
    result.otherPointIndex = otherPointIndex;

    result.plateA = plateA;
    result.plateB = plateB;

    result.position = positionA;
    result.otherPosition = positionB;

    result.velocityA = velocityA;
    result.velocityB = velocityB;

    result.type = type;

    result.convergentStress = convergentStress;
    result.divergentStress = divergentStress;
    result.transformStress = transformStress;

    result.finalStress = finalStress;

    return result;
}

inline void UpdateBoundaryTypeFromStress(
    PlateBoundaryPoint& point,
    const PlanetTectonicSettings& settings
)
{
    point.type = PLATE_BOUNDARY_UNKNOWN;
    point.finalStress = 0.0f;

    if (
        point.convergentStress >= point.divergentStress &&
        point.convergentStress >= point.transformStress &&
        point.convergentStress >= settings.minClassificationStress
        )
    {
        point.type = PLATE_BOUNDARY_CONVERGENT;
        point.finalStress = point.convergentStress;
        return;
    }

    if (
        point.divergentStress >= point.convergentStress &&
        point.divergentStress >= point.transformStress &&
        point.divergentStress >= settings.minClassificationStress
        )
    {
        point.type = PLATE_BOUNDARY_DIVERGENT;
        point.finalStress = point.divergentStress;
        return;
    }

    if (point.transformStress >= settings.minClassificationStress)
    {
        point.type = PLATE_BOUNDARY_TRANSFORM;
        point.finalStress = point.transformStress;
        return;
    }
}

inline void SmoothTectonicBoundaryStress(
    const std::vector<Vector3>& points,
    const KDTree& pointTree,
    PlanetTectonicData& tectonicData,
    const PlanetTectonicSettings& settings
)
{
    if (tectonicData.boundaryPoints.empty())
    {
        return;
    }

    int neighborCount = settings.smoothingNeighborCheckCount;

    if (neighborCount < 1)
    {
        neighborCount = 1;
    }

    std::vector<int> nearestPointIndices;
    nearestPointIndices.reserve(neighborCount + 1);

    for (int iteration = 0; iteration < settings.smoothingIterations; iteration++)
    {
        std::vector<PlateBoundaryPoint> smoothed = tectonicData.boundaryPoints;

        for (int boundaryIndex = 0;
            boundaryIndex < (int)tectonicData.boundaryPoints.size();
            boundaryIndex++)
        {
            const PlateBoundaryPoint& current =
                tectonicData.boundaryPoints[boundaryIndex];

            if (!current.valid)
            {
                continue;
            }

            FindKNearestKDTreePoints(
                pointTree,
                current.position,
                neighborCount + 1,
                nearestPointIndices
            );

            float totalWeight = 1.0f;

            float convergentTotal = current.convergentStress;
            float divergentTotal = current.divergentStress;
            float transformTotal = current.transformStress;

            for (int neighborPointIndex : nearestPointIndices)
            {
                if (
                    neighborPointIndex < 0 ||
                    neighborPointIndex >= (int)tectonicData.boundaryPointIndexByPointIndex.size()
                    )
                {
                    continue;
                }

                int neighborBoundaryIndex =
                    tectonicData.boundaryPointIndexByPointIndex[neighborPointIndex];

                if (
                    neighborBoundaryIndex < 0 ||
                    neighborBoundaryIndex >= (int)tectonicData.boundaryPoints.size()
                    )
                {
                    continue;
                }

                if (neighborBoundaryIndex == boundaryIndex)
                {
                    continue;
                }

                const PlateBoundaryPoint& neighbor =
                    tectonicData.boundaryPoints[neighborBoundaryIndex];

                if (!neighbor.valid)
                {
                    continue;
                }

                if (
                    !TectonicSamePlatePair(
                        current.plateA,
                        current.plateB,
                        neighbor.plateA,
                        neighbor.plateB
                    )
                    )
                {
                    continue;
                }

                float distance = Vector3Distance(
                    current.position,
                    neighbor.position
                );

                if (distance > settings.smoothingDistance)
                {
                    continue;
                }

                float t = 1.0f - distance / settings.smoothingDistance;
                float weight = TectonicSmoothStep01(t);

                convergentTotal += neighbor.convergentStress * weight;
                divergentTotal += neighbor.divergentStress * weight;
                transformTotal += neighbor.transformStress * weight;

                totalWeight += weight;
            }

            if (totalWeight <= 0.0001f)
            {
                continue;
            }

            smoothed[boundaryIndex].convergentStress =
                TectonicClamp01(convergentTotal / totalWeight);

            smoothed[boundaryIndex].divergentStress =
                TectonicClamp01(divergentTotal / totalWeight);

            smoothed[boundaryIndex].transformStress =
                TectonicClamp01(transformTotal / totalWeight);

            UpdateBoundaryTypeFromStress(
                smoothed[boundaryIndex],
                settings
            );
        }

        tectonicData.boundaryPoints = smoothed;
    }
}

inline PlanetTectonicData GeneratePlanetTectonicData(
    const PlanetPlateData& plateData,
    const PlanetTectonicSettings& settings,
    unsigned int seed
)
{
    PlanetTectonicData data;

    if (plateData.points.empty())
    {
        return data;
    }

    data.finalPlateIds = BuildFinalPointPlateIds(
        plateData.points,
        plateData.largeSeeds,
        plateData.largePlateIndices,
        plateData.smallSeeds,
        plateData.smallPlateIndices
    );

    data.finalPlateCount =
        (int)plateData.largeSeeds.size() +
        (int)plateData.smallSeeds.size();

    if (data.finalPlateCount <= 0)
    {
        return data;
    }

    data.edgePoints = DetectPlateEdgePoints(
        plateData.points,
        data.finalPlateIds,
        settings.edgeNeighborCheckCount
    );

    data.plateMotions = GeneratePlateMotions(
        plateData.points,
        data.finalPlateIds,
        data.finalPlateCount,
        settings,
        seed
    );

    data.boundaryPointIndexByPointIndex.resize(
        plateData.points.size(),
        -1
    );

    KDTree pointTree = BuildKDTree(plateData.points);

    std::vector<int> nearestPointIndices;
    nearestPointIndices.reserve(settings.edgeNeighborCheckCount + 1);

    for (int pointIndex = 0; pointIndex < (int)plateData.points.size(); pointIndex++)
    {
        if (
            pointIndex >= (int)data.edgePoints.size() ||
            !data.edgePoints[pointIndex]
            )
        {
            continue;
        }

        int plateA = data.finalPlateIds[pointIndex];

        if (plateA < 0)
        {
            continue;
        }

        FindKNearestKDTreePoints(
            pointTree,
            plateData.points[pointIndex],
            settings.edgeNeighborCheckCount + 1,
            nearestPointIndices
        );

        int closestDifferentPlatePointIndex = -1;
        float closestDifferentPlateDistanceSqr = 999999999.0f;

        for (int neighborPointIndex : nearestPointIndices)
        {
            if (neighborPointIndex == pointIndex)
            {
                continue;
            }

            if (
                neighborPointIndex < 0 ||
                neighborPointIndex >= (int)data.finalPlateIds.size()
                )
            {
                continue;
            }

            int plateB = data.finalPlateIds[neighborPointIndex];

            if (plateB < 0 || plateB == plateA)
            {
                continue;
            }

            float distanceSqr = Vector3DistanceSqr(
                plateData.points[pointIndex],
                plateData.points[neighborPointIndex]
            );

            if (distanceSqr < closestDifferentPlateDistanceSqr)
            {
                closestDifferentPlateDistanceSqr = distanceSqr;
                closestDifferentPlatePointIndex = neighborPointIndex;
            }
        }

        if (closestDifferentPlatePointIndex < 0)
        {
            continue;
        }

        PlateBoundaryPoint boundaryPoint = BuildRawBoundaryPoint(
            pointIndex,
            closestDifferentPlatePointIndex,
            plateData.points,
            data.finalPlateIds,
            data.plateMotions,
            settings
        );

        if (!boundaryPoint.valid)
        {
            continue;
        }

        int boundaryIndex = (int)data.boundaryPoints.size();

        data.boundaryPoints.push_back(boundaryPoint);
        data.boundaryPointIndexByPointIndex[pointIndex] = boundaryIndex;
    }

    SmoothTectonicBoundaryStress(
        plateData.points,
        pointTree,
        data,
        settings
    );

    data.valid = true;

    return data;
}

inline Color GetPlateBoundaryDebugColor(
    PlateBoundaryType type,
    float stress
)
{
    stress = TectonicClamp01(stress);

    Color weak = Color{ 60, 60, 60, 255 };

    if (type == PLATE_BOUNDARY_CONVERGENT)
    {
        return TectonicLerpColor(
            weak,
            Color{ 255, 70, 30, 255 },
            stress
        );
    }

    if (type == PLATE_BOUNDARY_DIVERGENT)
    {
        return TectonicLerpColor(
            weak,
            Color{ 40, 140, 255, 255 },
            stress
        );
    }

    if (type == PLATE_BOUNDARY_TRANSFORM)
    {
        return TectonicLerpColor(
            weak,
            Color{ 255, 230, 40, 255 },
            stress
        );
    }

    return Color{ 80, 80, 80, 255 };
}

inline void DrawTectonicBoundaryDebug(
    const PlanetTectonicData& tectonicData,
    float drawRadius,
    const PlanetTectonicSettings& settings
)
{
    if (!tectonicData.valid)
    {
        return;
    }

    for (const PlateBoundaryPoint& point : tectonicData.boundaryPoints)
    {
        if (!point.valid)
        {
            continue;
        }

        if (point.type == PLATE_BOUNDARY_UNKNOWN)
        {
            continue;
        }

        float stress = TectonicClamp01(point.finalStress);

        Color color = GetPlateBoundaryDebugColor(
            point.type,
            stress
        );

        float pointRadius = TectonicLerp(
            settings.minDrawPointRadius,
            settings.maxDrawPointRadius,
            stress
        );

        Vector3 drawPosition = Vector3Scale(
            Vector3Normalize(point.position),
            drawRadius
        );

        DrawSphere(
            drawPosition,
            pointRadius,
            color
        );

        if (settings.drawNeighborLines)
        {
            Vector3 otherDrawPosition = Vector3Scale(
                Vector3Normalize(point.otherPosition),
                drawRadius
            );

            DrawLine3D(
                drawPosition,
                otherDrawPosition,
                color
            );
        }
    }
}

inline void DrawPlateMotionDebug(
    const PlanetTectonicData& tectonicData,
    float drawRadius,
    float arrowLength
)
{
    if (!tectonicData.valid)
    {
        return;
    }

    for (const PlateMotion& motion : tectonicData.plateMotions)
    {
        Vector3 centerPosition = Vector3Scale(
            Vector3Normalize(motion.center),
            drawRadius
        );

        Vector3 velocity = GetPlateVelocityAtPoint(
            motion,
            centerPosition
        );

        velocity = Vector3Normalize(velocity);

        Vector3 endPosition = Vector3Add(
            centerPosition,
            Vector3Scale(velocity, arrowLength)
        );

        DrawSphere(centerPosition, 0.045f, WHITE);
        DrawLine3D(centerPosition, endPosition, WHITE);
        DrawSphere(endPosition, 0.025f, WHITE);
    }
}

struct TectonicBoundaryTextureSample
{
    bool valid = false;

    float edgeMask = 0.0f;

    float convergentStress = 0.0f;
    float divergentStress = 0.0f;
    float transformStress = 0.0f;

    float finalStress = 0.0f;

    Color color = { 0, 0, 0, 255 };
};

struct TectonicBoundaryTextureLookup
{
    bool valid = false;

    std::vector<Vector3> boundaryPositions;

    std::vector<int> boundaryPointIndices;

    KDTree boundaryTree;
};

inline Vector3 TectonicAtlasPixelToSpherePoint(
    int pixelX,
    int pixelY,
    int faceResolution,
    float sphereRadius
)
{
    int tileX = pixelX / faceResolution;
    int tileY = pixelY / faceResolution;

    int face = tileY * 3 + tileX;

    float localU =
        (float)(pixelX % faceResolution) /
        (float)(faceResolution - 1);

    float localV =
        (float)(pixelY % faceResolution) /
        (float)(faceResolution - 1);

    float cubeU = localU * 2.0f - 1.0f;
    float cubeV = 1.0f - localV * 2.0f;

    Vector3 cubePoint = CubeFacePoint(face, cubeU, cubeV);

    Vector3 spherePoint = Vector3Normalize(cubePoint);
    spherePoint = Vector3Scale(spherePoint, sphereRadius);

    return spherePoint;
}

struct TectonicPlatePairStress
{
    bool valid = false;

    int plateA = -1;
    int plateB = -1;

    float convergentStress = 0.0f;
    float divergentStress = 0.0f;
    float transformStress = 0.0f;

    float finalStress = 0.0f;

    PlateBoundaryType type = PLATE_BOUNDARY_UNKNOWN;
};

struct TectonicPlatePairStressMap
{
    int finalPlateCount = 0;

    std::vector<TectonicPlatePairStress> pairStress;
};

inline int GetTectonicPairIndex(
    int plateA,
    int plateB,
    int finalPlateCount
)
{
    if (plateA < 0 || plateB < 0)
    {
        return -1;
    }

    if (plateA >= finalPlateCount || plateB >= finalPlateCount)
    {
        return -1;
    }

    return plateA * finalPlateCount + plateB;
}

inline TectonicPlatePairStressMap BuildTectonicPlatePairStressMap(
    const PlanetTectonicData& tectonicData,
    const PlanetTectonicSettings& settings
)
{
    TectonicPlatePairStressMap map;

    map.finalPlateCount = tectonicData.finalPlateCount;

    if (!tectonicData.valid || map.finalPlateCount <= 0)
    {
        return map;
    }

    int pairCount = map.finalPlateCount * map.finalPlateCount;

    map.pairStress.resize(pairCount);

    std::vector<float> totalWeights;
    totalWeights.resize(pairCount, 0.0f);

    for (const PlateBoundaryPoint& point : tectonicData.boundaryPoints)
    {
        if (!point.valid)
        {
            continue;
        }

        int plateA = point.plateA;
        int plateB = point.plateB;

        int indexAB = GetTectonicPairIndex(
            plateA,
            plateB,
            map.finalPlateCount
        );

        int indexBA = GetTectonicPairIndex(
            plateB,
            plateA,
            map.finalPlateCount
        );

        if (indexAB < 0 || indexBA < 0)
        {
            continue;
        }

        float weight = 0.25f + point.finalStress * 0.75f;

        TectonicPlatePairStress& stressAB = map.pairStress[indexAB];
        TectonicPlatePairStress& stressBA = map.pairStress[indexBA];

        stressAB.valid = true;
        stressAB.plateA = plateA;
        stressAB.plateB = plateB;

        stressBA.valid = true;
        stressBA.plateA = plateB;
        stressBA.plateB = plateA;

        stressAB.convergentStress += point.convergentStress * weight;
        stressAB.divergentStress += point.divergentStress * weight;
        stressAB.transformStress += point.transformStress * weight;

        stressBA.convergentStress += point.convergentStress * weight;
        stressBA.divergentStress += point.divergentStress * weight;
        stressBA.transformStress += point.transformStress * weight;

        totalWeights[indexAB] += weight;
        totalWeights[indexBA] += weight;
    }

    for (int i = 0; i < pairCount; i++)
    {
        if (!map.pairStress[i].valid)
        {
            continue;
        }

        float totalWeight = totalWeights[i];

        if (totalWeight <= 0.0001f)
        {
            continue;
        }

        TectonicPlatePairStress& stress = map.pairStress[i];

        stress.convergentStress = TectonicClamp01(
            stress.convergentStress / totalWeight
        );

        stress.divergentStress = TectonicClamp01(
            stress.divergentStress / totalWeight
        );

        stress.transformStress = TectonicClamp01(
            stress.transformStress / totalWeight
        );

        stress.type = PLATE_BOUNDARY_UNKNOWN;
        stress.finalStress = 0.0f;

        if (
            stress.convergentStress >= stress.divergentStress &&
            stress.convergentStress >= stress.transformStress &&
            stress.convergentStress >= settings.minClassificationStress
            )
        {
            stress.type = PLATE_BOUNDARY_CONVERGENT;
            stress.finalStress = stress.convergentStress;
        }
        else if (
            stress.divergentStress >= stress.convergentStress &&
            stress.divergentStress >= stress.transformStress &&
            stress.divergentStress >= settings.minClassificationStress
            )
        {
            stress.type = PLATE_BOUNDARY_DIVERGENT;
            stress.finalStress = stress.divergentStress;
        }
        else if (stress.transformStress >= settings.minClassificationStress)
        {
            stress.type = PLATE_BOUNDARY_TRANSFORM;
            stress.finalStress = stress.transformStress;
        }
    }

    return map;
}

inline TectonicPlatePairStress GetTectonicPlatePairStress(
    const TectonicPlatePairStressMap& map,
    int plateA,
    int plateB
)
{
    TectonicPlatePairStress empty;

    int index = GetTectonicPairIndex(
        plateA,
        plateB,
        map.finalPlateCount
    );

    if (index < 0 || index >= (int)map.pairStress.size())
    {
        return empty;
    }

    return map.pairStress[index];
}

inline Color BlendTectonicBorderColor(
    Color background,
    PlateBoundaryType type,
    float stress,
    float borderMask
)
{
    stress = TectonicClamp01(stress);
    borderMask = TectonicClamp01(borderMask);

    Color boundaryColor = GetPlateBoundaryDebugColor(
        type,
        stress
    );

    float visibility = 0.25f + stress * 0.75f;

    float finalT = borderMask * visibility;

    return TectonicLerpColor(
        background,
        boundaryColor,
        finalT
    );
}

inline Texture2D GenerateTectonicBoundaryDebugTexture(
    int faceResolution,
    float sphereRadius,
    const PlanetPlateData& plateData,
    const PlanetTectonicData& tectonicData,
    const PlanetTectonicSettings& settings
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
        settings.textureBackgroundColor
    );

    if (!tectonicData.valid || plateData.points.empty())
    {
        Texture2D emptyTexture = LoadTextureFromImage(image);
        UnloadImage(image);
        return emptyTexture;
    }

    std::vector<int> finalPlateIds = BuildFinalPointPlateIds(
        plateData.points,
        plateData.largeSeeds,
        plateData.largePlateIndices,
        plateData.smallSeeds,
        plateData.smallPlateIndices
    );

    int finalPlateCount =
        (int)plateData.largeSeeds.size() +
        (int)plateData.smallSeeds.size();

    if (finalPlateCount <= 0)
    {
        Texture2D emptyTexture = LoadTextureFromImage(image);
        UnloadImage(image);
        return emptyTexture;
    }

    std::vector<PlatePointGroup> plateGroups = BuildPlatePointGroups(
        plateData.points,
        finalPlateIds,
        finalPlateCount
    );

    TectonicPlatePairStressMap pairStressMap =
        BuildTectonicPlatePairStressMap(
            tectonicData,
            settings
        );

    const int closestPlateCount = 4;

    std::vector<PlateDistanceResult> closestPlates;
    closestPlates.reserve(closestPlateCount);

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

            FindClosestFinalPlates(
                spherePoint,
                plateGroups,
                closestPlateCount,
                closestPlates
            );

            Color finalColor = settings.textureBackgroundColor;

            if ((int)closestPlates.size() >= 2)
            {
                int closestPlateId = closestPlates[0].plateId;

                float closestDistance = closestPlates[0].distance;

                float totalR = 0.0f;
                float totalG = 0.0f;
                float totalB = 0.0f;
                float totalWeight = 0.0f;

                for (int i = 1; i < (int)closestPlates.size(); i++)
                {
                    int otherPlateId = closestPlates[i].plateId;

                    if (otherPlateId == closestPlateId)
                    {
                        continue;
                    }

                    float distanceDifference =
                        closestPlates[i].distance - closestDistance;

                    if (distanceDifference > settings.textureBorderWidth)
                    {
                        continue;
                    }

                    float borderT =
                        1.0f -
                        distanceDifference / settings.textureBorderWidth;

                    float borderMask = TectonicSmoothStep01(borderT);

                    TectonicPlatePairStress pairStress =
                        GetTectonicPlatePairStress(
                            pairStressMap,
                            closestPlateId,
                            otherPlateId
                        );

                    if (!pairStress.valid)
                    {
                        continue;
                    }

                    if (pairStress.type == PLATE_BOUNDARY_UNKNOWN)
                    {
                        continue;
                    }

                    Color borderColor = GetPlateBoundaryDebugColor(
                        pairStress.type,
                        pairStress.finalStress
                    );

                    float stressVisibility =
                        0.25f + pairStress.finalStress * 0.75f;

                    float weight = borderMask * stressVisibility;

                    totalR += (float)borderColor.r * weight;
                    totalG += (float)borderColor.g * weight;
                    totalB += (float)borderColor.b * weight;

                    totalWeight += weight;
                }

                if (totalWeight > 0.0001f)
                {
                    Color borderColor;
                    borderColor.r = (unsigned char)(totalR / totalWeight);
                    borderColor.g = (unsigned char)(totalG / totalWeight);
                    borderColor.b = (unsigned char)(totalB / totalWeight);
                    borderColor.a = 255;

                    float fadeToBorder = TectonicClamp01(totalWeight);

                    finalColor = TectonicLerpColor(
                        settings.textureBackgroundColor,
                        borderColor,
                        fadeToBorder
                    );
                }
            }

            ImageDrawPixel(
                &image,
                pixelX,
                pixelY,
                finalColor
            );
        }
    }

    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);

    return texture;
}

struct TectonicHeightNoiseSample
{
    bool valid = false;

    float value01 = 0.5f;

    float edgeMask = 0.0f;

    float convergentStress = 0.0f;
    float divergentStress = 0.0f;
    float transformStress = 0.0f;

    float finalStress = 0.0f;

    PlateBoundaryType dominantType = PLATE_BOUNDARY_UNKNOWN;
};

inline float GetTectonicBoundaryHeightTarget01(
    PlateBoundaryType type,
    const PlanetTectonicSettings& settings
)
{
    if (type == PLATE_BOUNDARY_CONVERGENT)
    {
        return settings.heightMapConvergentValue;
    }

    if (type == PLATE_BOUNDARY_DIVERGENT)
    {
        return settings.heightMapDivergentValue;
    }

    if (type == PLATE_BOUNDARY_TRANSFORM)
    {
        return settings.heightMapTransformValue;
    }

    return settings.heightMapNeutralValue;
}

inline Color GetTectonicHeightNoiseColor(
    const TectonicHeightNoiseSample& sample
)
{
    float value = TectonicClamp01(sample.value01);
    float mask = TectonicClamp01(sample.edgeMask);

    Color neutral = { 8, 8, 8, 255 };

    if (!sample.valid || mask <= 0.0001f)
    {
        return neutral;
    }

    Color target;

    if (value > 0.5f)
    {
        float t = (value - 0.5f) * 2.0f;

        target = TectonicLerpColor(
            Color{ 20, 20, 20, 255 },
            Color{ 255, 40, 20, 255 },
            t
        );
    }
    else
    {
        float t = (0.5f - value) * 2.0f;

        target = TectonicLerpColor(
            Color{ 20, 20, 20, 255 },
            Color{ 30, 120, 255, 255 },
            t
        );
    }

    return TectonicLerpColor(
        neutral,
        target,
        mask
    );
}

inline TectonicHeightNoiseSample SampleTectonicHeightNoise(
    Vector3 spherePoint,
    const std::vector<PlatePointGroup>& plateGroups,
    const TectonicPlatePairStressMap& pairStressMap,
    const PlanetTectonicSettings& settings
)
{
    TectonicHeightNoiseSample sample;

    sample.value01 = settings.heightMapNeutralValue;

    if (plateGroups.empty())
    {
        return sample;
    }

    float borderWidth = settings.heightMapBorderWidth;

    if (borderWidth <= 0.0001f)
    {
        return sample;
    }

    int closestPlateCount = settings.heightMapClosestPlateCount;

    if (closestPlateCount < 2)
    {
        closestPlateCount = 2;
    }

    std::vector<PlateDistanceResult> closestPlates;
    closestPlates.reserve(closestPlateCount);

    FindClosestFinalPlates(
        spherePoint,
        plateGroups,
        closestPlateCount,
        closestPlates
    );

    if ((int)closestPlates.size() < 2)
    {
        return sample;
    }

    int closestPlateId = closestPlates[0].plateId;
    float closestDistance = closestPlates[0].distance;

    float totalWeight = 0.0f;
    float valueTotal = 0.0f;

    float convergentTotal = 0.0f;
    float divergentTotal = 0.0f;
    float transformTotal = 0.0f;
    float stressTotal = 0.0f;

    float strongestMask = 0.0f;

    PlateBoundaryType strongestType = PLATE_BOUNDARY_UNKNOWN;
    float strongestTypeWeight = 0.0f;

    for (int i = 1; i < (int)closestPlates.size(); i++)
    {
        int otherPlateId = closestPlates[i].plateId;

        if (otherPlateId == closestPlateId)
        {
            continue;
        }

        float distanceDifference =
            closestPlates[i].distance - closestDistance;

        if (distanceDifference > borderWidth)
        {
            continue;
        }

        float borderT =
            1.0f -
            distanceDifference / borderWidth;

        float borderMask = TectonicSmoothStep01(borderT);

        TectonicPlatePairStress pairStress =
            GetTectonicPlatePairStress(
                pairStressMap,
                closestPlateId,
                otherPlateId
            );

        if (!pairStress.valid)
        {
            continue;
        }

        if (pairStress.type == PLATE_BOUNDARY_UNKNOWN)
        {
            continue;
        }

        float targetValue = GetTectonicBoundaryHeightTarget01(
            pairStress.type,
            settings
        );

        float stressWeight = 0.25f + pairStress.finalStress * 0.75f;
        float weight = borderMask * stressWeight;

        valueTotal += targetValue * weight;

        convergentTotal += pairStress.convergentStress * weight;
        divergentTotal += pairStress.divergentStress * weight;
        transformTotal += pairStress.transformStress * weight;
        stressTotal += pairStress.finalStress * weight;

        totalWeight += weight;

        if (borderMask > strongestMask)
        {
            strongestMask = borderMask;
        }

        if (weight > strongestTypeWeight)
        {
            strongestTypeWeight = weight;
            strongestType = pairStress.type;
        }
    }

    if (totalWeight <= 0.0001f)
    {
        return sample;
    }

    sample.valid = true;

    sample.value01 = TectonicClamp01(valueTotal / totalWeight);
    sample.edgeMask = TectonicClamp01(strongestMask);

    sample.convergentStress = TectonicClamp01(convergentTotal / totalWeight);
    sample.divergentStress = TectonicClamp01(divergentTotal / totalWeight);
    sample.transformStress = TectonicClamp01(transformTotal / totalWeight);
    sample.finalStress = TectonicClamp01(stressTotal / totalWeight);

    sample.dominantType = strongestType;

    return sample;
}

inline Texture2D GenerateTectonicHeightNoiseTexture(
    int faceResolution,
    float sphereRadius,
    const PlanetPlateData& plateData,
    const PlanetTectonicData& tectonicData,
    const PlanetTectonicSettings& settings
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
        Color{ 8, 8, 8, 255 }
    );

    if (!tectonicData.valid || plateData.points.empty())
    {
        Texture2D texture = LoadTextureFromImage(image);
        UnloadImage(image);
        return texture;
    }

    std::vector<int> finalPlateIds = BuildFinalPointPlateIds(
        plateData.points,
        plateData.largeSeeds,
        plateData.largePlateIndices,
        plateData.smallSeeds,
        plateData.smallPlateIndices
    );

    int finalPlateCount =
        (int)plateData.largeSeeds.size() +
        (int)plateData.smallSeeds.size();

    if (finalPlateCount <= 0)
    {
        Texture2D texture = LoadTextureFromImage(image);
        UnloadImage(image);
        return texture;
    }

    std::vector<PlatePointGroup> plateGroups = BuildPlatePointGroups(
        plateData.points,
        finalPlateIds,
        finalPlateCount
    );

    TectonicPlatePairStressMap pairStressMap =
        BuildTectonicPlatePairStressMap(
            tectonicData,
            settings
        );

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

            TectonicHeightNoiseSample sample =
                SampleTectonicHeightNoise(
                    spherePoint,
                    plateGroups,
                    pairStressMap,
                    settings
                );

            Color color = GetTectonicHeightNoiseColor(sample);

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
