#pragma once

#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <cmath>
#include <algorithm>

struct PlateSeed
{
    int pointIndex;
    Color color;
    int targetPointAmount;
};

struct KDNode
{
    int pointIndex;
    int axis;
    int left;
    int right;
};

struct KDTree
{
    std::vector<Vector3> points;
    std::vector<KDNode> nodes;
    int root;
};

struct KDNeighborResult
{
    int pointIndex;
    float distanceSqr;
};

struct ContinentalDriftEvent
{
    bool valid;

    int originalPointIndex;
    int originalPlateIndex;

    Vector3 originalCenter;
    Vector3 movedCenter;

    float radius;
    float moveDistance;
};

struct PlanetPlateGenerationSettings
{
    int pointAmount = 1000;
    int largeSeedAmount = 12;

    float pointRadiusOffset = 0.05f;
    float pointDisplacementStrength = 0.08f;

    int driftStepAmount = 30;

    int edgeNeighborCheckCount = 6;

    int smallSeedAmount = 4;
    int smallPlateMinPoints = 6;
    int smallPlateMaxPoints = 18;

    int textureFaceResolution = 256;

    bool generateVoronoiTexture = false;
};

struct PlanetPlateData
{
    float pointRadiusFromCenter = 0.0f;

    std::vector<Vector3> points;

    std::vector<PlateSeed> largeSeeds;
    std::vector<int> largePlateIndices;

    std::vector<ContinentalDriftEvent> driftEvents;

    std::vector<bool> edgePoints;

    std::vector<PlateSeed> smallSeeds;
    std::vector<int> smallPlateIndices;

    Texture2D voronoiTexture = { 0 };
};

inline float RandomFloat(float minValue, float maxValue)
{
    int value = GetRandomValue(0, 10000);
    float t = (float)value / 10000.0f;

    return minValue + (maxValue - minValue) * t;
}

inline std::vector<Vector3> GenerateSpherePoints(
    int amount,
    float radius,
    float displacementStrength
)
{
    std::vector<Vector3> points;

    if (amount <= 0)
    {
        return points;
    }

    points.reserve(amount);

    const float goldenAngle = PI * (3.0f - sqrtf(5.0f));

    for (int i = 0; i < amount; i++)
    {
        float t = ((float)i + 0.5f) / (float)amount;

        float y = 1.0f - 2.0f * t;
        float circleRadius = sqrtf(1.0f - y * y);

        float theta = goldenAngle * (float)i;

        float x = cosf(theta) * circleRadius;
        float z = sinf(theta) * circleRadius;

        Vector3 point = {
            x * radius,
            y * radius,
            z * radius
        };

        points.push_back(point);
    }

    for (Vector3& point : points)
    {
        Vector3 randomOffset = {
            RandomFloat(-displacementStrength, displacementStrength),
            RandomFloat(-displacementStrength, displacementStrength),
            RandomFloat(-displacementStrength, displacementStrength)
        };

        point = Vector3Add(point, randomOffset);

        point = Vector3Normalize(point);
        point = Vector3Scale(point, radius);
    }

    return points;
}

inline float GetVector3Axis(Vector3 v, int axis)
{
    if (axis == 0) return v.x;
    if (axis == 1) return v.y;
    return v.z;
}

inline Color GetPlateSeedColor(int seedIndex)
{
    float hue = fmodf((float)seedIndex * 137.508f, 360.0f);
    return ColorFromHSV(hue, 0.85f, 1.0f);
}

