#pragma once

#include "planetDebugConfig.h"
#include "planetSettings.h"
#include "helperFunc.h"

#include <vector>
#include <algorithm>
#include <cmath>
#include <unordered_set>

enum PlanetPlateType
{
    PLANET_PLATE_LARGE = 0,
    PLANET_PLATE_SMALL
};

enum PlanetCrustType
{
    PLANET_CRUST_CONTINENTAL = 0,
    PLANET_CRUST_OCEANIC
};

struct PlanetPlate
{

    int id = -1;

    int localId = -1;

    PlanetPlateType type = PLANET_PLATE_LARGE;
    PlanetCrustType crustType = PLANET_CRUST_OCEANIC;

    int seedPointIndex = -1;

    int targetPointAmount = 0;

    Vector3 center = { 0.0f, 1.0f, 0.0f };

    std::vector<int> pointIndices;
};

struct PlanetPlateEdgePoint
{
    Vector3 position = { 0.0f, 0.0f, 0.0f };

    int pointAIndex = -1;
    int pointBIndex = -1;

    int plateAId = -1;
    int plateBId = -1;
};

#ifdef PLANET_ENABLE_PHASE1_DEBUG_DATA
struct PlanetDriftEvent
{
    bool valid = false;

    int originalPointIndex = -1;
    int originalPlateId = -1;

    Vector3 originalCenter = { 0.0f, 0.0f, 0.0f };
    Vector3 movedCenter = { 0.0f, 0.0f, 0.0f };

    float radius = 0.0f;
    float moveDistance = 0.0f;

    int affectedPointCount = 0;
};
#endif

struct PlanetPlateMap
{
    bool valid = false;

    float planetRadius = 1.0f;
    float pointRadius = 1.0f;

    std::vector<Vector3> points;

    std::vector<PlanetPlateEdgePoint> edgePoints;

    std::vector<PlanetPlate> largePlates;
    std::vector<PlanetPlate> smallPlates;
    std::vector<PlanetPlate> finalPlates;

    std::vector<int> largePlateIds;
    std::vector<int> smallPlateIds;
    std::vector<int> finalPlateIds;

#ifdef PLANET_ENABLE_PHASE1_DEBUG_DATA
    std::vector<PlanetDriftEvent> driftEvents;
#endif
};

struct PlanetPlateDistanceResult
{
    int plateId = -1;

    int pointIndex = -1;

    Vector3 closestPoint = { 0.0f, 0.0f, 0.0f };
    float distance = 0.0f;
};

struct PlanetPlatePointGroup
{
    int plateId = -1;

    std::vector<Vector3> points;

    std::vector<int> pointIndices;

    PlanetKDTree tree;
};

inline PlanetCrustType RollPlanetCrustType(
    float continentalChance,
    PlanetRandom& random
)
{
    continentalChance = PlanetClamp01(continentalChance);

    if (random.Float01() < continentalChance)
    {
        return PLANET_CRUST_CONTINENTAL;
    }

    return PLANET_CRUST_OCEANIC;
}

inline PlanetPlate MakePlanetPlate(
    int localId,
    int finalId,
    PlanetPlateType type,
    PlanetCrustType crustType,
    int seedPointIndex,
    int targetPointAmount
)
{
    PlanetPlate plate;

    plate.localId = localId;
    plate.id = finalId;
    plate.type = type;
    plate.crustType = crustType;
    plate.seedPointIndex = seedPointIndex;
    plate.targetPointAmount = targetPointAmount;

    return plate;
}

