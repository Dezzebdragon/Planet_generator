#pragma once

#include "raylib.h"
#include "raymath.h"
#include "cubesphere.h"

#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <thread>

static constexpr float PLANET_EPSILON = 0.0001f;

inline float PlanetClamp01(float value)
{
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

inline float PlanetClamp(float value, float minValue, float maxValue)
{
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

inline float PlanetLerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

inline float PlanetSmoothStep01(float value)
{
    value = PlanetClamp01(value);
    return value * value * (3.0f - 2.0f * value);
}

inline float PlanetInverseLerp01(float a, float b, float value)
{
    float range = b - a;

    if (fabsf(range) < PLANET_EPSILON)
    {
        return 0.0f;
    }

    return PlanetClamp01((value - a) / range);
}

inline float PlanetPeak01(float x, float center, float width)
{
    if (width < PLANET_EPSILON)
    {
        width = PLANET_EPSILON;
    }

    float d = (x - center) / width;
    return expf(-d * d);
}

inline Color PlanetLerpColor(Color a, Color b, float t)
{
    t = PlanetClamp01(t);

    Color result;
    result.r = (unsigned char)PlanetLerp((float)a.r, (float)b.r, t);
    result.g = (unsigned char)PlanetLerp((float)a.g, (float)b.g, t);
    result.b = (unsigned char)PlanetLerp((float)a.b, (float)b.b, t);
    result.a = 255;

    return result;
}

inline Color PlanetPlateIdColor(int plateId)
{
    if (plateId < 0)
    {
        return BLACK;
    }

    float hue = fmodf((float)plateId * 137.508f, 360.0f);
    return ColorFromHSV(hue, 0.85f, 1.0f);
}

inline Vector3 PlanetSafeNormalize(Vector3 value, Vector3 fallback)
{
    if (Vector3Length(value) < PLANET_EPSILON)
    {
        return Vector3Normalize(fallback);
    }

    return Vector3Normalize(value);
}

inline float PlanetGetVector3Axis(Vector3 value, int axis)
{
    if (axis == 0) return value.x;
    if (axis == 1) return value.y;
    return value.z;
}

struct PlanetRandom
{
    std::uint32_t state = 1;

    explicit PlanetRandom(std::uint32_t seed = 1)
    {
        if (seed == 0)
        {
            seed = 0x6D2B79F5u;
        }

        state = seed;
    }

    std::uint32_t NextU32()
    {
        state += 0x9E3779B9u;

        std::uint32_t z = state;
        z = (z ^ (z >> 16)) * 0x85EBCA6Bu;
        z = (z ^ (z >> 13)) * 0xC2B2AE35u;
        z = z ^ (z >> 16);

        return z;
    }

    float Float01()
    {
        return (float)(NextU32() & 0x00FFFFFFu) / (float)0x00FFFFFFu;
    }

    float RangeFloat(float minValue, float maxValue)
    {
        return minValue + (maxValue - minValue) * Float01();
    }

    int RangeInt(int minValue, int maxValue)
    {
        if (maxValue < minValue)
        {
            return minValue;
        }

        std::uint32_t range = (std::uint32_t)(maxValue - minValue + 1);
        return minValue + (int)(NextU32() % range);
    }
};

inline unsigned int PlanetHashSeed(unsigned int seed, unsigned int salt)
{
    PlanetRandom random(seed ^ salt);
    return random.NextU32();
}

inline std::uint64_t PlanetMakeUndirectedIndexPairKey(
    int indexA,
    int indexB
)
{
    std::uint32_t smallerIndex =
        (std::uint32_t)(indexA < indexB ? indexA : indexB);

    std::uint32_t largerIndex =
        (std::uint32_t)(indexA < indexB ? indexB : indexA);

    return
        ((std::uint64_t)smallerIndex << 32) |
        (std::uint64_t)largerIndex;
}

inline std::vector<Vector3> PlanetGenerateFibonacciSpherePoints(
    int amount,
    float radius,
    float displacementStrength,
    PlanetRandom& random
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

        Vector3 point =
        {
            x * radius,
            y * radius,
            z * radius
        };

        if (displacementStrength > 0.0f)
        {
            Vector3 offset =
            {
                random.RangeFloat(-displacementStrength, displacementStrength),
                random.RangeFloat(-displacementStrength, displacementStrength),
                random.RangeFloat(-displacementStrength, displacementStrength)
            };

            point = Vector3Add(point, offset);
            point = Vector3Normalize(point);
            point = Vector3Scale(point, radius);
        }

        points.push_back(point);
    }

    return points;
}

struct PlanetKDNode
{
    int pointIndex = -1;
    int axis = 0;
    int left = -1;
    int right = -1;
};

struct PlanetKDTree
{
    std::vector<Vector3> points;
    std::vector<PlanetKDNode> nodes;
    int root = -1;
};

struct PlanetKDNeighborResult
{
    int pointIndex = -1;
    float distanceSqr = 0.0f;
};

inline int PlanetBuildKDTreeRecursive(
    PlanetKDTree& tree,
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
            return PlanetGetVector3Axis(tree.points[a], axis) <
                PlanetGetVector3Axis(tree.points[b], axis);
        }
    );

    PlanetKDNode node;
    node.pointIndex = pointIndices[middle];
    node.axis = axis;

    int nodeIndex = (int)tree.nodes.size();
    tree.nodes.push_back(node);

    tree.nodes[nodeIndex].left = PlanetBuildKDTreeRecursive(
        tree,
        pointIndices,
        start,
        middle,
        depth + 1
    );

    tree.nodes[nodeIndex].right = PlanetBuildKDTreeRecursive(
        tree,
        pointIndices,
        middle + 1,
        end,
        depth + 1
    );

    return nodeIndex;
}

