#include "raylib.h"
#include "raymath.h"

#include "cubesphere.h"

#include "planetDebugConfig.h"
#include "helperFunc.h"
#include "planetSettings.h"
#include "planetPlates.h"
#include "helperDraw.h"

#include "planetTerrain.h"
#include "planetClimate.h"
#include "planetTextures.h"

int main(void)
{
    const int screenWidth = 1800;
    const int screenHeight = 850;

    InitWindow(
        screenWidth,
        screenHeight,
        "Planet Generator - Phase 1 Plates"
    );

    Camera3D camera = { 0 };

    camera.position =
    {
        10.0f,
        10.0f,
        10.0f
    };
    camera.target =
    {
        0.0f,
        0.0f,
        0.0f
    };

    camera.up =
    {
        0.0f,
        1.0f,
        0.0f
    };

    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    DisableCursor();
    SetTargetFPS(60);

    unsigned int rootSeed = 123456789u;

    PlanetGenerationSettings planetSettings =
        GeneratePlanetSettings(rootSeed);

    TraceLog(
        LOG_INFO,
        TextFormat(
            "Planet root seed: %u",
            planetSettings.rootSeed
        )
    );

    PlanetPlateMap plateMap =
        GeneratePlanetPlateMap(
            planetSettings
        );

    double bakeStartTime = GetTime();

    PlanetSurfaceMap surface =
        GeneratePlanetTerrainSurface(
            plateMap,
            planetSettings
        );

    ApplyPlanetClimateToSurface(
        surface,
        planetSettings
    );

    PlanetMapTextures mapTextures =
        GeneratePlanetMapTextures(
            surface,
            plateMap,
            planetSettings
        );

    TraceLog(
        LOG_INFO,
        TextFormat(
            "Planet surface bake: %dx%d texels, elevation %.3f..%.3f, %.2f s",
            surface.width,
            surface.height,
            surface.minElevation,
            surface.maxElevation,
            GetTime() - bakeStartTime
        )
    );

    Mesh mesh =
        GenMeshCubeSphere(
            planetSettings.planetRadius,
            planetSettings.terrain.meshSubdivisions,
            surface.faceResolution
        );

    ApplyPlanetSurfaceToMesh(
        mesh,
        surface,
        planetSettings
    );

    UploadMesh(
        &mesh,
        false
    );

    Model planet =
        LoadModelFromMesh(mesh);

    PlanetMapMode mapMode = PLANET_MAP_BIOME;
    bool showMapTexture = true;

    ApplyPlanetMapTexture(
        planet,
        mapTextures,
        mapMode,
        showMapTexture
    );

#ifdef PLANET_DEBUG_PHASE_1

    PlanetPlateDebugDrawSettings plateDebugSettings =
        CreateDefaultPlanetPlateDebugDrawSettings();

    plateDebugSettings.drawPlatePoints = true;
    plateDebugSettings.drawLargeSeeds = true;
    plateDebugSettings.drawSmallSeeds = true;
    plateDebugSettings.drawEdgePoints = false;
    plateDebugSettings.drawDriftEvents = false;

#endif

    bool showWires = false;
    bool showPlatePoints = false;
    bool showDriftEvents = false;
    bool showEdgePoints = false;

    while (!WindowShouldClose())
    {
        UpdateCamera(
            &camera,
            CAMERA_FREE
        );

        if (IsKeyPressed(KEY_Z))
        {
            camera.target =
            {
                0.0f,
                0.0f,
                0.0f
            };
        }

        if (IsKeyPressed(KEY_ONE))
        {
            showWires =
                !showWires;
        }

        if (IsKeyPressed(KEY_TWO))
        {
            showPlatePoints =
                !showPlatePoints;
        }

        if (IsKeyPressed(KEY_THREE))
        {
            showDriftEvents =
                !showDriftEvents;
        }

        if (IsKeyPressed(KEY_FOUR))
        {
            showEdgePoints =
                !showEdgePoints;
        }

        if (IsKeyPressed(KEY_FIVE))
        {
            mapMode =
                (PlanetMapMode)((mapMode + 1) % PLANET_MAP_COUNT);

            ApplyPlanetMapTexture(
                planet,
                mapTextures,
                mapMode,
                showMapTexture
            );
        }

        if (IsKeyPressed(KEY_SIX))
        {
            showMapTexture =
                !showMapTexture;

            ApplyPlanetMapTexture(
                planet,
                mapTextures,
                mapMode,
                showMapTexture
            );
        }

        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode3D(camera);

        DrawModel(
            planet,
            Vector3{ 0.0f, 0.0f, 0.0f },
            1.0f,
            WHITE
        );

        if (showWires)
        {
            DrawModelWires(
                planet,
                Vector3{ 0.0f, 0.0f, 0.0f },
                1.0f,
                BLACK
            );
        }

#ifdef PLANET_DEBUG_PHASE_1

        plateDebugSettings.drawPlatePoints =
            showPlatePoints;

        plateDebugSettings.drawLargeSeeds =
            showPlatePoints;

        plateDebugSettings.drawSmallSeeds =
            showPlatePoints;

        plateDebugSettings.drawEdgePoints =
            showEdgePoints;

        plateDebugSettings.drawDriftEvents =
            showDriftEvents;

#if defined(PLANET_DEBUG_DRAW_PHASE1_DRIFT_EVENTS) && defined(PLANET_ENABLE_PHASE1_DEBUG_DATA)

        DrawPlanetPhase1DriftEvents(
            plateMap,
            plateDebugSettings
        );

#endif
#endif

        DrawGrid(
            10,
            1.0f
        );

        EndMode3D();

#ifdef PLANET_DEBUG_PHASE_1
#if defined(PLANET_DEBUG_DRAW_PHASE1_POINTS)

        DrawPlanetPhase1PlatePoints(
            plateMap,
            plateDebugSettings,
            camera
        );

#endif
#endif

        DrawText(
            "PHASE 1: PLATE GENERATION",
            10,
            10,
            20,
            WHITE
        );

        DrawText(
            "1 wires | 2 plates | 3 drift | 4 finalized edge points | Z center camera",
            10,
            35,
            18,
            LIGHTGRAY
        );

        DrawText(
            TextFormat(
                "5 next map | 6 map on/off   MAP: %s%s",
                PlanetMapModeName(mapMode),
                showMapTexture ? "" : " (off)"
            ),
            10,
            58,
            18,
            LIGHTGRAY
        );

        DrawFPS(
            10,
            85
        );

        EndDrawing();
    }

    ApplyPlanetMapTexture(
        planet,
        mapTextures,
        mapMode,
        false
    );

    UnloadPlanetMapTextures(mapTextures);

    UnloadModel(planet);
    CloseWindow();

    return 0;
}