inline void RebuildPlanetPlatePointListsAndCenters(
    const std::vector<Vector3>& points,
    const std::vector<int>& pointPlateIds,
    std::vector<PlanetPlate>& plates
)
{
    for (PlanetPlate& plate : plates)
    {
        plate.pointIndices.clear();
        plate.center = { 0.0f, 0.0f, 0.0f };
    }

    if (points.empty() || pointPlateIds.size() != points.size())
    {
        return;
    }

    for (int pointIndex = 0; pointIndex < (int)points.size(); pointIndex++)
    {
        int plateIndex = pointPlateIds[pointIndex];

        if (plateIndex < 0 || plateIndex >= (int)plates.size())
        {
            continue;
        }

        plates[plateIndex].pointIndices.push_back(pointIndex);
    }

    for (PlanetPlate& plate : plates)
    {
        Vector3 centerSum = { 0.0f, 0.0f, 0.0f };

        for (int pointIndex : plate.pointIndices)
        {
            Vector3 normal = PlanetSafeNormalize(
                points[pointIndex],
                Vector3{ 0.0f, 1.0f, 0.0f }
            );

            centerSum = Vector3Add(centerSum, normal);
        }

        if (Vector3Length(centerSum) > PLANET_EPSILON)
        {
            plate.center = Vector3Normalize(centerSum);
        }
        else if (
            plate.seedPointIndex >= 0 &&
            plate.seedPointIndex < (int)points.size()
            )
        {
            plate.center = Vector3Normalize(points[plate.seedPointIndex]);
        }
        else
        {
            plate.center = { 0.0f, 1.0f, 0.0f };
        }
    }
}

inline std::vector<PlanetPlate> SelectPlanetPlateSeedsFromCandidates(
    const std::vector<Vector3>& points,
    const std::vector<int>& candidatePointIndices,
    int plateAmount,
    PlanetPlateType plateType,
    int finalIdStart,
    int minTargetPointAmount,
    int maxTargetPointAmount,
    float continentalChance,
    PlanetRandom& random
)
{
    std::vector<PlanetPlate> plates;

    if (points.empty() || candidatePointIndices.empty() || plateAmount <= 0)
    {
        return plates;
    }

    std::vector<int> validCandidates;
    validCandidates.reserve(candidatePointIndices.size());

    for (int pointIndex : candidatePointIndices)
    {
        if (pointIndex >= 0 && pointIndex < (int)points.size())
        {
            validCandidates.push_back(pointIndex);
        }
    }

    if (validCandidates.empty())
    {
        return plates;
    }

    if (plateAmount > (int)validCandidates.size())
    {
        plateAmount = (int)validCandidates.size();
    }

    bool useTargetPointAmount = maxTargetPointAmount > 0;

    if (useTargetPointAmount)
    {
        if (minTargetPointAmount < 1)
        {
            minTargetPointAmount = 1;
        }

        if (maxTargetPointAmount < minTargetPointAmount)
        {
            maxTargetPointAmount = minTargetPointAmount;
        }
    }

    std::vector<bool> selected(points.size(), false);
    std::vector<float> closestSeedDistanceSqr(points.size(), 999999999.0f);

    int firstCandidateIndex = random.RangeInt(
        0,
        (int)validCandidates.size() - 1
    );

    int firstSeedPointIndex = validCandidates[firstCandidateIndex];

    int firstTargetAmount = useTargetPointAmount
        ? random.RangeInt(minTargetPointAmount, maxTargetPointAmount)
        : 0;

    PlanetCrustType firstCrustType =
        RollPlanetCrustType(
            continentalChance,
            random
        );

    plates.push_back(
        MakePlanetPlate(
            0,
            finalIdStart,
            plateType,
            firstCrustType,
            firstSeedPointIndex,
            firstTargetAmount
        )
    );

    selected[firstSeedPointIndex] = true;

    for (int candidatePointIndex : validCandidates)
    {
        closestSeedDistanceSqr[candidatePointIndex] =
            Vector3DistanceSqr(
                points[candidatePointIndex],
                points[firstSeedPointIndex]
            );
    }

    for (int localPlateId = 1; localPlateId < plateAmount; localPlateId++)
    {
        int bestPointIndex = -1;
        float bestDistanceSqr = -1.0f;

        for (int candidatePointIndex : validCandidates)
        {
            if (selected[candidatePointIndex])
            {
                continue;
            }

            if (closestSeedDistanceSqr[candidatePointIndex] > bestDistanceSqr)
            {
                bestDistanceSqr = closestSeedDistanceSqr[candidatePointIndex];
                bestPointIndex = candidatePointIndex;
            }
        }

        if (bestPointIndex < 0)
        {
            break;
        }

        int targetAmount = useTargetPointAmount
            ? random.RangeInt(minTargetPointAmount, maxTargetPointAmount)
            : 0;

        PlanetCrustType crustType =
            RollPlanetCrustType(
                continentalChance,
                random
            );

        plates.push_back(
            MakePlanetPlate(
                localPlateId,
                finalIdStart + localPlateId,
                plateType,
                crustType,
                bestPointIndex,
                targetAmount
            )
        );

        selected[bestPointIndex] = true;

        for (int candidatePointIndex : validCandidates)
        {
            float distanceToNewSeedSqr =
                Vector3DistanceSqr(
                    points[candidatePointIndex],
                    points[bestPointIndex]
                );

            if (distanceToNewSeedSqr < closestSeedDistanceSqr[candidatePointIndex])
            {
                closestSeedDistanceSqr[candidatePointIndex] = distanceToNewSeedSqr;
            }
        }
    }

    return plates;
}

