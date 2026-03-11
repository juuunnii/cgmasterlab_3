#include <vector>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <iostream>
#include "tgaimage.h"
#include "matrix.h"
#include "model.h"
#include "camera.h"
#include "geometry.h"

Model* g_model = nullptr;
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 800;
const int DEPTH_MAX = 255;

Matrix create_viewport_matrix(int x, int y, int w, int h) {
    Matrix viewport = Matrix::create_identity(4);
    viewport[0][3] = x + w / 2.f;
    viewport[1][3] = y + h / 2.f;
    viewport[2][3] = DEPTH_MAX / 2.f;

    viewport[0][0] = w / 2.f;
    viewport[1][1] = h / 2.f;
    viewport[2][2] = DEPTH_MAX / 2.f;

    return viewport;
}

Vec3f calculate_barycentric(Vec3f vertexA, Vec3f vertexB, Vec3f vertexC, Vec3f pointP) {
    Vec3f crossVectors[2];
    for (int i = 2; i--; ) {
        crossVectors[i][0] = vertexC[i] - vertexA[i];
        crossVectors[i][1] = vertexB[i] - vertexA[i];
        crossVectors[i][2] = vertexA[i] - pointP[i];
    }

    Vec3f crossProduct = crossVectors[0] ^ crossVectors[1];

    if (std::abs(crossProduct[2]) > 1e-2)
        return Vec3f(1.f - (crossProduct.x + crossProduct.y) / crossProduct.z,
            crossProduct.y / crossProduct.z,
            crossProduct.x / crossProduct.z);
    return Vec3f(-1, 1, 1);
}

void render_triangle(Vec3i* screenVertices, Vec2f* textureCoords, Vec3f* vertexNormals,
    float* depthBuffer, TGAImage& outputImage, Vec3f lightDirection) {
    Vec2i minBounds(outputImage.get_width() - 1, outputImage.get_height() - 1);
    Vec2i maxBounds(0, 0);
    Vec2i imageBounds(outputImage.get_width() - 1, outputImage.get_height() - 1);

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            minBounds[j] = std::max(0, std::min(minBounds[j], screenVertices[i][j]));
            maxBounds[j] = std::min(imageBounds[j], std::max(maxBounds[j], screenVertices[i][j]));
        }
    }

    Vec3f currentPixel;
    for (currentPixel.x = minBounds.x; currentPixel.x <= maxBounds.x; currentPixel.x++) {
        for (currentPixel.y = minBounds.y; currentPixel.y <= maxBounds.y; currentPixel.y++) {
            Vec3f barycentricScreen = calculate_barycentric(
                Vec3f(screenVertices[0].x, screenVertices[0].y, screenVertices[0].z),
                Vec3f(screenVertices[1].x, screenVertices[1].y, screenVertices[1].z),
                Vec3f(screenVertices[2].x, screenVertices[2].y, screenVertices[2].z),
                currentPixel
            );

            if (barycentricScreen.x < 0 || barycentricScreen.y < 0 || barycentricScreen.z < 0)
                continue;

            currentPixel.z = screenVertices[0].z * barycentricScreen.x +
                screenVertices[1].z * barycentricScreen.y +
                screenVertices[2].z * barycentricScreen.z;

            int bufferIndex = (int)currentPixel.x + (int)currentPixel.y * SCREEN_WIDTH;
            if (depthBuffer[bufferIndex] < currentPixel.z) {
                depthBuffer[bufferIndex] = currentPixel.z;

                // Интерполяция UV координат
                Vec2f interpolatedUV;
                interpolatedUV.x = textureCoords[0].x * barycentricScreen.x +
                    textureCoords[1].x * barycentricScreen.y +
                    textureCoords[2].x * barycentricScreen.z;
                interpolatedUV.y = textureCoords[0].y * barycentricScreen.x +
                    textureCoords[1].y * barycentricScreen.y +
                    textureCoords[2].y * barycentricScreen.z;

                // Интерполяция нормали
                Vec3f interpolatedNormal;
                interpolatedNormal.x = vertexNormals[0].x * barycentricScreen.x +
                    vertexNormals[1].x * barycentricScreen.y +
                    vertexNormals[2].x * barycentricScreen.z;
                interpolatedNormal.y = vertexNormals[0].y * barycentricScreen.x +
                    vertexNormals[1].y * barycentricScreen.y +
                    vertexNormals[2].y * barycentricScreen.z;
                interpolatedNormal.z = vertexNormals[0].z * barycentricScreen.x +
                    vertexNormals[1].z * barycentricScreen.y +
                    vertexNormals[2].z * barycentricScreen.z;

                interpolatedNormal.make_unit();

                float lightIntensity = interpolatedNormal * lightDirection;
                lightIntensity = std::max(0.0f, lightIntensity);

                TGAColor textureColor = g_model->diffuse(interpolatedUV);

                outputImage.set(currentPixel.x, currentPixel.y, TGAColor(
                    static_cast<unsigned char>(textureColor.r * lightIntensity),
                    static_cast<unsigned char>(textureColor.g * lightIntensity),
                    static_cast<unsigned char>(textureColor.b * lightIntensity),
                    255
                ));
            }
        }
    }
}

