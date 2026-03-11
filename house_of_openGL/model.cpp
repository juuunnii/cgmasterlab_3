#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"

Model::Model(const char* filename) : verts_(), uv_(), norms_(), faces_(), faces_uv_(), faces_norm_(), diffusemap_() {
    std::ifstream in;
    in.open(filename, std::ifstream::in);
    if (in.fail()) return;
    std::string line;
    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;
        if (!line.compare(0, 2, "v ")) {
            iss >> trash;
            Vec3f v;
            for (int i = 0; i < 3; i++) iss >> v.raw[i];
            verts_.push_back(v);
        }
        else if (!line.compare(0, 3, "vt ")) {
            iss >> trash >> trash;
            Vec2f uv;
            for (int i = 0; i < 2; i++) iss >> uv.raw[i];
            uv_.push_back(uv);
        }
        else if (!line.compare(0, 3, "vn ")) {
            iss >> trash >> trash;
            Vec3f n;
            for (int i = 0; i < 3; i++) iss >> n.raw[i];
            norms_.push_back(n);
        }
        else if (!line.compare(0, 2, "f ")) {
            std::vector<int> f;
            std::vector<int> f_uv;
            std::vector<int> f_norm;
            int v_idx, vt_idx, vn_idx;
            iss >> trash;
            while (iss >> v_idx >> trash >> vt_idx >> trash >> vn_idx) {
                f.push_back(v_idx - 1);
                f_uv.push_back(vt_idx - 1);
                f_norm.push_back(vn_idx - 1);
            }
            faces_.push_back(f);
            faces_uv_.push_back(f_uv);
            faces_norm_.push_back(f_norm);
        }
    }
    std::cerr << "\033[32m" << "кол - во вершин : " << verts_.size() << "\nкол - во текстурных координат : " << uv_.size()
        << "\nкол-во нормалей: " << norms_.size() << "\nкол-во граней: " << faces_.size() << "\033[0m" << std::endl;

    load_texture(filename, "_diffuse.tga", diffusemap_);

    if (diffusemap_.get_width() == 0) {
        std::cerr << "предупреждение: не удалось загрузить текстуру!" << std::endl;
    }
}

Model::~Model() {
}

int Model::nverts() {
    return (int)verts_.size();
}

int Model::nfaces() {
    return (int)faces_.size();
}

int Model::nnorms() {
    return (int)norms_.size();
}

std::vector<int> Model::face(int idx) {
    return faces_[idx];
}

std::vector<int> Model::face_uv(int idx) {
    return faces_uv_[idx];
}

std::vector<int> Model::face_norm(int idx) {
    return faces_norm_[idx];
}

Vec3f Model::vert(int i) {
    return verts_[i];
}

Vec2f Model::uv(int i) {
    return uv_[i];
}

Vec3f Model::norm(int i) {
    return norms_[i];
}

void Model::load_texture(std::string filename, const char* suffix, TGAImage& img) {
    std::string texfile(filename);
    size_t dot = texfile.find_last_of(".");
    if (dot != std::string::npos) {
        texfile = texfile.substr(0, dot) + std::string(suffix); 
        std::cerr << "\033[32m" << "подгрузка текстуры: " << texfile << "\033[0m" << std::endl;
        img.read_tga_file(texfile.c_str());
        img.flip_vertically();
    }
}

TGAColor Model::diffuse(Vec2f uvf) {
    Vec2i uv(static_cast<int>(uvf.x * diffusemap_.get_width()),
        static_cast<int>(uvf.y * diffusemap_.get_height()));
    return diffusemap_.get(uv.x, uv.y);
}