inline PlanetKDTree PlanetBuildKDTree(const std::vector<Vector3>& points)
{
    PlanetKDTree tree;
    tree.points = points;

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

    tree.root = PlanetBuildKDTreeRecursive(
        tree,
        pointIndices,
        0,
        (int)pointIndices.size(),
        0
    );

    return tree;
}

inline void PlanetFindNearestKDTreeRecursive(
    const PlanetKDTree& tree,
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

    const PlanetKDNode& node = tree.nodes[nodeIndex];
    Vector3 point = tree.points[node.pointIndex];

    float distanceSqr = Vector3DistanceSqr(target, point);

    if (distanceSqr < bestDistanceSqr)
    {
        bestDistanceSqr = distanceSqr;
        bestPointIndex = node.pointIndex;
    }

    int axis = node.axis;

    float targetAxis = PlanetGetVector3Axis(target, axis);
    float pointAxis = PlanetGetVector3Axis(point, axis);

    int firstBranch = node.left;
    int secondBranch = node.right;

    if (targetAxis > pointAxis)
    {
        firstBranch = node.right;
        secondBranch = node.left;
    }

    PlanetFindNearestKDTreeRecursive(
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
        PlanetFindNearestKDTreeRecursive(
            tree,
            secondBranch,
            target,
            bestPointIndex,
            bestDistanceSqr
        );
    }
}

inline int PlanetFindNearestKDTreePoint(
    const PlanetKDTree& tree,
    Vector3 target
)
{
    if (tree.root < 0)
    {
        return -1;
    }

    int bestPointIndex = -1;
    float bestDistanceSqr = 999999999.0f;

    PlanetFindNearestKDTreeRecursive(
        tree,
        tree.root,
        target,
        bestPointIndex,
        bestDistanceSqr
    );

    return bestPointIndex;
}

inline void PlanetInsertKDNeighborResult(
    std::vector<PlanetKDNeighborResult>& results,
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

    PlanetKDNeighborResult result;
    result.pointIndex = pointIndex;
    result.distanceSqr = distanceSqr;

    results.insert(results.begin() + insertIndex, result);

    if ((int)results.size() > maxResultCount)
    {
        results.pop_back();
    }
}

