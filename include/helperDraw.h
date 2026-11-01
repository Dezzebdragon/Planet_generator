#pragma once

#include "planetMovement.h"
#include "planetEdges.h"
#include "raylib.h"
#include "raymath.h"

#include <vector>
#include <cmath>

#ifdef PLANET_DEBUG_PHASE_1

struct PlanetPlateDebugDrawSettings
{
    bool drawPlatePoints = true;
    bool drawLargeSeeds = true;
    bool drawSmallSeeds = true;
    bool drawEdgePoints = false;
    bool drawDriftEvents = false;

    bool generateVoronoiTextureOnStartup = false;

    int voronoiFaceResolution = 256;
    float voronoiBlendWidth = 0.2f;
    int voronoiMaxBlendPlates = 4;

    float normalPointRadius = 0.018f;
    float smallPlatePointRadius = 0.030f;
    float largeSeedRadius = 0.060f;
    float smallSeedRadius = 0.050f;
    float edgePointRadius = 0.026f;

    Color edgePointColor = BLACK;

    Color driftOriginalColor = GRAY;
    Color driftMovedColor = RED;
};

inline PlanetPlateDebugDrawSettings CreateDefaultPlanetPlateDebugDrawSettings()
{
    PlanetPlateDebugDrawSettings settings;
    return settings;
}

inline Color PlanetDebugPlateColor(int plateId)
{
    float hue = fmodf((float)plateId * 137.508f, 360.0f);
    return ColorFromHSV(hue, 0.85f, 1.0f);
}

#endif

#if defined(PLANET_DEBUG_DRAW_PHASE1_POINTS)

inline void DrawPlanetPhase1PlatePoints(
    const PlanetPlateMap& plateMap,
    const PlanetPlateDebugDrawSettings& drawSettings,
    const Camera3D& camera
)
{
    if (!plateMap.valid || plateMap.points.empty())
    {
        return;
    }

    const Vector3 planetCenter =
    {
        0.0f,
        0.0f,
        0.0f
    };

    const float planetOcclusionRadius =
        plateMap.planetRadius;

    if (drawSettings.drawPlatePoints)
    {
        for (
            int pointIndex = 0;
            pointIndex < (int)plateMap.points.size();
            pointIndex++
            )
        {
            Vector3 worldPosition =
                plateMap.points[pointIndex];

            if (
                !PlanetDebugPointVisibleFromCamera(
                    worldPosition,
                    planetCenter,
                    planetOcclusionRadius,
                    camera
                )
                )
            {
                continue;
            }

            int finalPlateId = -1;

            if (
                plateMap.finalPlateIds.size() ==
                plateMap.points.size()
                )
            {
                finalPlateId =
                    plateMap.finalPlateIds[pointIndex];
            }

            Color color =
                DARKGRAY;

            if (finalPlateId >= 0)
            {
                color =
                    PlanetDebugPlateColor(
                        finalPlateId
                    );
            }

            float circleRadius =
                3.0f;

            if (
                plateMap.smallPlateIds.size() ==
                plateMap.points.size() &&
                plateMap.smallPlateIds[pointIndex] >= 0
                )
            {
                circleRadius =
                    5.0f;
            }

            Vector2 screenPosition =
                GetWorldToScreen(
                    worldPosition,
                    camera
                );

            DrawCircleV(
                screenPosition,
                circleRadius,
                color
            );
        }
    }

    if (drawSettings.drawLargeSeeds)
    {
        for (const PlanetPlate& plate : plateMap.largePlates)
        {
            if (
                plate.seedPointIndex < 0 ||
                plate.seedPointIndex >=
                (int)plateMap.points.size()
                )
            {
                continue;
            }

            Vector3 worldPosition =
                plateMap.points[
                    plate.seedPointIndex
                ];

            if (
                !PlanetDebugPointVisibleFromCamera(
                    worldPosition,
                    planetCenter,
                    planetOcclusionRadius,
                    camera
                )
                )
            {
                continue;
            }

            Vector2 screenPosition =
                GetWorldToScreen(
                    worldPosition,
                    camera
                );

            DrawCircleV(
                screenPosition,
                8.0f,
                PlanetDebugPlateColor(
                    plate.id
                )
            );
        }
    }

    if (drawSettings.drawSmallSeeds)
    {
        for (const PlanetPlate& plate : plateMap.smallPlates)
        {
            if (
                plate.seedPointIndex < 0 ||
                plate.seedPointIndex >=
                (int)plateMap.points.size()
                )
            {
                continue;
            }

            Vector3 worldPosition =
                plateMap.points[
                    plate.seedPointIndex
                ];

            if (
                !PlanetDebugPointVisibleFromCamera(
                    worldPosition,
                    planetCenter,
                    planetOcclusionRadius,
                    camera
                )
                )
            {
                continue;
            }

            Vector2 screenPosition =
                GetWorldToScreen(
                    worldPosition,
                    camera
                );

            DrawCircleV(
                screenPosition,
                7.0f,
                PlanetDebugPlateColor(
                    plate.id
                )
            );
        }
    }

    if (drawSettings.drawEdgePoints)
    {
        Vector3 planetCenter =
        {
            0.0f,
            0.0f,
            0.0f
        };

        for (
            const PlanetPlateEdgePoint& edgePoint :
            plateMap.edgePoints
            )
        {
            if (
                !PlanetDebugPointVisibleFromCamera(
                    edgePoint.position,
                    planetCenter,
                    plateMap.planetRadius,
                    camera
                )
                )
            {
                continue;
            }

            Vector2 screenPosition =
                GetWorldToScreen(
                    edgePoint.position,
                    camera
                );

            DrawCircleV(
                screenPosition,
                5.0f,
                drawSettings.edgePointColor
            );
        }
    }
}