inline std::vector<PlateSeed> SelectEvenlySpacedSeedsFromCandidates(
    const std::vector<Vector3>& points,
    const std::vector<int>& candidatePointIndices,
    int seedAmount,
    int minTargetPointAmount,
    int maxTargetPointAmount
)
{
    std::vector<PlateSeed> seeds;

    if (points.empty() || candidatePointIndices.empty() || seedAmount <= 0)
    {
        return seeds;
    }

    std::vector<int> validCandidatePointIndices;
    validCandidatePointIndices.reserve(candidatePointIndices.size());

    for (int pointIndex : candidatePointIndices)
    {
        if (pointIndex >= 0 && pointIndex < (int)points.size())
        {
            validCandidatePointIndices.push_back(pointIndex);
        }
    }

    if (validCandidatePointIndices.empty())
    {
        return seeds;
    }

    if (seedAmount > (int)validCandidatePointIndices.size())
    {
        seedAmount = (int)validCandidatePointIndices.size();
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
    else
    {
        minTargetPointAmount = 0;
        maxTargetPointAmount = 0;
    }

    std::vector<bool> selected(points.size(), false);
    std::vector<float> closestSeedDistanceSqr(points.size(), 999999999.0f);

    int firstCandidateIndex = GetRandomValue(
        0,
        (int)validCandidatePointIndices.size() - 1
    );

    int firstSeedPointIndex = validCandidatePointIndices[firstCandidateIndex];

    PlateSeed firstSeed;
    firstSeed.pointIndex = firstSeedPointIndex;
    firstSeed.color = GetPlateSeedColor(0);
    firstSeed.targetPointAmount = useTargetPointAmount
        ? GetRandomValue(minTargetPointAmount, maxTargetPointAmount)
        : 0;

    seeds.push_back(firstSeed);
    selected[firstSeedPointIndex] = true;

    for (int candidatePointIndex : validCandidatePointIndices)
    {
        closestSeedDistanceSqr[candidatePointIndex] = Vector3DistanceSqr(
            points[candidatePointIndex],
            points[firstSeedPointIndex]
        );
    }

    for (int seedIndex = 1; seedIndex < seedAmount; seedIndex++)
    {
        int bestPointIndex = -1;
        float bestDistanceSqr = -1.0f;

        for (int candidatePointIndex : validCandidatePointIndices)
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

        if (bestPointIndex == -1)
        {
            break;
        }

        PlateSeed seed;
        seed.pointIndex = bestPointIndex;
        seed.color = GetPlateSeedColor(seedIndex);
        seed.targetPointAmount = useTargetPointAmount
            ? GetRandomValue(minTargetPointAmount, maxTargetPointAmount)
            : 0;

        seeds.push_back(seed);
        selected[bestPointIndex] = true;

        for (int candidatePointIndex : validCandidatePointIndices)
        {
            float distanceToNewSeedSqr = Vector3DistanceSqr(
                points[candidatePointIndex],
                points[bestPointIndex]
            );

            if (distanceToNewSeedSqr < closestSeedDistanceSqr[candidatePointIndex])
            {
                closestSeedDistanceSqr[candidatePointIndex] = distanceToNewSeedSqr;
            }
        }
    }

    return seeds;
}

inline std::vector<PlateSeed> SelectPlateSeeds(
    const std::vector<Vector3>& points,
    int seedAmount
)
{
    std::vector<int> candidatePointIndices;
    candidatePointIndices.reserve(points.size());

    for (int i = 0; i < (int)points.size(); i++)
    {
        candidatePointIndices.push_back(i);
    }

    return SelectEvenlySpacedSeedsFromCandidates(
        points,
        candidatePointIndices,
        seedAmount,
        0,
        0
    );
}

inline void DrawPlatePoints(
    const std::vector<Vector3>& points,

    const std::vector<PlateSeed>& largePlateSeeds,
    const std::vector<int>& pointLargePlateIndices,

    const std::vector<PlateSeed>& smallPlateSeeds,
    const std::vector<int>& pointSmallPlateIndices,

    float normalPointRadius,
    float smallPlatePointRadius,
    float largeSeedRadius,
    float smallSeedRadius
)
{
    if (points.empty())
    {
        return;
    }

    for (int pointIndex = 0; pointIndex < (int)points.size(); pointIndex++)
    {
        Color pointColor = DARKGRAY;
        float pointRadius = normalPointRadius;

        bool belongsToSmallPlate = false;

        if (pointSmallPlateIndices.size() == points.size())
        {
            int smallPlateIndex = pointSmallPlateIndices[pointIndex];

            if (smallPlateIndex >= 0 && smallPlateIndex < (int)smallPlateSeeds.size())
            {
                pointColor = smallPlateSeeds[smallPlateIndex].color;
                pointRadius = smallPlatePointRadius;
                belongsToSmallPlate = true;
            }
        }

        if (!belongsToSmallPlate && pointLargePlateIndices.size() == points.size())
        {
            int largePlateIndex = pointLargePlateIndices[pointIndex];

            if (largePlateIndex >= 0 && largePlateIndex < (int)largePlateSeeds.size())
            {
                pointColor = largePlateSeeds[largePlateIndex].color;
            }
        }

        DrawSphere(points[pointIndex], pointRadius, pointColor);
    }

    for (const PlateSeed& seed : largePlateSeeds)
    {
        if (seed.pointIndex < 0 || seed.pointIndex >= (int)points.size())
        {
            continue;
        }

        DrawSphere(points[seed.pointIndex], largeSeedRadius, seed.color);
    }

    for (const PlateSeed& seed : smallPlateSeeds)
    {
        if (seed.pointIndex < 0 || seed.pointIndex >= (int)points.size())
        {
            continue;
        }

        DrawSphere(points[seed.pointIndex], smallSeedRadius, seed.color);
    }
}

inline std::vector<int> AssignPointsToPlates(
    const std::vector<Vector3>& points,
    const std::vector<PlateSeed>& seeds
)
{
    std::vector<int> pointPlateIndices;

    if (points.empty() || seeds.empty())
    {
        return pointPlateIndices;
    }

    pointPlateIndices.resize(points.size(), -1);

    bool hasOnlyNormalPlates = true;

    for (const PlateSeed& seed : seeds)
    {
        if (seed.targetPointAmount > 0)
        {
            hasOnlyNormalPlates = false;
            break;
        }
    }

    if (hasOnlyNormalPlates)
    {
        for (int pointIndex = 0; pointIndex < (int)points.size(); pointIndex++)
        {
            int closestSeedIndex = -1;
            float closestDistanceSqr = 999999999.0f;

            for (int seedIndex = 0; seedIndex < (int)seeds.size(); seedIndex++)
            {
                int seedPointIndex = seeds[seedIndex].pointIndex;

                if (seedPointIndex < 0 || seedPointIndex >= (int)points.size())
                {
                    continue;
                }

                float distanceSqr = Vector3DistanceSqr(
                    points[pointIndex],
                    points[seedPointIndex]
                );

                if (distanceSqr < closestDistanceSqr)
                {
                    closestDistanceSqr = distanceSqr;
                    closestSeedIndex = seedIndex;
                }
            }

            pointPlateIndices[pointIndex] = closestSeedIndex;
        }

        return pointPlateIndices;
    }

    for (int seedIndex = 0; seedIndex < (int)seeds.size(); seedIndex++)
    {
        const PlateSeed& seed = seeds[seedIndex];

        if (seed.targetPointAmount <= 0)
        {
            continue;
        }

        int seedPointIndex = seed.pointIndex;

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
                float distanceASqr = Vector3DistanceSqr(points[a], points[seedPointIndex]);
                float distanceBSqr = Vector3DistanceSqr(points[b], points[seedPointIndex]);

                return distanceASqr < distanceBSqr;
            }
        );

        int claimedPoints = 0;

        for (int i = 0; i < (int)sortedPointIndices.size(); i++)
        {
            if (claimedPoints >= seed.targetPointAmount)
            {
                break;
            }

            int pointIndex = sortedPointIndices[i];

            if (pointPlateIndices[pointIndex] != -1)
            {
                continue;
            }

            pointPlateIndices[pointIndex] = seedIndex;
            claimedPoints++;
        }
    }

    return pointPlateIndices;
}