inline std::vector<PlanetPlate> SelectLargePlanetPlateSeeds(
    const std::vector<Vector3>& points,
    int largePlateAmount,
    float continentalChance,
    PlanetRandom& random
)
{
    std::vector<int> candidates;
    candidates.reserve(points.size());

    for (int i = 0; i < (int)points.size(); i++)
    {
        candidates.push_back(i);
    }

    return SelectPlanetPlateSeedsFromCandidates(
        points,
        candidates,
        largePlateAmount,
        PLANET_PLATE_LARGE,
        0,
        0,
        0,
        continentalChance,
        random
    );
}

inline std::vector<int> AssignPointsToNearestPlanetPlates(
    const std::vector<Vector3>& points,
    const std::vector<PlanetPlate>& plates
)
{
    std::vector<int> pointPlateIds;

    if (points.empty() || plates.empty())
    {
        return pointPlateIds;
    }

    pointPlateIds.resize(points.size(), -1);

    for (int pointIndex = 0; pointIndex < (int)points.size(); pointIndex++)
    {
        int closestLocalPlateId = -1;
        float closestDistanceSqr = 999999999.0f;

        for (int localPlateId = 0; localPlateId < (int)plates.size(); localPlateId++)
        {
            int seedPointIndex = plates[localPlateId].seedPointIndex;

            if (seedPointIndex < 0 || seedPointIndex >= (int)points.size())
            {
                continue;
            }

            float distanceSqr =
                Vector3DistanceSqr(
                    points[pointIndex],
                    points[seedPointIndex]
                );

            if (distanceSqr < closestDistanceSqr)
            {
                closestDistanceSqr = distanceSqr;
                closestLocalPlateId = localPlateId;
            }
        }

        pointPlateIds[pointIndex] = closestLocalPlateId;
    }

    return pointPlateIds;
}