#endif

#if defined(PLANET_DEBUG_DRAW_PHASE1_DRIFT_EVENTS) && defined(PLANET_ENABLE_PHASE1_DEBUG_DATA)

inline void DrawPlanetPhase1DriftEvents(
    const PlanetPlateMap& plateMap,
    const PlanetPlateDebugDrawSettings& drawSettings
)
{
    if (!plateMap.valid)
    {
        return;
    }

    if (!drawSettings.drawDriftEvents)
    {
        return;
    }

    for (const PlanetDriftEvent& event : plateMap.driftEvents)
    {
        if (!event.valid)
        {
            continue;
        }

        DrawSphereWires(
            event.originalCenter,
            event.radius,
            16,
            16,
            drawSettings.driftOriginalColor
        );

        DrawSphereWires(
            event.movedCenter,
            event.radius,
            16,
            16,
            drawSettings.driftMovedColor
        );

        DrawLine3D(
            event.originalCenter,
            event.movedCenter,
            drawSettings.driftMovedColor
        );

        DrawSphere(
            event.originalCenter,
            0.055f,
            drawSettings.driftOriginalColor
        );

        DrawSphere(
            event.movedCenter,
            0.055f,
            drawSettings.driftMovedColor
        );
    }
}

#endif

#if defined(PLANET_DEBUG_GENERATE_PHASE1_VORONOI)

inline Color BlendPlanetDebugPlateColorsByDistance(
    const std::vector<PlanetPlateDistanceResult>& closestPlates,
    float blendWidth,
    int maxBlendPlates
)
{
    if (closestPlates.empty())
    {
        return BLACK;
    }

    int closestPlateId = closestPlates[0].plateId;

    if (closestPlateId < 0)
    {
        return BLACK;
    }

    if (blendWidth <= PLANET_EPSILON || maxBlendPlates <= 1)
    {
        return PlanetDebugPlateColor(closestPlateId);
    }

    float closestDistance = closestPlates[0].distance;

    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float totalWeight = 0.0f;

    int usedPlateCount = 0;

    for (const PlanetPlateDistanceResult& result : closestPlates)
    {
        if (usedPlateCount >= maxBlendPlates)
        {
            break;
        }

        if (result.plateId < 0)
        {
            continue;
        }

        float distanceDifference = result.distance - closestDistance;

        if (distanceDifference > blendWidth)
        {
            continue;
        }

        float t = 1.0f - distanceDifference / blendWidth;
        float weight = PlanetSmoothStep01(t);

        Color color = PlanetDebugPlateColor(result.plateId);

        r += (float)color.r * weight;
        g += (float)color.g * weight;
        b += (float)color.b * weight;

        totalWeight += weight;
        usedPlateCount++;
    }

    if (totalWeight <= PLANET_EPSILON)
    {
        return PlanetDebugPlateColor(closestPlateId);
    }

    Color result;
    result.r = (unsigned char)(r / totalWeight);
    result.g = (unsigned char)(g / totalWeight);
    result.b = (unsigned char)(b / totalWeight);
    result.a = 255;

    return result;
}

inline Texture2D GeneratePlanetPhase1VoronoiDebugTexture(
    const PlanetPlateMap& plateMap,
    const PlanetPlateDebugDrawSettings& drawSettings
)
{
    int faceResolution = drawSettings.voronoiFaceResolution;

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

    if (
        !plateMap.valid ||
        plateMap.points.empty() ||
        plateMap.finalPlateIds.size() != plateMap.points.size() ||
        plateMap.finalPlates.empty()
        )
    {
        Texture2D texture = LoadTextureFromImage(image);
        UnloadImage(image);
        return texture;
    }

    std::vector<PlanetPlatePointGroup> plateGroups =
        BuildPlanetPlatePointGroups(
            plateMap.points,
            plateMap.finalPlateIds,
            (int)plateMap.finalPlates.size()
        );

    int maxBlendPlates = drawSettings.voronoiMaxBlendPlates;

    if (maxBlendPlates < 1)
    {
        maxBlendPlates = 1;
    }

    std::vector<PlanetPlateDistanceResult> closestPlates;
    closestPlates.reserve(maxBlendPlates);

    for (int pixelY = 0; pixelY < atlasHeight; pixelY++)
    {
        for (int pixelX = 0; pixelX < atlasWidth; pixelX++)
        {
            Vector3 spherePoint = PlanetAtlasPixelToSpherePoint(
                pixelX,
                pixelY,
                faceResolution,
                plateMap.pointRadius
            );

            FindClosestPlanetPlates(
                spherePoint,
                plateGroups,
                maxBlendPlates,
                closestPlates
            );

            Color color = BlendPlanetDebugPlateColorsByDistance(
                closestPlates,
                drawSettings.voronoiBlendWidth,
                maxBlendPlates
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

#endif