inline int BuildKDTreeRecursive(
    KDTree& tree,
    std::vector<int>& pointIndices,
    int start,
    int end,
    int depth
)
{
    if (start >= end)
    {
        return -1;
    }

    int axis = depth % 3;
    int middle = start + (end - start) / 2;

    std::nth_element(
        pointIndices.begin() + start,
        pointIndices.begin() + middle,
        pointIndices.begin() + end,
        [&](int a, int b)
        {
            return GetVector3Axis(tree.points[a], axis) <
                GetVector3Axis(tree.points[b], axis);
        }
    );

    KDNode node;
    node.pointIndex = pointIndices[middle];
    node.axis = axis;
    node.left = -1;
    node.right = -1;

    int nodeIndex = (int)tree.nodes.size();
    tree.nodes.push_back(node);

    tree.nodes[nodeIndex].left = BuildKDTreeRecursive(
        tree,
        pointIndices,
        start,
        middle,
        depth + 1
    );

    tree.nodes[nodeIndex].right = BuildKDTreeRecursive(
        tree,
        pointIndices,
        middle + 1,
        end,
        depth + 1
    );

    return nodeIndex;
}

inline KDTree BuildKDTree(const std::vector<Vector3>& points)
{
    KDTree tree;
    tree.points = points;
    tree.root = -1;

    if (points.empty())
    {
        return tree;
    }

    std::vector<int> pointIndices;
    pointIndices.reserve(points.size());

    for (int i = 0; i < (int)points.size(); i++)
    {
        pointIndices.push_back(i);
    }

    tree.root = BuildKDTreeRecursive(
        tree,
        pointIndices,
        0,
        (int)pointIndices.size(),
        0
    );

    return tree;
}

inline void FindNearestKDTreeRecursive(
    const KDTree& tree,
    int nodeIndex,
    Vector3 target,
    int& bestPointIndex,
    float& bestDistanceSqr
)
{
    if (nodeIndex < 0)
    {
        return;
    }

    const KDNode& node = tree.nodes[nodeIndex];

    Vector3 point = tree.points[node.pointIndex];

    float distanceSqr = Vector3DistanceSqr(target, point);

    if (distanceSqr < bestDistanceSqr)
    {
        bestDistanceSqr = distanceSqr;
        bestPointIndex = node.pointIndex;
    }

    int axis = node.axis;

    float targetAxis = GetVector3Axis(target, axis);
    float pointAxis = GetVector3Axis(point, axis);

    int firstBranch = node.left;
    int secondBranch = node.right;

    if (targetAxis > pointAxis)
    {
        firstBranch = node.right;
        secondBranch = node.left;
    }

    FindNearestKDTreeRecursive(
        tree,
        firstBranch,
        target,
        bestPointIndex,
        bestDistanceSqr
    );

    float axisDistance = targetAxis - pointAxis;
    float axisDistanceSqr = axisDistance * axisDistance;

    if (axisDistanceSqr < bestDistanceSqr)
    {
        FindNearestKDTreeRecursive(
            tree,
            secondBranch,
            target,
            bestPointIndex,
            bestDistanceSqr
        );
    }
}