inline bool SimulatePlanetContinentalDrift(
    const std::vector<Vector3>& points,
    std::vector<int>& pointPlateIds,
    float sphereRadius,
    PlanetRandom& random
#ifdef PLANET_ENABLE_PHASE1_DEBUG_DATA
    ,
    PlanetDriftEvent* debugEvent
#endif
)
{
#ifdef PLANET_ENABLE_PHASE1_DEBUG_DATA
    if (debugEvent != nullptr)
    {
        *debugEvent = PlanetDriftEvent();
    }
#endif

    if (points.empty() || pointPlateIds.size() != points.size())
    {
        return false;
    }

    std::vector<int> validPointIndices;

    for (int i = 0; i < (int)points.size(); i++)
    {
        if (pointPlateIds[i] >= 0)
        {
            validPointIndices.push_back(i);
        }
    }

    if (validPointIndices.empty())
    {
        return false;
    }

    int randomListIndex = random.RangeInt(
        0,
        (int)validPointIndices.size() - 1
    );

    int startPointIndex = validPointIndices[randomListIndex];
    int startPlateId = pointPlateIds[startPointIndex];

    Vector3 startCenter = points[startPointIndex];

    int differentPlatePointIndex = -1;
    float radiusSqr = 999999999.0f;

    for (int i = 0; i < (int)points.size(); i++)
    {
        int plateId = pointPlateIds[i];

        if (plateId < 0 || plateId == startPlateId)
        {
            continue;
        }

        float distanceSqr = Vector3DistanceSqr(
            startCenter,
            points[i]
        );

        if (distanceSqr < radiusSqr)
        {
            radiusSqr = distanceSqr;
            differentPlatePointIndex = i;
        }
    }

    if (differentPlatePointIndex < 0)
    {
        return false;
    }

    if (radiusSqr <= PLANET_EPSILON * PLANET_EPSILON)
    {
        return false;
    }

    float radius = sqrtf(radiusSqr);

    Vector3 movedCenter = points[differentPlatePointIndex];
    movedCenter = Vector3Normalize(movedCenter);
    movedCenter = Vector3Scale(movedCenter, sphereRadius);

    int affectedPointCount = 0;

    for (int i = 0; i < (int)points.size(); i++)
    {
        float distanceSqr = Vector3DistanceSqr(
            movedCenter,
            points[i]
        );

        if (distanceSqr <= radiusSqr)
        {
            pointPlateIds[i] = startPlateId;
            affectedPointCount++;
        }
    }

#ifdef PLANET_ENABLE_PHASE1_DEBUG_DATA
    if (debugEvent != nullptr)
    {
        debugEvent->valid = true;
        debugEvent->originalPointIndex = startPointIndex;
        debugEvent->originalPlateId = startPlateId;
        debugEvent->originalCenter = startCenter;
        debugEvent->movedCenter = movedCenter;
        debugEvent->radius = radius;
        debugEvent->moveDistance = Vector3Distance(startCenter, movedCenter);
        debugEvent->affectedPointCount = affectedPointCount;
    }
#endif

    return true;
}