inline void PlanetFindKNearestKDTreeRecursive(
    const PlanetKDTree& tree,
    int nodeIndex,
    Vector3 target,
    int maxResultCount,
    std::vector<PlanetKDNeighborResult>& results
)
{
    if (nodeIndex < 0)
    {
        return;
    }

    const PlanetKDNode& node = tree.nodes[nodeIndex];
    Vector3 point = tree.points[node.pointIndex];

    float distanceSqr = Vector3DistanceSqr(target, point);

    PlanetInsertKDNeighborResult(
        results,
        maxResultCount,
        node.pointIndex,
        distanceSqr
    );

    int axis = node.axis;

    float targetAxis = PlanetGetVector3Axis(target, axis);
    float pointAxis = PlanetGetVector3Axis(point, axis);

    int firstBranch = node.left;
    int secondBranch = node.right;

    if (targetAxis > pointAxis)
    {
        firstBranch = node.right;
        secondBranch = node.left;
    }

    PlanetFindKNearestKDTreeRecursive(
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
        PlanetFindKNearestKDTreeRecursive(
            tree,
            secondBranch,
            target,
            maxResultCount,
            results
        );
    }
}

inline void PlanetFindKNearestKDTreePoints(
    const PlanetKDTree& tree,
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

    std::vector<PlanetKDNeighborResult> nearestResults;
    nearestResults.reserve(maxResultCount);

    PlanetFindKNearestKDTreeRecursive(
        tree,
        tree.root,
        target,
        maxResultCount,
        nearestResults
    );

    for (const PlanetKDNeighborResult& result : nearestResults)
    {
        resultPointIndices.push_back(result.pointIndex);
    }
}

inline float PlanetSampleAtlasGridBilinear(
    const std::vector<float>& grid,
    int faceResolution,
    int face,
    float localU,
    float localV
)
{
    int atlasWidth = faceResolution * 3;

    int tileX = face % 3;
    int tileY = face / 3;

    float fx = PlanetClamp01(localU) * (float)(faceResolution - 1);
    float fy = PlanetClamp01(localV) * (float)(faceResolution - 1);

    int x0 = (int)fx;
    int y0 = (int)fy;

    int x1 = x0 + 1 < faceResolution ? x0 + 1 : x0;
    int y1 = y0 + 1 < faceResolution ? y0 + 1 : y0;

    float tx = fx - (float)x0;
    float ty = fy - (float)y0;

    int baseX = tileX * faceResolution;
    int baseY = tileY * faceResolution;

    float v00 = grid[(baseY + y0) * atlasWidth + baseX + x0];
    float v10 = grid[(baseY + y0) * atlasWidth + baseX + x1];
    float v01 = grid[(baseY + y1) * atlasWidth + baseX + x0];
    float v11 = grid[(baseY + y1) * atlasWidth + baseX + x1];

    float top = PlanetLerp(v00, v10, tx);
    float bottom = PlanetLerp(v01, v11, tx);

    return PlanetLerp(top, bottom, ty);
}

inline void PlanetSpherePointToAtlasFaceUV(
    Vector3 point,
    int& outFace,
    float& outLocalU,
    float& outLocalV
)
{
    float ax = fabsf(point.x);
    float ay = fabsf(point.y);
    float az = fabsf(point.z);

    float cubeU = 0.0f;
    float cubeV = 0.0f;

    if (az >= ax && az >= ay)
    {
        float inv = 1.0f / fmaxf(az, PLANET_EPSILON);

        if (point.z >= 0.0f)
        {
            outFace = 0;
            cubeU = point.x * inv;
        }
        else
        {
            outFace = 1;
            cubeU = -point.x * inv;
        }

        cubeV = point.y * inv;
    }
    else if (ax >= ay)
    {
        float inv = 1.0f / fmaxf(ax, PLANET_EPSILON);

        if (point.x >= 0.0f)
        {
            outFace = 2;
            cubeU = -point.z * inv;
        }
        else
        {
            outFace = 3;
            cubeU = point.z * inv;
        }

        cubeV = point.y * inv;
    }
    else
    {
        float inv = 1.0f / fmaxf(ay, PLANET_EPSILON);

        cubeU = point.x * inv;

        if (point.y >= 0.0f)
        {
            outFace = 4;
            cubeV = -point.z * inv;
        }
        else
        {
            outFace = 5;
            cubeV = point.z * inv;
        }
    }

    outLocalU = PlanetClamp01((cubeU + 1.0f) * 0.5f);
    outLocalV = PlanetClamp01((1.0f - cubeV) * 0.5f);
}

