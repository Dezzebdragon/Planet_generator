#pragma once

#include "raylib.h"
#include "rlgl.h"

#include "helperFunc.h"
#include "planetSettings.h"
#include "planetPlates.h"
#include "planetTerrain.h"

#include <vector>

enum PlanetMapMode
{
    PLANET_MAP_BIOME = 0,
    PLANET_MAP_HEIGHT,
    PLANET_MAP_TEMPERATURE,
    PLANET_MAP_MOISTURE,
    PLANET_MAP_PLATES,

    PLANET_MAP_COUNT
};

inline const char* PlanetMapModeName(PlanetMapMode mode)
{
    switch (mode)
    {
    case PLANET_MAP_BIOME:       return "BIOME";
    case PLANET_MAP_HEIGHT:      return "HEIGHT";
    case PLANET_MAP_TEMPERATURE: return "TEMPERATURE";
    case PLANET_MAP_MOISTURE:    return "MOISTURE";
    case PLANET_MAP_PLATES:      return "PLATES";
    default:                     return "UNKNOWN";
    }
}

struct PlanetMapTextures
{
    bool valid = false;
    Texture2D textures[PLANET_MAP_COUNT] = {};
};

inline Color PlanetHeightColor(
    float elevation,
    const PlanetTerrainStageSettings& terrain
)
{
    if (elevation < 0.0f)
    {
        float depth01 = PlanetInverseLerp01(0.0f, -terrain.oceanDepth, elevation);

        return PlanetLerpColor(
            Color{ 35, 130, 200, 255 },
            Color{ 5, 20, 80, 255 },
            depth01
        );
    }

    float height01 = PlanetInverseLerp01(0.0f, terrain.landHeight, elevation);

    if (height01 < 0.03f)
    {
        return Color{ 210, 195, 130, 255 };
    }

    if (height01 < 0.35f)
    {
        return PlanetLerpColor(
            Color{ 45, 130, 55, 255 },
            Color{ 90, 160, 70, 255 },
            PlanetInverseLerp01(0.03f, 0.35f, height01)
        );
    }

    if (height01 < 0.60f)
    {
        return PlanetLerpColor(
            Color{ 120, 105, 65, 255 },
            Color{ 145, 120, 80, 255 },
            PlanetInverseLerp01(0.35f, 0.60f, height01)
        );
    }

    if (height01 < 0.85f)
    {
        return PlanetLerpColor(
            Color{ 120, 120, 120, 255 },
            Color{ 190, 190, 190, 255 },
            PlanetInverseLerp01(0.60f, 0.85f, height01)
        );
    }

    return Color{ 245, 245, 245, 255 };
}

inline Color PlanetTemperatureColor(float temperature01)
{
    temperature01 = PlanetClamp01(temperature01);

    if (temperature01 < 0.5f)
    {
        return PlanetLerpColor(
            Color{ 20, 35, 120, 255 },
            Color{ 70, 210, 230, 255 },
            temperature01 / 0.5f
        );
    }

    return PlanetLerpColor(
        Color{ 240, 220, 75, 255 },
        Color{ 210, 45, 30, 255 },
        (temperature01 - 0.5f) / 0.5f
    );
}

inline Color PlanetMoistureColor(float moisture01)
{
    moisture01 = PlanetClamp01(moisture01);

    if (moisture01 < 0.5f)
    {
        return PlanetLerpColor(
            Color{ 210, 185, 95, 255 },
            Color{ 95, 165, 70, 255 },
            moisture01 / 0.5f
        );
    }

    return PlanetLerpColor(
        Color{ 95, 165, 70, 255 },
        Color{ 20, 70, 140, 255 },
        (moisture01 - 0.5f) / 0.5f
    );
}

inline Color PlanetBiomeColor(
    float elevation,
    float temperature,
    float moisture,
    const PlanetTerrainStageSettings& terrain
)
{
    if (elevation < 0.0f)
    {
        if (temperature < 0.18f)
        {
            return Color{ 185, 225, 235, 255 };
        }

        float depth01 = PlanetInverseLerp01(0.0f, -terrain.oceanDepth, elevation);

        return PlanetLerpColor(
            Color{ 25, 120, 185, 255 },
            Color{ 8, 35, 95, 255 },
            depth01
        );
    }

    if (temperature < 0.12f) return Color{ 235, 235, 230, 255 };

    if (temperature < 0.25f)
    {
        if (moisture < 0.35f) return Color{ 160, 155, 135, 255 };
        return Color{ 120, 145, 115, 255 };
    }

    if (moisture < 0.20f) return Color{ 215, 185, 100, 255 };
    if (moisture < 0.35f) return Color{ 150, 170, 75, 255 };

    if (moisture < 0.62f)
    {
        if (temperature > 0.65f) return Color{ 75, 145, 55, 255 };
        return Color{ 70, 125, 65, 255 };
    }

    if (temperature > 0.65f) return Color{ 20, 105, 45, 255 };
    return Color{ 35, 105, 60, 255 };
}

