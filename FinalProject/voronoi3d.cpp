#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/math/al_Random.hpp"
#include <vector>
#include <cmath>

using namespace al;
using namespace std;

struct Fragment {
  std::vector<Vec3f> vertices;
  Vec3f velocity;
  Vec3f center;  // Add center point for 3D rotation

  void update(float dt) {
    // Update position
    center += velocity * dt;
    
    // Apply the same translation to all vertices
    for (auto& v : vertices) {
      v += velocity * dt;
    }
  }

  void draw(Graphics& g) {
    Mesh m;
    m.primitive(Mesh::TRIANGLE_FAN);
    
    // Add center point first for triangle fan
    m.vertex(center);
    m.color(0.7, 0.9, 1.0);
    
    // Add perimeter vertices
    for (auto& v : vertices) {
      m.vertex(v);
      m.color(0.5, 0.8, 1.0);
    }
    // Close the fan by repeating first vertex
    if (!vertices.empty()) {
      m.vertex(vertices[0]);
      m.color(0.5, 0.8, 1.0);
    }
    
    g.draw(m);
  }
};

struct VoronoiSim : App {
  std::vector<Fragment> fragments;
  float noiseAmount = 0.1f;  // Height variation for 3D effect

  void onCreate() override {
    nav().pos(0, 0, 5);
    nav().setHome();

    const int numSeeds = 20;
    // Generate seed points in 3D space
    std::vector<Vec3f> seeds;
    for (int i = 0; i < numSeeds; ++i) {
      seeds.push_back(Vec3f(
        rnd::uniformS() * 1.5f,  // x
        rnd::uniformS() * 1.5f,  // y
        rnd::uniformS() * noiseAmount  // z - slight variation for initial height
      ));
    }

    // Create 3D fragments
    for (int i = 0; i < numSeeds; ++i) {
      Fragment frag;
      float angleOffset = rnd::uniform() * M_2PI;
      
      // Store the center point
      frag.center = seeds[i];
      
      // Create vertices around the center point
      const int numVertices = 7;
      for (int j = 0; j < numVertices; ++j) {
        float angle = angleOffset + j * (M_2PI / float(numVertices));
        float radius = 0.2f + rnd::uniform() * 0.05f;
        
        // Create point with height variation
        Vec3f pt = seeds[i] + Vec3f(
          cos(angle) * radius,
          sin(angle) * radius,
          rnd::uniformS() * noiseAmount  // Individual vertex height variation
        );
        frag.vertices.push_back(pt);
      }

      // Set 3D velocity with vertical component
      frag.velocity = Vec3f(
        rnd::uniformS() * 0.1f,   // x velocity
        rnd::uniformS() * 0.1f,   // y velocity
        rnd::uniformS() * 0.02f   // z velocity - slower vertical movement
      );
      
      fragments.push_back(frag);
    }
  }

  void onAnimate(double dt) override {
    for (auto& f : fragments) f.update(dt);
  }

  void onDraw(Graphics& g) override {
    g.clear(0.1);  // Darker background for better 3D effect
    g.camera(nav());
    
    // Enable lighting for 3D effect
    g.lighting(true);
    
    // Set up a basic light
    Light light;
    light.pos(5, 5, 5);              // Light position
    light.diffuse(RGB(0.4));         // Diffuse light color
    light.ambient(RGB(0.4));         // Ambient light color
    g.light(light);
    
    // Draw fragments with depth testing and transparency
    g.depthTesting(true);
    g.blending(true);
    g.blendAdd();  // Use additive blending for a nice ice effect
    for (auto& f : fragments) f.draw(g);
  }
};

int main() {
  VoronoiSim app;
  app.configureAudio(44100, 512, 2, 0);
  app.dimensions(1024, 768);
  app.title("3D Iceberg Break Simulation");
  app.fps(60);
  app.displayMode(Window::DEFAULT_BUF);
  app.start();
  return 0;
}