inline int FindNearestKDTreePoint(
    const KDTree& tree,
    Vector3 target
)
{
    if (tree.root < 0)
    {
        return -1;
    }

    int bestPointIndex = -1;
    float bestDistanceSqr = 999999999.0f;

    FindNearestKDTreeRecursive(
        tree,
        tree.root,
        target,
        bestPointIndex,
        bestDistanceSqr
    );

    return bestPointIndex;
}

inline void InsertKDNeighborResult(
    std::vector<KDNeighborResult>& results,
    int maxResultCount,
    int pointIndex,
    float distanceSqr
)
{
    if (maxResultCount <= 0)
    {
        return;
    }

    int insertIndex = 0;

    while (
        insertIndex < (int)results.size() &&
        results[insertIndex].distanceSqr < distanceSqr
        )
    {
        insertIndex++;
    }

    if (insertIndex >= maxResultCount)
    {
        return;
    }

    KDNeighborResult result;
    result.pointIndex = pointIndex;
    result.distanceSqr = distanceSqr;

    results.insert(
        results.begin() + insertIndex,
        result
    );

    if ((int)results.size() > maxResultCount)
    {
        results.pop_back();
    }
}

inline void FindKNearestKDTreeRecursive(
    const KDTree& tree,
    int nodeIndex,
    Vector3 target,
    int maxResultCount,
    std::vector<KDNeighborResult>& results
)
{
    if (nodeIndex < 0)
    {
        return;
    }

    const KDNode& node = tree.nodes[nodeIndex];
    Vector3 point = tree.points[node.pointIndex];

    float distanceSqr = Vector3DistanceSqr(
        target,
        point
    );

    InsertKDNeighborResult(
        results,
        maxResultCount,
        node.pointIndex,
        distanceSqr
    );

    int axis = node.axis;

    float targetAxis = GetVector3Axis(target, axis);
    float pointAxis = GetVector3Axis(point, axis);

    int firstBranch = node.left;
    int secondBranch = node.right;

    if (targetAxis > pointAxis)
    {
        firstBranch = node.right;
        secondBranch = node.left;
    }

    FindKNearestKDTreeRecursive(
        tree,
        firstBranch,
        target,
        maxResultCount,
        results
    );

    float axisDistance = targetAxis - pointAxis;
    float axisDistanceSqr = axisDistance * axisDistance;

    float worstAcceptedDistanceSqr = 999999999.0f;

    if ((int)results.size() >= maxResultCount)
    {
        worstAcceptedDistanceSqr = results.back().distanceSqr;
    }

    if (axisDistanceSqr <= worstAcceptedDistanceSqr)
    {
        FindKNearestKDTreeRecursive(
            tree,
            secondBranch,
            target,
            maxResultCount,
            results
        );
    }
}

inline void FindKNearestKDTreePoints(
    const KDTree& tree,
    Vector3 target,
    int maxResultCount,
    std::vector<int>& resultPointIndices
)
{
    resultPointIndices.clear();

    if (tree.root < 0 || maxResultCount <= 0)
    {
        return;
    }

    std::vector<KDNeighborResult> nearestResults;
    nearestResults.reserve(maxResultCount);

    FindKNearestKDTreeRecursive(
        tree,
        tree.root,
        target,
        maxResultCount,
        nearestResults
    );

    for (const KDNeighborResult& result : nearestResults)
    {
        resultPointIndices.push_back(result.pointIndex);
    }
}

inline std::vector<bool> DetectPlateEdgePoints(
    const std::vector<Vector3>& points,
    const std::vector<int>& finalPlateIds,
    int neighborCheckCount
)
{
    std::vector<bool> edgePoints;

    if (points.empty() || finalPlateIds.size() != points.size())
    {
        return edgePoints;
    }

    if (neighborCheckCount < 1)
    {
        neighborCheckCount = 1;
    }

    if (neighborCheckCount >= (int)points.size())
    {
        neighborCheckCount = (int)points.size() - 1;
    }

    edgePoints.resize(points.size(), false);

    KDTree pointTree = BuildKDTree(points);

    std::vector<int> nearestPointIndices;
    nearestPointIndices.reserve(neighborCheckCount + 1);

    for (int pointIndex = 0; pointIndex < (int)points.size(); pointIndex++)
    {
        int currentPlateId = finalPlateIds[pointIndex];

        if (currentPlateId < 0)
        {
            continue;
        }

        FindKNearestKDTreePoints(
            pointTree,
            points[pointIndex],
            neighborCheckCount + 1,
            nearestPointIndices
        );

        int checkedNeighborCount = 0;

        for (int neighborIndex : nearestPointIndices)
        {
            if (neighborIndex == pointIndex)
            {
                continue;
            }

            if (neighborIndex < 0 || neighborIndex >= (int)finalPlateIds.size())
            {
                continue;
            }

            int neighborPlateId = finalPlateIds[neighborIndex];

            if (neighborPlateId >= 0 && neighborPlateId != currentPlateId)
            {
                edgePoints[pointIndex] = true;
                break;
            }

            checkedNeighborCount++;

            if (checkedNeighborCount >= neighborCheckCount)
            {
                break;
            }
        }
    }

    return edgePoints;
}