inline std::vector<PlanetPlateEdgePoint> GeneratePlanetPlateEdgePoints(
    const std::vector<Vector3>& points,
    const std::vector<int>& finalPlateIds,
    float sphereRadius,
    int neighborCheckCount
)
{
    std::vector<PlanetPlateEdgePoint> edgePoints;

    if (
        points.size() < 2 ||
        finalPlateIds.size() != points.size() ||
        sphereRadius <= PLANET_EPSILON
        )
    {
        return edgePoints;
    }

    if (neighborCheckCount < 1)
    {
        neighborCheckCount = 1;
    }

    if (neighborCheckCount >= (int)points.size())
    {
        neighborCheckCount =
            (int)points.size() - 1;
    }

    PlanetKDTree pointTree =
        PlanetBuildKDTree(points);

    std::vector<int> nearestPointIndices;

    nearestPointIndices.reserve(
        neighborCheckCount + 1
    );

    edgePoints.reserve(
        points.size() * 2
    );

    std::unordered_set<std::uint64_t> createdSourcePairs;

    createdSourcePairs.reserve(
        points.size() * 2
    );

    for (
        int pointAIndex = 0;
        pointAIndex < (int)points.size();
        pointAIndex++
        )
    {
        int plateAId =
            finalPlateIds[pointAIndex];

        if (plateAId < 0)
        {
            continue;
        }

        PlanetFindKNearestKDTreePoints(
            pointTree,
            points[pointAIndex],
            neighborCheckCount + 1,
            nearestPointIndices
        );

        int checkedNeighborCount = 0;

        for (int pointBIndex : nearestPointIndices)
        {
            if (pointBIndex == pointAIndex)
            {
                continue;
            }

            if (
                pointBIndex < 0 ||
                pointBIndex >= (int)points.size()
                )
            {
                continue;
            }

            checkedNeighborCount++;

            int plateBId =
                finalPlateIds[pointBIndex];

            if (
                plateBId < 0 ||
                plateBId == plateAId
                )
            {
                if (
                    checkedNeighborCount >=
                    neighborCheckCount
                    )
                {
                    break;
                }

                continue;
            }

            std::uint64_t sourcePairKey =
                PlanetMakeUndirectedIndexPairKey(
                    pointAIndex,
                    pointBIndex
                );

            if (
                createdSourcePairs.find(sourcePairKey) !=
                createdSourcePairs.end()
                )
            {
                if (
                    checkedNeighborCount >=
                    neighborCheckCount
                    )
                {
                    break;
                }

                continue;
            }

            Vector3 midpoint =
                Vector3Scale(
                    Vector3Add(
                        points[pointAIndex],
                        points[pointBIndex]
                    ),
                    0.5f
                );

            if (Vector3Length(midpoint) <= PLANET_EPSILON)
            {
                if (
                    checkedNeighborCount >=
                    neighborCheckCount
                    )
                {
                    break;
                }

                continue;
            }

            Vector3 edgePosition =
                Vector3Scale(
                    Vector3Normalize(midpoint),
                    sphereRadius
                );

            int sourceAIndex =
                pointAIndex < pointBIndex
                ? pointAIndex
                : pointBIndex;

            int sourceBIndex =
                pointAIndex < pointBIndex
                ? pointBIndex
                : pointAIndex;

            PlanetPlateEdgePoint edgePoint;

            edgePoint.position =
                edgePosition;

            edgePoint.pointAIndex =
                sourceAIndex;

            edgePoint.pointBIndex =
                sourceBIndex;

            edgePoint.plateAId =
                finalPlateIds[sourceAIndex];

            edgePoint.plateBId =
                finalPlateIds[sourceBIndex];

            edgePoints.push_back(
                edgePoint
            );

            createdSourcePairs.insert(
                sourcePairKey
            );

            if (
                checkedNeighborCount >=
                neighborCheckCount
                )
            {
                break;
            }
        }
    }

    return edgePoints;
}

inline std::vector<bool> GenerateProvisionalLargePlateEdgeFlags(
    const std::vector<Vector3>& points,
    const std::vector<int>& largePlateIds,
    int neighborCheckCount
)
{
    std::vector<bool> edgeFlags;

    if (
        points.size() < 2 ||
        largePlateIds.size() != points.size()
        )
    {
        return edgeFlags;
    }

    if (neighborCheckCount < 1)
    {
        neighborCheckCount = 1;
    }

    if (neighborCheckCount >= (int)points.size())
    {
        neighborCheckCount =
            (int)points.size() - 1;
    }

    edgeFlags.resize(
        points.size(),
        false
    );

    PlanetKDTree pointTree =
        PlanetBuildKDTree(points);

    std::vector<int> nearestPointIndices;

    nearestPointIndices.reserve(
        neighborCheckCount + 1
    );

    for (
        int pointIndex = 0;
        pointIndex < (int)points.size();
        pointIndex++
        )
    {
        int currentLargePlateId =
            largePlateIds[pointIndex];

        if (currentLargePlateId < 0)
        {
            continue;
        }

        PlanetFindKNearestKDTreePoints(
            pointTree,
            points[pointIndex],
            neighborCheckCount + 1,
            nearestPointIndices
        );

        int checkedNeighborCount = 0;

        for (int neighborPointIndex : nearestPointIndices)
        {
            if (neighborPointIndex == pointIndex)
            {
                continue;
            }

            if (
                neighborPointIndex < 0 ||
                neighborPointIndex >= (int)largePlateIds.size()
                )
            {
                continue;
            }

            checkedNeighborCount++;

            int neighborLargePlateId =
                largePlateIds[neighborPointIndex];

            if (
                neighborLargePlateId >= 0 &&
                neighborLargePlateId != currentLargePlateId
                )
            {
                edgeFlags[pointIndex] = true;
                break;
            }

            if (
                checkedNeighborCount >=
                neighborCheckCount
                )
            {
                break;
            }
        }
    }

    return edgeFlags;
}

