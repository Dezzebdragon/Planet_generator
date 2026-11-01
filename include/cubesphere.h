#pragma once
#include "raylib.h"
#include "raymath.h"

inline Vector3 CubeFacePoint(int face, float u, float v)
{

    switch (face)
    {
    case 0: return Vector3{ u,  v,  1.0f };
    case 1: return Vector3{ -u,  v, -1.0f };
    case 2: return Vector3{ 1.0f, v, -u };
    case 3: return Vector3{ -1.0f, v,  u };
    case 4: return Vector3{ u,  1.0f, -v };
    case 5: return Vector3{ u, -1.0f,  v };
    }

    return Vector3{ 0.0f, 0.0f, 1.0f };
}

inline Mesh GenMeshCubeSphere(float radius, int subdivisions, int atlasFaceResolution = 0)
{
    if (subdivisions < 1)
    {
        subdivisions = 1;
    }

    int quadsPerFace = subdivisions * subdivisions;
    int trianglesPerFace = quadsPerFace * 2;
    int verticesPerFace = trianglesPerFace * 3;

    Mesh mesh = { 0 };

    mesh.vertexCount = verticesPerFace * 6;
    mesh.triangleCount = mesh.vertexCount / 3;

    mesh.vertices = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
    mesh.normals = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
    mesh.texcoords = (float*)MemAlloc(mesh.vertexCount * 2 * sizeof(float));

    mesh.colors = nullptr;

    int vertexIndex = 0;

    const float atlasTileW = 1.0f / 3.0f;
    const float atlasTileH = 1.0f / 2.0f;

    for (int face = 0; face < 6; face++)
    {
        int tileX = face % 3;
        int tileY = face / 3;

        for (int y = 0; y < subdivisions; y++)
        {
            for (int x = 0; x < subdivisions; x++)
            {

                float u0 = (float)x / (float)subdivisions;
                float u1 = (float)(x + 1) / (float)subdivisions;

                float v0 = (float)y / (float)subdivisions;
                float v1 = (float)(y + 1) / (float)subdivisions;

                Vector2 localUV[4] = {
                    Vector2{ u0, v0 },
                    Vector2{ u1, v0 },
                    Vector2{ u1, v1 },
                    Vector2{ u0, v1 }
                };

                Vector3 cubeP[4];

                for (int i = 0; i < 4; i++)
                {

                    float cubeU = localUV[i].x * 2.0f - 1.0f;
                    float cubeV = 1.0f - localUV[i].y * 2.0f;

                    cubeP[i] = CubeFacePoint(face, cubeU, cubeV);
                }

                int tri[6] = {
                    0, 3, 2,
                    0, 2, 1
                };

                for (int i = 0; i < 6; i++)
                {
                    int corner = tri[i];

                    Vector3 normal = Vector3Normalize(cubeP[corner]);
                    Vector3 pos = Vector3Scale(normal, radius);

                    mesh.vertices[vertexIndex * 3 + 0] = pos.x;
                    mesh.vertices[vertexIndex * 3 + 1] = pos.y;
                    mesh.vertices[vertexIndex * 3 + 2] = pos.z;

                    mesh.normals[vertexIndex * 3 + 0] = normal.x;
                    mesh.normals[vertexIndex * 3 + 1] = normal.y;
                    mesh.normals[vertexIndex * 3 + 2] = normal.z;

                    float tileU = localUV[corner].x;
                    float tileV = localUV[corner].y;

                    if (atlasFaceResolution > 1)
                    {
                        float res = (float)atlasFaceResolution;
                        tileU = (0.5f + tileU * (res - 1.0f)) / res;
                        tileV = (0.5f + tileV * (res - 1.0f)) / res;
                    }

                    float atlasU = ((float)tileX + tileU) * atlasTileW;
                    float atlasV = ((float)tileY + tileV) * atlasTileH;

                    mesh.texcoords[vertexIndex * 2 + 0] = atlasU;
                    mesh.texcoords[vertexIndex * 2 + 1] = atlasV;

                    vertexIndex++;
                }
            }
        }
    }

    return mesh;
}
