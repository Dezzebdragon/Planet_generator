#pragma once

#include "raylib.h"
#include "raymath.h"

#include "cubesphere.h"
#include "plates.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <vector>
#include <string>
#include <filesystem>

struct PlanetNoiseSettings
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

    float oceanLevel = 0.46f;
    float oceanDepth = 0.12f;
    float landHeight = 0.38f;

    float landPower = 1.6f;

    bool exportOctaveLayers = false;
    std::string octaveExportFolder = "resources";

    float finalContrast = 1.0f;

    float plateNoiseOffsetDistance = 100.0f;

    float plateNoiseBlendWidth = 0.2f;

    int maxNoiseBlendPlates = 4;
};

inline float PlanetNoiseClamp01(float value)
{
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

inline float PlanetNoiseSmoothStep01(float value)
{
    value = PlanetNoiseClamp01(value);
    return value * value * (3.0f - 2.0f * value);
}

inline float PlanetNoiseHashToFloat01(std::uint32_t hash)
{
    return (float)(hash & 0xFFFFFFu) / (float)0xFFFFFFu;
}

inline std::uint32_t PlanetNoiseHash3D(int x, int y, int z, unsigned int seed)
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

class PerlinNoise
{
public:
    explicit PerlinNoise(unsigned int seed = 1337)
    {
        reseed(seed);
    }

    void reseed(unsigned int seed)
    {
        permutation.resize(256);

        std::iota(permutation.begin(), permutation.end(), 0);

        std::mt19937 generator(seed);
        std::shuffle(permutation.begin(), permutation.end(), generator);

        permutation.insert(
            permutation.end(),
            permutation.begin(),
            permutation.end()
        );
    }

float noise01(float x, float y, float z) const
    {
        return noiseSigned(x, y, z) * 0.5f + 0.5f;
    }

float noiseSigned(float x, float y, float z) const
    {
        int xi = fastFloor(x) & 255;
        int yi = fastFloor(y) & 255;
        int zi = fastFloor(z) & 255;

        float xf = x - std::floor(x);
        float yf = y - std::floor(y);
        float zf = z - std::floor(z);

        float u = fade(xf);
        float v = fade(yf);
        float w = fade(zf);

        int aaa = permutation[permutation[permutation[xi] + yi] + zi];
        int aba = permutation[permutation[permutation[xi] + yi + 1] + zi];
        int aab = permutation[permutation[permutation[xi] + yi] + zi + 1];
        int abb = permutation[permutation[permutation[xi] + yi + 1] + zi + 1];

        int baa = permutation[permutation[permutation[xi + 1] + yi] + zi];
        int bba = permutation[permutation[permutation[xi + 1] + yi + 1] + zi];
        int bab = permutation[permutation[permutation[xi + 1] + yi] + zi + 1];
        int bbb = permutation[permutation[permutation[xi + 1] + yi + 1] + zi + 1];

        float x1 = lerp(
            grad(aaa, xf, yf, zf),
            grad(baa, xf - 1.0f, yf, zf),
            u
        );

        float x2 = lerp(
            grad(aba, xf, yf - 1.0f, zf),
            grad(bba, xf - 1.0f, yf - 1.0f, zf),
            u
        );

        float y1 = lerp(x1, x2, v);

        x1 = lerp(
            grad(aab, xf, yf, zf - 1.0f),
            grad(bab, xf - 1.0f, yf, zf - 1.0f),
            u
        );

        x2 = lerp(
            grad(abb, xf, yf - 1.0f, zf - 1.0f),
            grad(bbb, xf - 1.0f, yf - 1.0f, zf - 1.0f),
            u
        );

        float y2 = lerp(x1, x2, v);

        return lerp(y1, y2, w);
    }

    float fbm01(
        float x,
        float y,
        float z,
        int octaves,
        float persistence,
        float lacunarity
    ) const
    {
        float total = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float maxAmplitude = 0.0f;

        if (octaves < 1)
        {
            octaves = 1;
        }

        for (int i = 0; i < octaves; i++)
        {
            total += noise01(
                x * frequency,
                y * frequency,
                z * frequency
            ) * amplitude;

            maxAmplitude += amplitude;
            amplitude *= persistence;
            frequency *= lacunarity;
        }

        if (maxAmplitude <= 0.0001f)
        {
            return 0.0f;
        }

        return PlanetNoiseClamp01(total / maxAmplitude);
    }

    float ridged01(
        float x,
        float y,
        float z,
        int octaves,
        float persistence,
        float lacunarity
    ) const
    {
        float total = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float maxAmplitude = 0.0f;

        if (octaves < 1)
        {
            octaves = 1;
        }

        for (int i = 0; i < octaves; i++)
        {
            float n = noiseSigned(
                x * frequency,
                y * frequency,
                z * frequency
            );

            n = 1.0f - fabsf(n);
            n *= n;

            total += n * amplitude;
            maxAmplitude += amplitude;

            amplitude *= persistence;
            frequency *= lacunarity;
        }

        if (maxAmplitude <= 0.0001f)
        {
            return 0.0f;
        }

        return PlanetNoiseClamp01(total / maxAmplitude);
    }

    float planetValue01(
        Vector3 sphereDirection,
        const PlanetNoiseSettings& settings
    ) const
    {
        Vector3 noOffset = { 0.0f, 0.0f, 0.0f };

        return planetValue01WithOffset(
            sphereDirection,
            noOffset,
            settings
        );
    }

    float planetValue01WithOffset(
        Vector3 sphereDirection,
        Vector3 noiseOffset,
        const PlanetNoiseSettings& settings
    ) const
    {
        Vector3 n = Vector3Normalize(sphereDirection);

        float continent = fbm01(
            n.x * settings.continentFrequency + noiseOffset.x,
            n.y * settings.continentFrequency + noiseOffset.y,
            n.z * settings.continentFrequency + noiseOffset.z,
            settings.continentOctaves,
            settings.persistence,
            settings.lacunarity
        );

        float detail = fbm01(
            n.x * settings.detailFrequency + noiseOffset.x,
            n.y * settings.detailFrequency + noiseOffset.y,
            n.z * settings.detailFrequency + noiseOffset.z,
            settings.detailOctaves,
            settings.persistence,
            settings.lacunarity
        ) - 0.5f;

        float ridge = ridged01(
            n.x * settings.ridgeFrequency + noiseOffset.x,
            n.y * settings.ridgeFrequency + noiseOffset.y,
            n.z * settings.ridgeFrequency + noiseOffset.z,
            settings.ridgeOctaves,
            settings.persistence,
            settings.lacunarity
        );

        float landMask = PlanetNoiseSmoothStep01(
            (continent - settings.oceanLevel) /
            (1.0f - settings.oceanLevel)
        );

        float value =
            continent +
            detail * settings.detailStrength +
            ridge * settings.ridgeStrength * landMask;

        return PlanetNoiseClamp01(value);
    }

    float planetElevation(
        Vector3 sphereDirection,
        const PlanetNoiseSettings& settings
    ) const
    {
        float value = planetValue01(sphereDirection, settings);

        if (value < settings.oceanLevel)
        {
            float ocean01 = (settings.oceanLevel - value) / settings.oceanLevel;
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

    Vector3 displacedSpherePoint(
        Vector3 sphereDirection,
        float baseRadius,
        const PlanetNoiseSettings& settings
    ) const
    {
        Vector3 normal = Vector3Normalize(sphereDirection);
        float elevation = planetElevation(normal, settings);
        return Vector3Scale(normal, baseRadius + elevation);
    }

private:
    std::vector<int> permutation;

    static int fastFloor(float value)
    {
        return (int)std::floor(value);
    }

    static float fade(float t)
    {

        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    static float lerp(float a, float b, float t)
    {
        return a + t * (b - a);
    }

    static float grad(int hash, float x, float y, float z)
    {
        int h = hash & 15;

        float u = h < 8 ? x : y;

        float v;
        if (h < 4)
        {
            v = y;
        }
        else if (h == 12 || h == 14)
        {
            v = x;
        }
        else
        {
            v = z;
        }

        return ((h & 1) == 0 ? u : -u) +
               ((h & 2) == 0 ? v : -v);
    }
};

inline float PNClamp01(float value)
{
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

inline float PNApplyContrast(float value, float contrast)
{
    value = PNClamp01(value);

    if (contrast <= 0.0001f)
    {
        return value;
    }

    if (value < 0.5f)
    {
        return 0.5f * powf(value * 2.0f, contrast);
    }

    return 1.0f - 0.5f * powf((1.0f - value) * 2.0f, contrast);
}

inline Color PNGrayColor(float value)
{
    value = PNClamp01(value);

    unsigned char gray = (unsigned char)(value * 255.0f);

    return Color{
        gray,
        gray,
        gray,
        255
    };
}

inline Color PNTerrainPreviewColor(float value, const PlanetNoiseSettings& settings)
{
    value = PNClamp01(value);

    if (value < settings.oceanLevel)
    {
        return Color{ 20, 70, 160, 255 };
    }

    if (value < 0.60f)
    {
        return Color{ 40, 150, 70, 255 };
    }

    if (value < 0.78f)
    {
        return Color{ 120, 100, 60, 255 };
    }

    return Color{ 235, 235, 235, 255 };
}

inline Vector3 AtlasPixelToSpherePoint(
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

inline float SampleNormalOctave01(
    const PerlinNoise& noise,
    Vector3 spherePoint,
    float baseFrequency,
    int octave,
    float lacunarity
)
{
    Vector3 n = Vector3Normalize(spherePoint);

    float frequency = baseFrequency * powf(lacunarity, (float)octave);

    return noise.noise01(
        n.x * frequency,
        n.y * frequency,
        n.z * frequency
    );
}

inline float SampleRidgedOctave01(
    const PerlinNoise& noise,
    Vector3 spherePoint,
    float baseFrequency,
    int octave,
    float lacunarity
)
{
    Vector3 n = Vector3Normalize(spherePoint);

    float frequency = baseFrequency * powf(lacunarity, (float)octave);

    float value = noise.noiseSigned(
        n.x * frequency,
        n.y * frequency,
        n.z * frequency
    );

    value = 1.0f - fabsf(value);
    value *= value;

    return PNClamp01(value);
}

inline float SampleLayeredNoise01(
    const PerlinNoise& noise,
    Vector3 spherePoint,
    float baseFrequency,
    int octaveCount,
    float persistence,
    float lacunarity
)
{
    if (octaveCount < 1)
    {
        octaveCount = 1;
    }

    float total = 0.0f;
    float amplitude = 1.0f;
    float amplitudeSum = 0.0f;

    for (int octave = 0; octave < octaveCount; octave++)
    {
        float octaveValue = SampleNormalOctave01(
            noise,
            spherePoint,
            baseFrequency,
            octave,
            lacunarity
        );

        total += octaveValue * amplitude;
        amplitudeSum += amplitude;

        amplitude *= persistence;
    }

    if (amplitudeSum <= 0.0001f)
    {
        return 0.0f;
    }

    return PNClamp01(total / amplitudeSum);
}

inline float SamplePlanetTerrainValue01(
    const PerlinNoise& noise,
    Vector3 spherePoint,
    const PlanetNoiseSettings& settings
)
{
    float continent = SampleLayeredNoise01(
        noise,
        spherePoint,
        settings.continentFrequency,
        settings.continentOctaves,
        settings.persistence,
        settings.lacunarity
    );

    float detail = SampleLayeredNoise01(
        noise,
        spherePoint,
        settings.detailFrequency,
        settings.detailOctaves,
        settings.persistence,
        settings.lacunarity
    ) - 0.5f;

    float ridge = 0.0f;
    float ridgeAmplitude = 1.0f;
    float ridgeAmplitudeSum = 0.0f;

    int ridgeOctaves = settings.ridgeOctaves;
    if (ridgeOctaves < 1)
    {
        ridgeOctaves = 1;
    }

    for (int octave = 0; octave < ridgeOctaves; octave++)
    {
        float octaveValue = SampleRidgedOctave01(
            noise,
            spherePoint,
            settings.ridgeFrequency,
            octave,
            settings.lacunarity
        );

        ridge += octaveValue * ridgeAmplitude;
        ridgeAmplitudeSum += ridgeAmplitude;

        ridgeAmplitude *= settings.persistence;
    }

    if (ridgeAmplitudeSum > 0.0001f)
    {
        ridge /= ridgeAmplitudeSum;
    }

    float landMask = (continent - settings.oceanLevel) /
        (1.0f - settings.oceanLevel);

    landMask = PNClamp01(landMask);
    landMask = landMask * landMask * (3.0f - 2.0f * landMask);

    float value =
        continent +
        detail * settings.detailStrength +
        ridge * settings.ridgeStrength * landMask;

    value = PNClamp01(value);
    value = PNApplyContrast(value, settings.finalContrast);

    return PNClamp01(value);
}

inline Image GenerateSingleOctaveLayerImage(
    int faceResolution,
    float sphereRadius,
    const PerlinNoise& noise,
    float baseFrequency,
    int octave,
    float lacunarity,
    bool ridged
)
{
    if (faceResolution < 2)
    {
        faceResolution = 2;
    }

    int atlasWidth = faceResolution * 3;
    int atlasHeight = faceResolution * 2;

    Image image = GenImageColor(atlasWidth, atlasHeight, BLACK);

    for (int pixelY = 0; pixelY < atlasHeight; pixelY++)
    {
        for (int pixelX = 0; pixelX < atlasWidth; pixelX++)
        {
            Vector3 spherePoint = AtlasPixelToSpherePoint(
                pixelX,
                pixelY,
                faceResolution,
                sphereRadius
            );

            float value;

            if (ridged)
            {
                value = SampleRidgedOctave01(
                    noise,
                    spherePoint,
                    baseFrequency,
                    octave,
                    lacunarity
                );
            }
            else
            {
                value = SampleNormalOctave01(
                    noise,
                    spherePoint,
                    baseFrequency,
                    octave,
                    lacunarity
                );
            }

            ImageDrawPixel(
                &image,
                pixelX,
                pixelY,
                PNGrayColor(value)
            );
        }
    }

    return image;
}

inline Texture2D GeneratePlanetNoiseTextureFromOctaves(
    int faceResolution,
    float sphereRadius,
    const PerlinNoise& noise,
    const PlanetNoiseSettings& settings,
    bool useTerrainPreviewColors
)
{
    if (faceResolution < 2)
    {
        faceResolution = 2;
    }

    int atlasWidth = faceResolution * 3;
    int atlasHeight = faceResolution * 2;

    Image image = GenImageColor(atlasWidth, atlasHeight, BLACK);

    for (int pixelY = 0; pixelY < atlasHeight; pixelY++)
    {
        for (int pixelX = 0; pixelX < atlasWidth; pixelX++)
        {
            Vector3 spherePoint = AtlasPixelToSpherePoint(
                pixelX,
                pixelY,
                faceResolution,
                sphereRadius
            );

            float value = SamplePlanetTerrainValue01(
                noise,
                spherePoint,
                settings
            );

            Color color;

            if (useTerrainPreviewColors)
            {
                color = PNTerrainPreviewColor(value, settings);
            }
            else
            {
                color = PNGrayColor(value);
            }

            ImageDrawPixel(&image, pixelX, pixelY, color);
        }
    }

    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);

    return texture;
}

inline void ExportPlanetNoiseOctaveLayers(
    int faceResolution,
    float sphereRadius,
    const PerlinNoise& noise,
    const PlanetNoiseSettings& settings
)
{
    std::filesystem::create_directories(settings.octaveExportFolder);

    for (int octave = 0; octave < settings.continentOctaves; octave++)
    {
        Image image = GenerateSingleOctaveLayerImage(
            faceResolution,
            sphereRadius,
            noise,
            settings.continentFrequency,
            octave,
            settings.lacunarity,
            false
        );

        std::string path =
            settings.octaveExportFolder +
            "/continent_octave_" +
            std::to_string(octave) +
            ".png";

        ExportImage(image, path.c_str());
        UnloadImage(image);
    }

    for (int octave = 0; octave < settings.detailOctaves; octave++)
    {
        Image image = GenerateSingleOctaveLayerImage(
            faceResolution,
            sphereRadius,
            noise,
            settings.detailFrequency,
            octave,
            settings.lacunarity,
            false
        );

        std::string path =
            settings.octaveExportFolder +
            "/detail_octave_" +
            std::to_string(octave) +
            ".png";

        ExportImage(image, path.c_str());
        UnloadImage(image);
    }

    for (int octave = 0; octave < settings.ridgeOctaves; octave++)
    {
        Image image = GenerateSingleOctaveLayerImage(
            faceResolution,
            sphereRadius,
            noise,
            settings.ridgeFrequency,
            octave,
            settings.lacunarity,
            true
        );

        std::string path =
            settings.octaveExportFolder +
            "/ridge_octave_" +
            std::to_string(octave) +
            ".png";

        ExportImage(image, path.c_str());
        UnloadImage(image);
    }
}

inline Vector3 GetPlateNoiseOffset(
    int plateId,
    unsigned int seed,
    float offsetDistance
)
{
    std::uint32_t hashX = PlanetNoiseHash3D(plateId, 11, 17, seed);
    std::uint32_t hashY = PlanetNoiseHash3D(plateId, 23, 31, seed);
    std::uint32_t hashZ = PlanetNoiseHash3D(plateId, 41, 47, seed);

    float x = PlanetNoiseHashToFloat01(hashX) * 2.0f - 1.0f;
    float y = PlanetNoiseHashToFloat01(hashY) * 2.0f - 1.0f;
    float z = PlanetNoiseHashToFloat01(hashZ) * 2.0f - 1.0f;

    Vector3 offset = { x, y, z };

    if (Vector3Length(offset) < 0.0001f)
    {
        offset = { 1.0f, 0.0f, 0.0f };
    }

    offset = Vector3Normalize(offset);
    offset = Vector3Scale(offset, offsetDistance);

    return offset;
}

inline std::vector<Vector3> BuildPlateNoiseOffsets(
    int plateCount,
    unsigned int seed,
    float offsetDistance
)
{
    std::vector<Vector3> offsets;
    offsets.resize(plateCount);

    for (int plateId = 0; plateId < plateCount; plateId++)
    {
        offsets[plateId] = GetPlateNoiseOffset(
            plateId,
            seed,
            offsetDistance
        );
    }

    return offsets;
}

inline Color NoiseValueToGrayscaleColor(float value)
{
    value = PlanetNoiseClamp01(value);

    unsigned char gray = (unsigned char)(value * 255.0f);

    return Color{ gray, gray, gray, 255 };
}

inline Color NoiseValueToTerrainPreviewColor(
    float value,
    const PlanetNoiseSettings& settings
)
{
    value = PlanetNoiseClamp01(value);

    if (value < settings.oceanLevel)
    {
        return Color{ 20, 70, 160, 255 };
    }

    if (value < 0.60f)
    {
        return Color{ 45, 145, 75, 255 };
    }

    if (value < 0.78f)
    {
        return Color{ 125, 105, 65, 255 };
    }

    return Color{ 235, 235, 235, 255 };
}

inline float BlendPlateNoiseByDistance(
    Vector3 spherePoint,
    const std::vector<PlateDistanceResult>& closestPlates,
    const std::vector<Vector3>& plateNoiseOffsets,
    const PerlinNoise& noise,
    const PlanetNoiseSettings& settings
)
{
    if (closestPlates.empty())
    {
        return 0.0f;
    }

    int closestPlateId = closestPlates[0].plateId;

    if (closestPlateId < 0 || closestPlateId >= (int)plateNoiseOffsets.size())
    {
        return 0.0f;
    }

    int maxBlendPlates = settings.maxNoiseBlendPlates;
    if (maxBlendPlates < 1)
    {
        maxBlendPlates = 1;
    }

    if (settings.plateNoiseBlendWidth <= 0.0001f || maxBlendPlates == 1)
    {
        return noise.planetValue01WithOffset(
            spherePoint,
            plateNoiseOffsets[closestPlateId],
            settings
        );
    }

    float closestDistance = closestPlates[0].distance;

    float totalWeight = 0.0f;
    float blendedValue = 0.0f;

    int usedPlateCount = 0;

    for (const PlateDistanceResult& result : closestPlates)
    {
        if (usedPlateCount >= maxBlendPlates)
        {
            break;
        }

        int plateId = result.plateId;

        if (plateId < 0 || plateId >= (int)plateNoiseOffsets.size())
        {
            continue;
        }

        float distanceDifference = result.distance - closestDistance;

        if (distanceDifference > settings.plateNoiseBlendWidth)
        {
            continue;
        }

        float t = 1.0f - distanceDifference / settings.plateNoiseBlendWidth;
        float weight = PlanetNoiseSmoothStep01(t);

        float plateValue = noise.planetValue01WithOffset(
            spherePoint,
            plateNoiseOffsets[plateId],
            settings
        );

        blendedValue += plateValue * weight;
        totalWeight += weight;

        usedPlateCount++;
    }

    if (totalWeight <= 0.0001f)
    {
        return noise.planetValue01WithOffset(
            spherePoint,
            plateNoiseOffsets[closestPlateId],
            settings
        );
    }

    return PlanetNoiseClamp01(blendedValue / totalWeight);
}

inline Texture2D GeneratePlanetNoiseTexture(
    int faceResolution,
    float sphereRadius,
    float frequency,
    int octaves,
    float persistence,
    float lacunarity,
    unsigned int seed
)
{
    if (faceResolution < 2)
    {
        faceResolution = 2;
    }

    PlanetNoiseSettings settings;
    settings.continentFrequency = frequency;
    settings.continentOctaves = octaves;
    settings.persistence = persistence;
    settings.lacunarity = lacunarity;

    PerlinNoise noise(seed);

    int atlasWidth = faceResolution * 3;
    int atlasHeight = faceResolution * 2;

    Image image = GenImageColor(atlasWidth, atlasHeight, BLACK);

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

            float value = noise.planetValue01(spherePoint, settings);
            Color color = NoiseValueToGrayscaleColor(value);

            ImageDrawPixel(&image, pixelX, pixelY, color);
        }
    }

    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);

    return texture;
}

inline Texture2D GeneratePlanetPlateNoiseTexture(
    int faceResolution,
    float sphereRadius,
    const PlanetPlateData& plateData,
    const PlanetNoiseSettings& settings,
    unsigned int seed,
    bool useTerrainColors = false
)
{
    if (faceResolution < 2)
    {
        faceResolution = 2;
    }

    int atlasWidth = faceResolution * 3;
    int atlasHeight = faceResolution * 2;

    Image image = GenImageColor(atlasWidth, atlasHeight, BLACK);

    if (plateData.points.empty())
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

    std::vector<Vector3> plateNoiseOffsets = BuildPlateNoiseOffsets(
        finalPlateCount,
        seed,
        settings.plateNoiseOffsetDistance
    );

    PerlinNoise noise(seed);

    int maxBlendPlates = settings.maxNoiseBlendPlates;
    if (maxBlendPlates < 1)
    {
        maxBlendPlates = 1;
    }

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

            float value = BlendPlateNoiseByDistance(
                spherePoint,
                closestPlates,
                plateNoiseOffsets,
                noise,
                settings
            );

            Color color = useTerrainColors
                ? NoiseValueToTerrainPreviewColor(value, settings)
                : NoiseValueToGrayscaleColor(value);

            ImageDrawPixel(&image, pixelX, pixelY, color);
        }
    }

    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);

    return texture;
}
