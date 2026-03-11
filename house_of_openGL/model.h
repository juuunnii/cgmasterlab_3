#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"
#include "tgaimage.h"

class Model {
private:
    std::vector<Vec3f> verts_;
    std::vector<Vec2f> uv_;
    std::vector<Vec3f> norms_;  //нормализация
    std::vector<std::vector<int> > faces_;
    std::vector<std::vector<int> > faces_uv_;
    std::vector<std::vector<int> > faces_norm_;  //индексы нормали для каждой грани
    TGAImage diffusemap_;

    void load_texture(std::string filename, const char* suffix, TGAImage& img);

public:
    Model(const char* filename);
    ~Model();

    int nverts();
    int nfaces();
    int nnorms();

    Vec3f vert(int i);
    Vec2f uv(int i);
    Vec3f norm(int i);

    TGAColor diffuse(Vec2f uv);

    std::vector<int> face(int idx);
    std::vector<int> face_uv(int idx);
    std::vector<int> face_norm(int idx);

    //названия методов
    int vertex_count() { return nverts(); }
    int face_count() { return nfaces(); }
    int normal_count() { return nnorms(); }

    Vec3f get_vertex(int i) { return vert(i); }
    Vec2f get_uv(int i) { return uv(i); }
    Vec3f get_normal(int i) { return norm(i); }

    TGAColor get_diffuse_color(Vec2f uv) { return diffuse(uv); }

    std::vector<int> get_face(int idx) { return face(idx); }
    std::vector<int> get_face_uv(int idx) { return face_uv(idx); }
    std::vector<int> get_face_norm(int idx) { return face_norm(idx); }
};

#endif //__MODEL_H__