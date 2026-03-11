#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "geometry.h"
#include "matrix.h"

class Camera {
public:
    Vec3f eye;
    Vec3f center;
    Vec3f up;
    float zoom;
    float focus;

    Camera(const Vec3f& e = Vec3f(1, 1, 3),
        const Vec3f& c = Vec3f(0, 0, 0),
        const Vec3f& u = Vec3f(0, 1, 0))
        : eye(e), center(c), up(u), zoom(1.0f), focus(4.0f) {}

    Matrix get_view_matrix() const {
        Vec3f z = (eye - center);
        z.make_unit();
        Vec3f x = (up ^ z);
        x.make_unit();
        Vec3f y = (z ^ x);
        y.make_unit();

        Matrix m = Matrix::create_identity(4);
        for (int i = 0; i < 3; ++i) {
            m[0][i] = x[i];
            m[1][i] = y[i];
            m[2][i] = z[i];
        }

        Matrix t = Matrix::create_identity(4);
        t[0][3] = -eye.x;
        t[1][3] = -eye.y;
        t[2][3] = -eye.z;

        return multiply_matrices(m, t);
    }

    Matrix get_projection_matrix() const {
        Matrix p = Matrix::create_identity(4);
        float f = focus / zoom;
        p[3][2] =  ( - 2.f / f)/3;
        return p;
    }

    void adjust_zoom(float factor) {
        zoom *= factor;
    }

    // הכÿ סמגלוסעטלמסעט
    Matrix view() const { return get_view_matrix(); }
    Matrix projection() const { return get_projection_matrix(); }
    void changeZoom(float factor) { adjust_zoom(factor); }
};

#endif // __CAMERA_H__