template <typename RowFunction>
inline void PlanetParallelForRows(
    int rowCount,
    RowFunction rowFunction
)
{
    int threadCount = (int)std::thread::hardware_concurrency();

    if (threadCount < 1)
    {
        threadCount = 1;
    }

    if (threadCount > rowCount)
    {
        threadCount = rowCount;
    }

    if (threadCount <= 1)
    {
        for (int row = 0; row < rowCount; row++)
        {
            rowFunction(row);
        }

        return;
    }

    std::vector<std::thread> threads;
    threads.reserve(threadCount);

    for (int threadIndex = 0; threadIndex < threadCount; threadIndex++)
    {
        threads.emplace_back(
            [threadIndex, threadCount, rowCount, &rowFunction]()
            {
                for (int row = threadIndex; row < rowCount; row += threadCount)
                {
                    rowFunction(row);
                }
            }
        );
    }

    for (std::thread& thread : threads)
    {
        thread.join();
    }
}

inline Vector3 PlanetAtlasPixelToSpherePoint(
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

inline bool PlanetDebugPointVisibleFromCamera(
    Vector3 point,
    Vector3 planetCenter,
    float planetOcclusionRadius,
    const Camera3D& camera
)
{

    if (planetOcclusionRadius <= PLANET_EPSILON)
    {
        return true;
    }

    Vector3 cameraToPoint =
        Vector3Subtract(
            point,
            camera.position
        );

    float pointDistanceSqr =
        Vector3LengthSqr(cameraToPoint);

    if (
        pointDistanceSqr <=
        PLANET_EPSILON * PLANET_EPSILON
        )
    {
        return true;
    }

    float pointDistance =
        sqrtf(pointDistanceSqr);

    Vector3 rayDirection =
        Vector3Scale(
            cameraToPoint,
            1.0f / pointDistance
        );

    Vector3 cameraForward =
        PlanetSafeNormalize(
            Vector3Subtract(
                camera.target,
                camera.position
            ),
            rayDirection
        );

    if (
        Vector3DotProduct(
            rayDirection,
            cameraForward
        ) <= 0.0f
        )
    {
        return false;
    }

    Vector3 cameraToCenter =
        Vector3Subtract(
            camera.position,
            planetCenter
        );

    float b =
        Vector3DotProduct(
            cameraToCenter,
            rayDirection
        );

    float c =
        Vector3DotProduct(
            cameraToCenter,
            cameraToCenter
        ) -
        planetOcclusionRadius *
        planetOcclusionRadius;

    float discriminant =
        b * b - c;

    if (discriminant < 0.0f)
    {
        return true;
    }

    float sqrtDiscriminant =
        sqrtf(discriminant);

    float nearHitDistance =
        -b - sqrtDiscriminant;

    float farHitDistance =
        -b + sqrtDiscriminant;

    float firstPositiveHitDistance =
        nearHitDistance;

    if (firstPositiveHitDistance <= PLANET_EPSILON)
    {
        firstPositiveHitDistance =
            farHitDistance;
    }

    float surfaceTolerance =
        fmaxf(
            PLANET_EPSILON * 10.0f,
            planetOcclusionRadius * 0.001f
        );

    if (
        firstPositiveHitDistance > PLANET_EPSILON &&
        firstPositiveHitDistance <
        pointDistance - surfaceTolerance
        )
    {
        return false;
    }

    return true;
}
