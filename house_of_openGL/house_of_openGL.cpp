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

Model* model = NULL;
const int width = 800;
const int height = 800;
const int depth = 255;

// фоновое изображение
TGAImage background;

Matrix create_viewport(int x, int y, int w, int h) {
    Matrix m = Matrix::create_identity(4);
    m[0][3] = x + w / 2.f;
    m[1][3] = y + h / 2.f;
    m[2][3] = depth / 2.f;

    m[0][0] = w / 2.f;
    m[1][1] = h / 2.f;
    m[2][2] = depth / 2.f;

    return m;
}

Vec3f barycentric_coords(Vec3f A, Vec3f B, Vec3f C, Vec3f P) {
    Vec3f s[2];
    for (int i = 2; i--; ) {
        s[i][0] = C[i] - A[i];
        s[i][1] = B[i] - A[i];
        s[i][2] = A[i] - P[i];
    }

    Vec3f u = s[0] ^ s[1];

    if (std::abs(u[2]) > 1e-2)
        return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
    return Vec3f(-1, 1, 1);
}

void draw_triangle(Vec3i* vertices, Vec2f* tex_coords, Vec3f* normals, float* depth_buffer, TGAImage& image, Vec3f light_dir) {
    Vec2i bboxmin(image.get_width() - 1, image.get_height() - 1);
    Vec2i bboxmax(0, 0);
    Vec2i clamp(image.get_width() - 1, image.get_height() - 1);

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            bboxmin[j] = std::max(0, std::min(bboxmin[j], vertices[i][j]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], vertices[i][j]));
        }
    }

    Vec3f P;
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
            Vec3f bc_screen = barycentric_coords(
                Vec3f(vertices[0].x, vertices[0].y, vertices[0].z),
                Vec3f(vertices[1].x, vertices[1].y, vertices[1].z),
                Vec3f(vertices[2].x, vertices[2].y, vertices[2].z),
                P
            );

            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
                continue;

            P.z = vertices[0].z * bc_screen.x + vertices[1].z * bc_screen.y + vertices[2].z * bc_screen.z;

            int idx = (int)P.x + (int)P.y * width;
            if (depth_buffer[idx] < P.z) {
                depth_buffer[idx] = P.z;

                //интерполяция UV координат
                Vec2f uv;
                uv.x = tex_coords[0].x * bc_screen.x + tex_coords[1].x * bc_screen.y + tex_coords[2].x * bc_screen.z;
                uv.y = tex_coords[0].y * bc_screen.x + tex_coords[1].y * bc_screen.y + tex_coords[2].y * bc_screen.z;

                //интерполяция нормали
                Vec3f normal;
                normal.x = normals[0].x * bc_screen.x + normals[1].x * bc_screen.y + normals[2].x * bc_screen.z;
                normal.y = normals[0].y * bc_screen.x + normals[1].y * bc_screen.y + normals[2].y * bc_screen.z;
                normal.z = normals[0].z * bc_screen.x + normals[1].z * bc_screen.y + normals[2].z * bc_screen.z;

                normal.make_unit();

                float intensity = normal * light_dir;
                intensity = std::max(0.0f, intensity);

                TGAColor color = model->diffuse(uv);

                image.set(P.x, P.y, TGAColor(
                    static_cast<unsigned char>(color.r * intensity),
                    static_cast<unsigned char>(color.g * intensity),
                    static_cast<unsigned char>(color.b * intensity),
                    255
                ));
            }
        }
    }
}

//загрузка фонового изображения
bool load_background(const char* filename) {
    if (!background.read_tga_file(filename)) {
        std::cerr << "ошибка загрузки задника: " << filename << std::endl;
        return false;
    }
    background.flip_vertically();

    //масштабируем под размер экрана, если надобно
    if (background.get_width() != width || background.get_height() != height) {
        std::cerr << "размер изображения (" << background.get_width() << "x" << background.get_height()
            << ") не соответсвует требуемуму разрешению (" << width << "x" << height << ")." << std::endl;
    }

    std::cerr << "успешная загрузка задника: " << filename << std::endl;
    return true;
}

//функция для применения фона к изображению
void apply_background(TGAImage& image) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            
            TGAColor bg_pixel = background.get(x % background.get_width(), y % background.get_height());

            TGAColor current_pixel = image.get(x, y);

            if (current_pixel.r == 0 && current_pixel.g == 0 && current_pixel.b == 0) {
                image.set(x, y, bg_pixel);
            }
        }
    }
}

int main() {
    setlocale(LC_ALL, "rus");
    model = new Model("african_head.obj");

    if (!model->nfaces()) {
        std::cerr << "ошибка загрзки модели!" << std::endl;
        return -1;
    }

  
    if (!load_background("background.tga")) {
        std::cerr << "свистопляска без фона..." << std::endl;
    }

    TGAImage image(width, height, TGAImage::RGB);

    //сначала применяем фон (если он загружен)
    if (background.get_width() > 0 && background.get_height() > 0) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                image.set(x, y, background.get(x % background.get_width(), y % background.get_height()));
            }
        }
    }
    else {
        //если фона нет, заливаем серым цветом
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                image.set(x, y, TGAColor(100, 100, 100, 255));
            }
        }
    }

    float* zbuffer = new float[width * height];
    for (int i = 0; i < width * height; i++) {
        zbuffer[i] = -std::numeric_limits<float>::max();
    }

   
    Camera camera(
        Vec3f(1, 1, 5),  
        Vec3f(0, 0, 0),
        Vec3f(0, 1, 0)
    );

    Matrix View = camera.get_view_matrix();
    Matrix Projection = camera.get_projection_matrix();
    Matrix ViewPort = create_viewport(0, 0, width, height);

    Vec3f light_dir(1, 1, 1);
    light_dir.make_unit();

    std::cerr << "рендерим " << model->nfaces() << " грани..." << std::endl;

    // рендерим модель поверх фона
    for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        std::vector<int> face_uv = model->face_uv(i);
        std::vector<int> face_norm = model->face_norm(i);

        Vec3i screen_coords[3];
        Vec2f uv_coords[3];
        Vec3f normals[3];

        for (int j = 0; j < 3; j++) {
            Vec3f v = model->vert(face[j]);

            Matrix v4 = to_homogeneous(v);
            Matrix clip = multiply_matrices(ViewPort, multiply_matrices(Projection, multiply_matrices(View, v4)));
            Vec3f screenf = from_homogeneous(clip);

            screen_coords[j] = Vec3i(
                static_cast<int>(screenf.x),
                static_cast<int>(screenf.y),
                static_cast<int>(screenf.z)
            );

            uv_coords[j] = model->uv(face_uv[j]);
            normals[j] = model->norm(face_norm[j]);
        }

        draw_triangle(screen_coords, uv_coords, normals, zbuffer, image, light_dir);
    }

    image.flip_vertically();

    if (image.write_tga_file("output_with_background.tga")) {
        std::cerr << "всё успешно сохранено в output_with_background.tga" << std::endl;
    }
    else {
        std::cerr << "ошибка загрузки" << std::endl;
    }

    delete[] zbuffer;
    delete model;

    return 0;
}