inline void DrawPlateEdgePoints(
    const std::vector<Vector3>& points,
    const std::vector<bool>& edgePoints,
    float edgePointRadius,
    Color edgeColor
)
{
    if (points.empty() || edgePoints.size() != points.size())
    {
        return;
    }

    for (int pointIndex = 0; pointIndex < (int)points.size(); pointIndex++)
    {
        if (edgePoints[pointIndex])
        {
            DrawSphere(points[pointIndex], edgePointRadius, edgeColor);
        }
    }
}

inline std::vector<PlateSeed> SelectSmallPlateSeedsFromEdges(
    const std::vector<Vector3>& points,
    const std::vector<bool>& edgePoints,
    int seedAmount,
    int minPointAmount,
    int maxPointAmount
)
{
    std::vector<int> edgePointIndices;

    if (points.empty() || edgePoints.size() != points.size() || seedAmount <= 0)
    {
        return std::vector<PlateSeed>();
    }

    edgePointIndices.reserve(edgePoints.size());

    for (int i = 0; i < (int)edgePoints.size(); i++)
    {
        if (edgePoints[i])
        {
            edgePointIndices.push_back(i);
        }
    }

    return SelectEvenlySpacedSeedsFromCandidates(
        points,
        edgePointIndices,
        seedAmount,
        minPointAmount,
        maxPointAmount
    );
}

inline std::vector<Color> BuildFinalPointColors(
    const std::vector<Vector3>& points,

    const std::vector<PlateSeed>& largePlateSeeds,
    const std::vector<int>& pointLargePlateIndices,

    const std::vector<PlateSeed>& smallPlateSeeds,
    const std::vector<int>& pointSmallPlateIndices
)
{
    std::vector<Color> pointColors;
    pointColors.resize(points.size(), DARKGRAY);

    for (int pointIndex = 0; pointIndex < (int)points.size(); pointIndex++)
    {
        if (pointLargePlateIndices.size() == points.size())
        {
            int largePlateIndex = pointLargePlateIndices[pointIndex];

            if (largePlateIndex >= 0 && largePlateIndex < (int)largePlateSeeds.size())
            {
                pointColors[pointIndex] = largePlateSeeds[largePlateIndex].color;
            }
        }

        if (pointSmallPlateIndices.size() == points.size())
        {
            int smallPlateIndex = pointSmallPlateIndices[pointIndex];

            if (smallPlateIndex >= 0 && smallPlateIndex < (int)smallPlateSeeds.size())
            {
                pointColors[pointIndex] = smallPlateSeeds[smallPlateIndex].color;
            }
        }
    }

    return pointColors;
}

inline std::vector<int> BuildFinalPointPlateIds(
    const std::vector<Vector3>& points,
    const std::vector<PlateSeed>& largePlateSeeds,
    const std::vector<int>& pointLargePlateIndices,
    const std::vector<PlateSeed>& smallPlateSeeds,
    const std::vector<int>& pointSmallPlateIndices
)
{
    std::vector<int> finalPlateIds;
    finalPlateIds.resize(points.size(), -1);

    for (int pointIndex = 0; pointIndex < (int)points.size(); pointIndex++)
    {
        if (pointLargePlateIndices.size() == points.size())
        {
            int largePlateIndex = pointLargePlateIndices[pointIndex];

            if (largePlateIndex >= 0 && largePlateIndex < (int)largePlateSeeds.size())
            {
                finalPlateIds[pointIndex] = largePlateIndex;
            }
        }

        if (pointSmallPlateIndices.size() == points.size())
        {
            int smallPlateIndex = pointSmallPlateIndices[pointIndex];

            if (smallPlateIndex >= 0 && smallPlateIndex < (int)smallPlateSeeds.size())
            {
                finalPlateIds[pointIndex] = (int)largePlateSeeds.size() + smallPlateIndex;
            }
        }
    }

    return finalPlateIds;
}

struct PlateDistanceResult
{
    int plateId;
    Vector3 closestPoint;
    float distance;
};

struct PlatePointGroup
{
    int plateId;
    std::vector<Vector3> points;
    KDTree tree;
};

