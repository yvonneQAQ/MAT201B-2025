#include "al/app/al_App.hpp"
#include "al/math/al_Random.hpp"
using namespace al;
#include <fstream> // for slurp()
#include <string> // for slurp()

Vec3f rvec() { return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()); }
RGB rcolor() { return RGB(rnd::uniform(), rnd::uniform(), rnd::uniform()); }

std::string slurp(std::string fileName); // only a declaration

class MyApp : public App {
    
    Mesh mesh;
    ShaderProgram shader;

    void onCreate() override {
        mesh.primitive(Mesh::POINTS);
        for (int i = 0; i < 100; ++i) {
            mesh.vertex(rvec());
            mesh.color(rcolor());
            mesh.texCoord(0.1, 0);
        }
    
        if (! shader.compile(slurp("../point-vertex.glsl"), slurp("../point-fragment.glsl"), slurp("../point-geometry.glsl"))) {
            printf("Shader failed to compile\n");
            exit(1);
        }
    }

    void onDraw(Graphics& g) override {
        g.clear(0.1);
        g.shader(shader);
        g.shader().uniform("pointSize", 0.1);
        g.blending(true);
        g.blendTrans();
        g.depthTesting(true);
        g.draw(mesh);
    }

    bool onKeyDown(const Keyboard& k) override {
        if (k.key() == 'q') {
            std::cout << "Exiting..." << std::endl;
            quit();
        }

        if (k.key() == ' ') {
            mesh.reset(); // deletes all vertices and colors
            for (int i = 0; i < 100; ++i) {
                mesh.vertex(rvec());
                mesh.color(rcolor());
                mesh.texCoord(0.1, 0);
            }
        }

        return true;
    }
};
int main() { MyApp().start(); }


std::string slurp(std::string fileName) {
    std::fstream file(fileName);
    std::string returnValue = "";
    while (file.good()) {
      std::string line;
      getline(file, line);
      returnValue += line + "\n";
    }
    return returnValue;
  }