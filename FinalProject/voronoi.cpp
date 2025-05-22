#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/math/al_Random.hpp"
#include <vector>
#include <cmath>

using namespace al;
using namespace std;

struct Fragment {
  std::vector<Vec2f> vertices;
  Vec2f velocity;

  void update(float dt) {
    for (auto& v : vertices) v += velocity * dt;
  }

  void draw(Graphics& g) {
    Mesh m;
    m.primitive(Mesh::LINE_LOOP);
    for (auto& v : vertices) m.vertex(v.x, v.y, 0);
    g.draw(m);
  }
};

struct VoronoiSim : App {
  std::vector<Fragment> fragments;

  void onCreate() override {
    nav().pos(0, 0, 5);

    // Step 1: generate seed points
    const int numSeeds = 20;
    std::vector<Vec2f> seeds;
    for (int i = 0; i < numSeeds; ++i) {
      seeds.push_back(Vec2f(rnd::uniformS() * 1.5f, rnd::uniformS() * 1.5f));
    }

    // Step 2: manually create Voronoi-like cell boundaries
    for (int i = 0; i < numSeeds; ++i) {
      Fragment frag;
      float angleOffset = rnd::uniform() * M_2PI;
      for (int j = 0; j < 7; ++j) {
        float angle = angleOffset + j * (M_2PI / 7.0f);
        float radius = 0.2f + rnd::uniform() * 0.05f;
        Vec2f pt = seeds[i] + Vec2f(cos(angle), sin(angle)) * radius;
        frag.vertices.push_back(pt);
      }
      frag.velocity = Vec2f(rnd::uniformS() * 0.1f, rnd::uniformS() * 0.1f);
      fragments.push_back(frag);
    }
  }

  void onAnimate(double dt) override {
    for (auto& f : fragments) f.update(dt);
  }

  void onDraw(Graphics& g) override {
    g.clear(0.9);
    g.color(0.5, 0.8, 1.0);
    for (auto& f : fragments) f.draw(g);
  }
};

int main() {
  VoronoiSim app;
  app.configureAudio(44100, 512, 2, 0);
  app.dimensions(1024, 768);
  app.title("Iceberg Break Sim");
  app.fps(60);
  app.displayMode(Window::DEFAULT_BUF);
  app.start();
  return 0;
}