inline float DistanceToBisectorPlane(
    Vector3 point,
    Vector3 pointA,
    Vector3 pointB
)
{
    Vector3 ab = Vector3Subtract(pointB, pointA);
    float abLength = Vector3Length(ab);

    if (abLength < 0.0001f)
    {
        return 999999999.0f;
    }

    Vector3 midpoint = Vector3Scale(
        Vector3Add(pointA, pointB),
        0.5f
    );

    Vector3 fromMidpointToPoint = Vector3Subtract(point, midpoint);

    float signedDistance = Vector3DotProduct(
        fromMidpointToPoint,
        ab
    ) / abLength;

    return fabsf(signedDistance);
}

inline std::vector<Color> BuildFinalPlateColors(
    const std::vector<PlateSeed>& largePlateSeeds,
    const std::vector<PlateSeed>& smallPlateSeeds
)
{
    std::vector<Color> plateColors;

    int finalPlateCount = (int)largePlateSeeds.size() + (int)smallPlateSeeds.size();
    plateColors.resize(finalPlateCount, DARKGRAY);

    for (int i = 0; i < (int)largePlateSeeds.size(); i++)
    {
        plateColors[i] = largePlateSeeds[i].color;
    }

    for (int i = 0; i < (int)smallPlateSeeds.size(); i++)
    {
        int finalPlateId = (int)largePlateSeeds.size() + i;
        plateColors[finalPlateId] = smallPlateSeeds[i].color;
    }

    return plateColors;
}

inline std::vector<PlatePointGroup> BuildPlatePointGroups(
    const std::vector<Vector3>& points,
    const std::vector<int>& finalPlateIds,
    int finalPlateCount
)
{
    std::vector<PlatePointGroup> groups;
    groups.resize(finalPlateCount);

    for (int plateId = 0; plateId < finalPlateCount; plateId++)
    {
        groups[plateId].plateId = plateId;
    }

    for (int pointIndex = 0; pointIndex < (int)points.size(); pointIndex++)
    {
        int plateId = finalPlateIds[pointIndex];

        if (plateId < 0 || plateId >= finalPlateCount)
        {
            continue;
        }

        groups[plateId].points.push_back(points[pointIndex]);
    }

    for (int plateId = 0; plateId < finalPlateCount; plateId++)
    {
        groups[plateId].tree = BuildKDTree(groups[plateId].points);
    }

    return groups;
}