inline Color PlanetMapTexelColor(
    PlanetMapMode mode,
    const PlanetSurfaceMap& surface,
    const PlanetPlateMap& plateMap,
    const PlanetTerrainStageSettings& terrain,
    int index
)
{
    float elevation = surface.elevation[index];

    switch (mode)
    {
    case PLANET_MAP_HEIGHT:
        return PlanetHeightColor(elevation, terrain);

    case PLANET_MAP_TEMPERATURE:
        return surface.hasClimate
            ? PlanetTemperatureColor(surface.temperature01[index])
            : MAGENTA;

    case PLANET_MAP_MOISTURE:
        return surface.hasClimate
            ? PlanetMoistureColor(surface.moisture01[index])
            : MAGENTA;

    case PLANET_MAP_PLATES:
    {
        int plateId = surface.plateIds[index];
        Color color = PlanetPlateIdColor(plateId);

        if (
            plateId >= 0 &&
            plateId < (int)plateMap.finalPlates.size() &&
            plateMap.finalPlates[plateId].crustType == PLANET_CRUST_OCEANIC
            )
        {
            color = PlanetLerpColor(color, BLACK, 0.45f);
        }

        return color;
    }

    case PLANET_MAP_BIOME:
    default:
        return surface.hasClimate
            ? PlanetBiomeColor(
                elevation,
                surface.temperature01[index],
                surface.moisture01[index],
                terrain
            )
            : PlanetHeightColor(elevation, terrain);
    }
}

inline Texture2D GeneratePlanetMapTexture(
    PlanetMapMode mode,
    const PlanetSurfaceMap& surface,
    const PlanetPlateMap& plateMap,
    const PlanetGenerationSettings& settings
)
{
    std::vector<Color> pixels(
        (size_t)surface.width * (size_t)surface.height,
        MAGENTA
    );

    if (surface.valid)
    {
        PlanetParallelForRows(
            surface.height,
            [&](int pixelY)
            {
                for (int pixelX = 0; pixelX < surface.width; pixelX++)
                {
                    int index = pixelY * surface.width + pixelX;

                    pixels[index] = PlanetMapTexelColor(
                        mode,
                        surface,
                        plateMap,
                        settings.terrain,
                        index
                    );
                }
            }
        );
    }

    Image image = { 0 };
    image.data = pixels.data();
    image.width = surface.width;
    image.height = surface.height;
    image.mipmaps = 1;
    image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

    Texture2D texture = LoadTextureFromImage(image);

    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(texture, TEXTURE_WRAP_CLAMP);

    return texture;
}

inline PlanetMapTextures GeneratePlanetMapTextures(
    const PlanetSurfaceMap& surface,
    const PlanetPlateMap& plateMap,
    const PlanetGenerationSettings& settings
)
{
    PlanetMapTextures maps;

    for (int mode = 0; mode < PLANET_MAP_COUNT; mode++)
    {
        maps.textures[mode] = GeneratePlanetMapTexture(
            (PlanetMapMode)mode,
            surface,
            plateMap,
            settings
        );
    }

    maps.valid = surface.valid;

    return maps;
}

inline void ApplyPlanetMapTexture(
    Model& model,
    const PlanetMapTextures& maps,
    PlanetMapMode mode,
    bool enabled = true
)
{
    if (model.materialCount <= 0 || model.materials == nullptr)
    {
        return;
    }

    Texture2D texture = { rlGetTextureIdDefault(), 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };

    if (enabled && maps.valid && mode >= 0 && mode < PLANET_MAP_COUNT)
    {
        texture = maps.textures[mode];
    }

    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
}

inline void UnloadPlanetMapTextures(PlanetMapTextures& maps)
{
    for (int mode = 0; mode < PLANET_MAP_COUNT; mode++)
    {
        if (maps.textures[mode].id != 0)
        {
            UnloadTexture(maps.textures[mode]);
            maps.textures[mode] = Texture2D{};
        }
    }

    maps.valid = false;
}
