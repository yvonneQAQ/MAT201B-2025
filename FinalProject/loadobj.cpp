#include "al/app/al_App.hpp"
#include "al_ext/assets3d/al_Asset.hpp"
#include "al/graphics/al_Mesh.hpp"
#include "al/graphics/al_Shapes.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>
#include <iostream>

using namespace al;
using namespace std;

struct MyApp : App {
    Scene* ascene{nullptr};
    vector<Mesh> meshes;
    double rotationAngle = 0;
    float scale = 1.0f;
    bool modelLoaded = false;
    Mesh errorMesh;  // Mesh for error state

    void onCreate() override {
        // Set up camera
        nav().pos(0, 0, 3);
        nav().faceToward(Vec3f(0, 0, 0));

        // Create error mesh (sphere)
        addSphere(errorMesh, 0.5);

        // Load the OBJ file
        std::string fileName = "../Moon2K.obj";
        ascene = Scene::import(fileName);
        if (!ascene) {
            std::cerr << "Error loading OBJ: " << fileName << std::endl;
            return;
        }

        std::cout << "Successfully loaded OBJ: " << fileName << std::endl;
        std::cout << "Number of meshes: " << ascene->meshes() << std::endl;

        // Extract meshes from scene
        meshes.resize(ascene->meshes());
        for (int i = 0; i < ascene->meshes(); ++i) {
            ascene->mesh(i, meshes[i]);
            std::cout << "Mesh " << i << " vertices: " << meshes[i].vertices().size() << std::endl;
        }

        if (!meshes.empty()) {
            // Calculate bounds and center
            Vec3f min, max;
            meshes[0].getBounds(min, max);
            Vec3f center = (min + max) * 0.5f;
            
            // Calculate appropriate scale
            float maxDim = std::max({max.x - min.x, max.y - min.y, max.z - min.z});
            if (maxDim > 0) {
                scale = 2.0f / maxDim;  // Scale to fit in a 2x2x2 box
            }

            // Center the mesh
            meshes[0].translate(-center);
            modelLoaded = true;
        }
    }

    void onAnimate(double dt) override {
        rotationAngle += dt * 0.5;  // Rotate 0.5 radians per second
    }

    void onDraw(Graphics& g) override {
        g.clear(0.1);
        g.lighting(true);
        g.depthTesting(true);

        if (modelLoaded) {
            g.pushMatrix();
            g.rotate(rotationAngle, 0, 1, 0);  // Rotate around Y axis
            g.scale(scale);
            g.color(0.0, 1.0, 0.0);  // Neon green color
            g.draw(meshes[0]);
            g.popMatrix();
        } else {
            // Draw a placeholder sphere if model failed to load
            g.pushMatrix();
            g.rotate(rotationAngle, 0, 1, 0);
            g.color(1.0, 0.0, 0.0);  // Red color for error state
            g.draw(errorMesh);
            g.popMatrix();
        }
    }

    bool onKeyDown(const Keyboard& k) override {
        if (k.key() == ' ') {  // Space bar to reset rotation
            rotationAngle = 0;
        }
        return true;  // Return true to indicate the key was handled
    }
};

int main() {
    MyApp app;
    app.configureAudio(0, 0);
    app.dimensions(1024, 768);
    app.title("Moon Viewer");
    app.start();
    return 0;
}