inline void FindClosestFinalPlates(
    Vector3 spherePoint,
    const std::vector<PlatePointGroup>& plateGroups,
    int maxResultCount,
    std::vector<PlateDistanceResult>& results
)
{
    results.clear();

    if (maxResultCount <= 0)
    {
        return;
    }

    for (const PlatePointGroup& group : plateGroups)
    {
        if (group.points.empty())
        {
            continue;
        }

        int localPointIndex = FindNearestKDTreePoint(
            group.tree,
            spherePoint
        );

        if (localPointIndex < 0 || localPointIndex >= (int)group.points.size())
        {
            continue;
        }

        Vector3 closestPoint = group.points[localPointIndex];

        float distanceSqr = Vector3DistanceSqr(
            spherePoint,
            closestPoint
        );

        PlateDistanceResult result;
        result.plateId = group.plateId;
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

        results.insert(
            results.begin() + insertIndex,
            result
        );

        if ((int)results.size() > maxResultCount)
        {
            results.pop_back();
        }
    }

    for (PlateDistanceResult& result : results)
    {
        result.distance = sqrtf(result.distance);
    }
}
inline float SmoothStep(float t)
{
    t = Clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

inline Color BlendPlateColorsByDistance(
    const std::vector<PlateDistanceResult>& closestPlates,
    const std::vector<Color>& finalPlateColors,
    float blendWidth,
    int maxBlendPlates
)
{
    if (closestPlates.empty())
    {
        return BLACK;
    }

    int closestPlateId = closestPlates[0].plateId;

    if (closestPlateId < 0 || closestPlateId >= (int)finalPlateColors.size())
    {
        return BLACK;
    }

    if (blendWidth <= 0.0001f)
    {
        return finalPlateColors[closestPlateId];
    }

    float closestDistance = closestPlates[0].distance;

    float totalWeight = 0.0f;
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 0.0f;

    int usedPlateCount = 0;

    for (const PlateDistanceResult& result : closestPlates)
    {
        if (usedPlateCount >= maxBlendPlates)
        {
            break;
        }

        int plateId = result.plateId;

        if (plateId < 0 || plateId >= (int)finalPlateColors.size())
        {
            continue;
        }

        float distanceDifference = result.distance - closestDistance;

        if (distanceDifference > blendWidth)
        {
            continue;
        }

        float t = 1.0f - distanceDifference / blendWidth;
        float weight = SmoothStep(t);

        Color color = finalPlateColors[plateId];

        r += (float)color.r * weight;
        g += (float)color.g * weight;
        b += (float)color.b * weight;
        a += (float)color.a * weight;

        totalWeight += weight;
        usedPlateCount++;
    }

    if (totalWeight <= 0.0001f)
    {
        return finalPlateColors[closestPlateId];
    }

    Color result;
    result.r = (unsigned char)(r / totalWeight);
    result.g = (unsigned char)(g / totalWeight);
    result.b = (unsigned char)(b / totalWeight);
    result.a = 255;

    return result;
}

inline Texture2D GeneratePlanetVoronoiTexture(
    int faceResolution,
    float sphereRadius,

    const std::vector<Vector3>& points,

    const std::vector<PlateSeed>& largePlateSeeds,
    const std::vector<int>& pointLargePlateIndices,

    const std::vector<PlateSeed>& smallPlateSeeds,
    const std::vector<int>& pointSmallPlateIndices
)
{
    if (faceResolution < 1)
    {
        faceResolution = 1;
    }

    int atlasWidth = faceResolution * 3;
    int atlasHeight = faceResolution * 2;

    Image image = GenImageColor(atlasWidth, atlasHeight, MAGENTA);

    if (points.empty())
    {
        Texture2D emptyTexture = LoadTextureFromImage(image);
        UnloadImage(image);
        return emptyTexture;
    }

    std::vector<int> finalPlateIds = BuildFinalPointPlateIds(
        points,
        largePlateSeeds,
        pointLargePlateIndices,
        smallPlateSeeds,
        pointSmallPlateIndices
    );

    std::vector<Color> finalPlateColors = BuildFinalPlateColors(
        largePlateSeeds,
        smallPlateSeeds
    );

    int finalPlateCount = (int)largePlateSeeds.size() + (int)smallPlateSeeds.size();

    std::vector<PlatePointGroup> plateGroups = BuildPlatePointGroups(
        points,
        finalPlateIds,
        finalPlateCount
    );

    const float plateBlendWidth = 0.2f;
    const int maxBlendPlates = 4;

    std::vector<PlateDistanceResult> closestPlates;
    closestPlates.reserve(maxBlendPlates);

    for (int pixelY = 0; pixelY < atlasHeight; pixelY++)
    {
        for (int pixelX = 0; pixelX < atlasWidth; pixelX++)
        {
            int tileX = pixelX / faceResolution;
            int tileY = pixelY / faceResolution;

            int face = tileY * 3 + tileX;

            float localU = (float)(pixelX % faceResolution) / (float)(faceResolution - 1);
            float localV = (float)(pixelY % faceResolution) / (float)(faceResolution - 1);

            float cubeU = localU * 2.0f - 1.0f;
            float cubeV = 1.0f - localV * 2.0f;

            Vector3 cubePoint = CubeFacePoint(face, cubeU, cubeV);

            Vector3 spherePoint = Vector3Normalize(cubePoint);
            spherePoint = Vector3Scale(spherePoint, sphereRadius);

            FindClosestFinalPlates(
                spherePoint,
                plateGroups,
                maxBlendPlates,
                closestPlates
            );

            Color color = BlendPlateColorsByDistance(
                closestPlates,
                finalPlateColors,
                plateBlendWidth,
                maxBlendPlates
            );

            ImageDrawPixel(&image, pixelX, pixelY, color);
        }
    }

    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);

    return texture;
}

inline Vector3 GetRandomTangentDirection(Vector3 spherePoint)
{
    Vector3 normal = Vector3Normalize(spherePoint);

    Vector3 randomDirection = {
        RandomFloat(-1.0f, 1.0f),
        RandomFloat(-1.0f, 1.0f),
        RandomFloat(-1.0f, 1.0f)
    };

    randomDirection = Vector3Normalize(randomDirection);

    float dot = Vector3DotProduct(randomDirection, normal);

    Vector3 tangent = Vector3Subtract(
        randomDirection,
        Vector3Scale(normal, dot)
    );

    if (Vector3Length(tangent) < 0.0001f)
    {
        tangent = Vector3{
            -normal.z,
            0.0f,
            normal.x
        };
    }

    return Vector3Normalize(tangent);
}