inline std::vector<PlanetPlate> SelectSmallPlanetPlateSeedsFromEdges(
    const std::vector<Vector3>& points,
    const std::vector<bool>& largeEdgePointFlags,
    int smallPlateAmount,
    int largePlateAmount,
    int minPointAmount,
    int maxPointAmount,
    float continentalChance,
    PlanetRandom& random
)
{
    std::vector<int> edgeCandidates;

    if (
        points.empty() ||
        largeEdgePointFlags.size() != points.size() ||
        smallPlateAmount <= 0
        )
    {
        return std::vector<PlanetPlate>();
    }

    edgeCandidates.reserve(largeEdgePointFlags.size());

    for (int i = 0; i < (int)largeEdgePointFlags.size(); i++)
    {
        if (largeEdgePointFlags[i])
        {
            edgeCandidates.push_back(i);
        }
    }

    return SelectPlanetPlateSeedsFromCandidates(
        points,
        edgeCandidates,
        smallPlateAmount,
        PLANET_PLATE_SMALL,
        largePlateAmount,
        minPointAmount,
        maxPointAmount,
        continentalChance,
        random
    );
}

inline std::vector<int> AssignPointsToSmallPlanetPlates(
    const std::vector<Vector3>& points,
    const std::vector<PlanetPlate>& smallPlates
)
{
    std::vector<int> smallPlateIds;
    smallPlateIds.resize(points.size(), -1);

    if (points.empty() || smallPlates.empty())
    {
        return smallPlateIds;
    }

    for (int localSmallId = 0; localSmallId < (int)smallPlates.size(); localSmallId++)
    {
        const PlanetPlate& plate = smallPlates[localSmallId];

        if (plate.targetPointAmount <= 0)
        {
            continue;
        }

        int seedPointIndex = plate.seedPointIndex;

        if (seedPointIndex < 0 || seedPointIndex >= (int)points.size())
        {
            continue;
        }

        std::vector<int> sortedPointIndices;
        sortedPointIndices.reserve(points.size());

        for (int pointIndex = 0; pointIndex < (int)points.size(); pointIndex++)
        {
            sortedPointIndices.push_back(pointIndex);
        }

        std::sort(
            sortedPointIndices.begin(),
            sortedPointIndices.end(),
            [&](int a, int b)
            {
                float distanceASqr =
                    Vector3DistanceSqr(points[a], points[seedPointIndex]);

                float distanceBSqr =
                    Vector3DistanceSqr(points[b], points[seedPointIndex]);

                return distanceASqr < distanceBSqr;
            }
        );

        int claimedPointCount = 0;

        for (int pointIndex : sortedPointIndices)
        {
            if (claimedPointCount >= plate.targetPointAmount)
            {
                break;
            }

            if (smallPlateIds[pointIndex] != -1)
            {
                continue;
            }

            smallPlateIds[pointIndex] = localSmallId;
            claimedPointCount++;
        }
    }

    return smallPlateIds;
}