int main() {

    setlocale(LC_ALL, "rus");
    g_model = new Model("african_head.obj");

    if (!g_model->nfaces()) {
        std::cerr << "\033[32m" << "не смогли загрузить модель..." << "\033[0m" << std::endl;
        return -1;
    }

    TGAImage outputImage(SCREEN_WIDTH, SCREEN_HEIGHT, TGAImage::RGB);

    float* depthBuffer = new float[SCREEN_WIDTH * SCREEN_HEIGHT];
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        depthBuffer[i] = -std::numeric_limits<float>::max();
    }

    Camera renderCamera(
        Vec3f(0, 0, 5),
        Vec3f(0, 0, 0),
        Vec3f(0, 1, 0)
    );

    Matrix viewMatrix = renderCamera.get_view_matrix();
    Matrix projectionMatrix = renderCamera.get_projection_matrix();
    Matrix viewportMatrix = create_viewport_matrix(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    Vec3f lightDirection(0, 1, 1);
    lightDirection.make_unit();

    std::cerr << "\033[32m" << "рендеринг в процессе:  " << g_model->nfaces() << " грани..." << "\033[0m" << std::endl;

    for (int faceIndex = 0; faceIndex < g_model->nfaces(); faceIndex++) {
        std::vector<int> currentFace = g_model->face(faceIndex);
        std::vector<int> currentUV = g_model->face_uv(faceIndex);
        std::vector<int> currentNormals = g_model->face_norm(faceIndex);

        Vec3i screenCoordinates[3];
        Vec2f uvCoordinates[3];
        Vec3f normalVectors[3];

        for (int vertexIndex = 0; vertexIndex < 3; vertexIndex++) {
            Vec3f worldVertex = g_model->vert(currentFace[vertexIndex]);

            Matrix homogeneousVertex = to_homogeneous(worldVertex);
            Matrix clipSpace = multiply_matrices(viewportMatrix,
                multiply_matrices(projectionMatrix,
                    multiply_matrices(viewMatrix, homogeneousVertex)));
            Vec3f screenSpace = from_homogeneous(clipSpace);

            screenCoordinates[vertexIndex] = Vec3i(
                static_cast<int>(screenSpace.x),
                static_cast<int>(screenSpace.y),
                static_cast<int>(screenSpace.z)
            );

            uvCoordinates[vertexIndex] = g_model->uv(currentUV[vertexIndex]);
            normalVectors[vertexIndex] = g_model->norm(currentNormals[vertexIndex]);
        }

        render_triangle(screenCoordinates, uvCoordinates, normalVectors,
            depthBuffer, outputImage, lightDirection);
    }

    outputImage.flip_vertically();

    if (outputImage.write_tga_file("output.tga")) {
        std::cerr << "\033[32m" << "готово!" << "\033[0m" << std::endl;
    }
    else {
        std::cerr << "\033[32m" << "ошибка сохранения" << "\033[0m" << std::endl;
    }

    delete[] depthBuffer;
    delete g_model;

    return 0;
}