inline ContinentalDriftEvent SimulateContinentalDrift(
    const std::vector<Vector3>& points,
    std::vector<int>& pointPlateIndices,
    float sphereRadius
)
{
    ContinentalDriftEvent event;
    event.valid = false;
    event.originalPointIndex = -1;
    event.originalPlateIndex = -1;
    event.originalCenter = Vector3{ 0.0f, 0.0f, 0.0f };
    event.movedCenter = Vector3{ 0.0f, 0.0f, 0.0f };
    event.radius = 0.0f;
    event.moveDistance = 0.0f;

    if (points.empty() || pointPlateIndices.size() != points.size())
    {
        return event;
    }

    std::vector<int> validPointIndices;

    for (int i = 0; i < (int)points.size(); i++)
    {
        if (pointPlateIndices[i] >= 0)
        {
            validPointIndices.push_back(i);
        }
    }

    if (validPointIndices.empty())
    {
        return event;
    }

    int randomListIndex = GetRandomValue(
        0,
        (int)validPointIndices.size() - 1
    );

    int startPointIndex = validPointIndices[randomListIndex];
    int startPlateIndex = pointPlateIndices[startPointIndex];

    Vector3 startCenter = points[startPointIndex];

    int differentPlatePointIndex = -1;

    float radiusSqr = 999999999.0f;

    for (int i = 0; i < (int)points.size(); i++)
    {
        int plateIndex = pointPlateIndices[i];

        if (plateIndex < 0)
        {
            continue;
        }

        if (plateIndex == startPlateIndex)
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

    if (differentPlatePointIndex == -1)
    {
        return event;
    }

    if (radiusSqr <= 0.0001f * 0.0001f)
    {
        return event;
    }

    float radius = sqrtf(radiusSqr);

    Vector3 radiusCenter = points[differentPlatePointIndex];

    radiusCenter = Vector3Normalize(radiusCenter);
    radiusCenter = Vector3Scale(radiusCenter, sphereRadius);

    for (int i = 0; i < (int)points.size(); i++)
    {
        float distanceSqr = Vector3DistanceSqr(
            radiusCenter,
            points[i]
        );

        if (distanceSqr <= radiusSqr)
        {
            pointPlateIndices[i] = startPlateIndex;
        }
    }

    event.valid = true;
    event.originalPointIndex = startPointIndex;
    event.originalPlateIndex = startPlateIndex;

    event.originalCenter = startCenter;
    event.movedCenter = radiusCenter;

    event.radius = radius;
    event.moveDistance = Vector3Distance(startCenter, radiusCenter);

    return event;
}

inline PlanetPlateData GeneratePlanetPlateData(
    float planetRadius,
    const PlanetPlateGenerationSettings& settings
)
{
    PlanetPlateData data;

    data.pointRadiusFromCenter = planetRadius + settings.pointRadiusOffset;

    data.points = GenerateSpherePoints(
        settings.pointAmount,
        data.pointRadiusFromCenter,
        settings.pointDisplacementStrength
    );

    data.largeSeeds = SelectPlateSeeds(
        data.points,
        settings.largeSeedAmount
    );

    data.largePlateIndices = AssignPointsToPlates(
        data.points,
        data.largeSeeds
    );

    for (int i = 0; i < settings.driftStepAmount; i++)
    {
        ContinentalDriftEvent event = SimulateContinentalDrift(
            data.points,
            data.largePlateIndices,
            data.pointRadiusFromCenter
        );

        if (event.valid)
        {
            data.driftEvents.push_back(event);
        }
    }

    data.edgePoints = DetectPlateEdgePoints(
        data.points,
        data.largePlateIndices,
        settings.edgeNeighborCheckCount
    );

    data.smallSeeds = SelectSmallPlateSeedsFromEdges(
        data.points,
        data.edgePoints,
        settings.smallSeedAmount,
        settings.smallPlateMinPoints,
        settings.smallPlateMaxPoints
    );

    data.smallPlateIndices = AssignPointsToPlates(
        data.points,
        data.smallSeeds
    );
    if (settings.generateVoronoiTexture)
    {
        data.voronoiTexture = GeneratePlanetVoronoiTexture(
            settings.textureFaceResolution,
            data.pointRadiusFromCenter,

            data.points,

            data.largeSeeds,
            data.largePlateIndices,

            data.smallSeeds,
            data.smallPlateIndices
        );
    }
    return data;
}

inline PlanetPlateData GeneratePlanetPlateData(
    float planetRadius,
    const PlanetPlateGenerationSettings& settings,
    unsigned int seed
)
{
    SetRandomSeed(seed);

    return GeneratePlanetPlateData(
        planetRadius,
        settings
    );
}

inline void DrawContinentalDriftEvent(
    const ContinentalDriftEvent& event,
    Color originalRadiusColor,
    Color movedRadiusColor
)
{
    if (!event.valid)
    {
        return;
    }

    DrawSphereWires(
        event.originalCenter,
        event.radius,
        16,
        16,
        originalRadiusColor
    );

    DrawSphereWires(
        event.movedCenter,
        event.radius,
        16,
        16,
        movedRadiusColor
    );

    DrawLine3D(
        event.originalCenter,
        event.movedCenter,
        movedRadiusColor
    );

    DrawSphere(
        event.originalCenter,
        0.07f,
        originalRadiusColor
    );

    DrawSphere(
        event.movedCenter,
        0.07f,
        movedRadiusColor
    );
}