inline std::vector<int> BuildPlanetFinalPlateIds(
    const std::vector<int>& largePlateIds,
    const std::vector<int>& smallPlateIds,
    int largePlateCount
)
{
    std::vector<int> finalPlateIds;

    if (largePlateIds.empty())
    {
        return finalPlateIds;
    }

    finalPlateIds.resize(largePlateIds.size(), -1);

    for (int pointIndex = 0; pointIndex < (int)largePlateIds.size(); pointIndex++)
    {
        int finalId = largePlateIds[pointIndex];

        if (
            smallPlateIds.size() == largePlateIds.size() &&
            smallPlateIds[pointIndex] >= 0
            )
        {
            finalId = largePlateCount + smallPlateIds[pointIndex];
        }

        finalPlateIds[pointIndex] = finalId;
    }

    return finalPlateIds;
}

inline std::vector<PlanetPlate> BuildPlanetFinalPlates(
    const std::vector<PlanetPlate>& largePlates,
    const std::vector<PlanetPlate>& smallPlates
)
{
    std::vector<PlanetPlate> finalPlates;

    int finalPlateCount =
        (int)largePlates.size() +
        (int)smallPlates.size();

    finalPlates.resize(finalPlateCount);

    for (int i = 0; i < (int)largePlates.size(); i++)
    {
        PlanetPlate plate = largePlates[i];

        plate.id = i;
        plate.localId = i;
        plate.type = PLANET_PLATE_LARGE;
        plate.pointIndices.clear();

        finalPlates[i] = plate;
    }

    for (int i = 0; i < (int)smallPlates.size(); i++)
    {
        PlanetPlate plate = smallPlates[i];

        int finalId = (int)largePlates.size() + i;

        plate.id = finalId;
        plate.localId = i;
        plate.type = PLANET_PLATE_SMALL;
        plate.pointIndices.clear();

        finalPlates[finalId] = plate;
    }

    return finalPlates;
}

inline std::vector<PlanetPlatePointGroup> BuildPlanetPlatePointGroups(
    const std::vector<Vector3>& points,
    const std::vector<int>& finalPlateIds,
    int finalPlateCount
)
{
    std::vector<PlanetPlatePointGroup> groups;
    groups.resize(finalPlateCount);

    for (int plateId = 0; plateId < finalPlateCount; plateId++)
    {
        groups[plateId].plateId = plateId;
    }

    if (points.empty() || finalPlateIds.size() != points.size())
    {
        return groups;
    }

    for (int pointIndex = 0; pointIndex < (int)points.size(); pointIndex++)
    {
        int plateId = finalPlateIds[pointIndex];

        if (plateId < 0 || plateId >= finalPlateCount)
        {
            continue;
        }

        groups[plateId].points.push_back(points[pointIndex]);
        groups[plateId].pointIndices.push_back(pointIndex);
    }

    for (PlanetPlatePointGroup& group : groups)
    {
        group.tree = PlanetBuildKDTree(group.points);
    }

    return groups;
}

inline void FindClosestPlanetPlates(
    Vector3 spherePoint,
    const std::vector<PlanetPlatePointGroup>& plateGroups,
    int maxResultCount,
    std::vector<PlanetPlateDistanceResult>& results
)
{
    results.clear();

    if (maxResultCount <= 0)
    {
        return;
    }

    for (const PlanetPlatePointGroup& group : plateGroups)
    {
        if (group.points.empty())
        {
            continue;
        }

        int localPointIndex = PlanetFindNearestKDTreePoint(
            group.tree,
            spherePoint
        );

        if (
            localPointIndex < 0 ||
            localPointIndex >= (int)group.points.size()
            )
        {
            continue;
        }

        Vector3 closestPoint = group.points[localPointIndex];
        int originalPointIndex = -1;

        if (
            localPointIndex >= 0 &&
            localPointIndex < (int)group.pointIndices.size()
            )
        {
            originalPointIndex = group.pointIndices[localPointIndex];
        }

        float distanceSqr = Vector3DistanceSqr(
            spherePoint,
            closestPoint
        );

        PlanetPlateDistanceResult result;
        result.plateId = group.plateId;
        result.pointIndex = originalPointIndex;
        result.closestPoint = closestPoint;
        result.distance = distanceSqr;

        int insertIndex = 0;

        while (
            insertIndex < (int)results.size() &&
            results[insertIndex].distance < distanceSqr
            )
        {
            insertIndex++;
        }

        if (insertIndex >= maxResultCount)
        {
            continue;
        }

        results.insert(results.begin() + insertIndex, result);

        if ((int)results.size() > maxResultCount)
        {
            results.pop_back();
        }
    }

    for (PlanetPlateDistanceResult& result : results)
    {
        result.distance = sqrtf(result.distance);
    }
}

inline PlanetPlateMap GeneratePlanetPlateMap(
    const PlanetGenerationSettings& settings
)
{
    PlanetPlateMap map;

    PlanetRandom random(
        settings.plateSeed
    );

    map.planetRadius =
        settings.planetRadius;

    map.pointRadius =
        settings.planetRadius +
        settings.plates.pointRadiusOffset;

    map.points =
        PlanetGenerateFibonacciSpherePoints(
            settings.plates.pointAmount,
            map.pointRadius,
            settings.plates.pointDisplacementStrength,
            random
        );

    if (map.points.empty())
    {
        return map;
    }

    map.largePlates =
        SelectLargePlanetPlateSeeds(
            map.points,
            settings.plates.largePlateAmount,
            settings.plates.largePlateContinentalChance,
            random
        );

    map.largePlateIds =
        AssignPointsToNearestPlanetPlates(
            map.points,
            map.largePlates
        );

    RebuildPlanetPlatePointListsAndCenters(
        map.points,
        map.largePlateIds,
        map.largePlates
    );

    for (
        int driftStep = 0;
        driftStep < settings.plates.driftStepAmount;
        driftStep++
        )
    {
#ifdef PLANET_ENABLE_PHASE1_DEBUG_DATA

        PlanetDriftEvent event;

        bool drifted =
            SimulatePlanetContinentalDrift(
                map.points,
                map.largePlateIds,
                map.pointRadius,
                random,
                &event
            );

        if (drifted && event.valid)
        {
            map.driftEvents.push_back(event);
        }

#else

        SimulatePlanetContinentalDrift(
            map.points,
            map.largePlateIds,
            map.pointRadius,
            random
        );

#endif
    }

    RebuildPlanetPlatePointListsAndCenters(
        map.points,
        map.largePlateIds,
        map.largePlates
    );

    std::vector<bool> provisionalLargeEdgeFlags =
        GenerateProvisionalLargePlateEdgeFlags(
            map.points,
            map.largePlateIds,
            settings.plates.provisionalLargeEdgeNeighborCheckCount
        );

    map.smallPlates =
        SelectSmallPlanetPlateSeedsFromEdges(
            map.points,
            provisionalLargeEdgeFlags,
            settings.plates.smallPlateAmount,
            (int)map.largePlates.size(),
            settings.plates.smallPlateMinPoints,
            settings.plates.smallPlateMaxPoints,
            settings.plates.smallPlateContinentalChance,
            random
        );

    map.smallPlateIds =
        AssignPointsToSmallPlanetPlates(
            map.points,
            map.smallPlates
        );

    RebuildPlanetPlatePointListsAndCenters(
        map.points,
        map.smallPlateIds,
        map.smallPlates
    );

    map.finalPlateIds =
        BuildPlanetFinalPlateIds(
            map.largePlateIds,
            map.smallPlateIds,
            (int)map.largePlates.size()
        );

    map.finalPlates =
        BuildPlanetFinalPlates(
            map.largePlates,
            map.smallPlates
        );

    RebuildPlanetPlatePointListsAndCenters(
        map.points,
        map.finalPlateIds,
        map.finalPlates
    );

    map.edgePoints =
        GeneratePlanetPlateEdgePoints(
            map.points,
            map.finalPlateIds,
            map.pointRadius,
            settings.plates.finalEdgePointNeighborCheckCount
        );

    map.valid = true;

    return map;
}
