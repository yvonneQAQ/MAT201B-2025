#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/math/al_Random.hpp"
#include <vector>
#include <map>
#include <limits>
#include <cmath>

using namespace al;
using namespace std;

const int NUM_POINTS = 40;
const int NUM_RELAXATIONS = 5;
const int GRID_RES = 800;  // resolution of the approximation grid
const float DOMAIN_SIZE = 4.0f;  // [-1,1] space
const float DRIFT_SPEED = 0.02f;  // Speed of drift animation

struct VoronoiApp : App {
  vector<Vec2f> points;
  vector<Vec2f> velocities;  // Added for drift animation
  vector<Color> colors;
  vector<vector<Vec2f>> cells;
  bool isDrifting = false;
  
  void onCreate() override {
    // Generate random points
    for (int i = 0; i < NUM_POINTS; ++i) {
      points.push_back(Vec2f(rnd::uniform(-1.0f, 1.0f), rnd::uniform(-1.0f, 1.0f)));
      colors.push_back(HSV(rnd::uniform() / 3.0, 0.2, (rnd::uniform() + 1.0) / 2));
      
      // Initialize velocities based on direction from center
      Vec2f toCenter = -points[i];  // Vector pointing from point to center
      toCenter.normalize();
      velocities.push_back(-toCenter * DRIFT_SPEED);  // Drift away from center
    }

    for (int i = 0; i < NUM_RELAXATIONS; ++i) {
      relaxPoints();
    }

    computeVoronoiCells();
  }

  void onAnimate(double dt) override {
    if (isDrifting) {
      // Update point positions based on velocities
      for (int i = 0; i < points.size(); ++i) {
        points[i] += velocities[i];
        
        // Optional: Slightly increase velocity over time for acceleration effect
        velocities[i] *= 1.001f;
      }
      
      // Recompute Voronoi cells with new point positions
      computeVoronoiCells();
    }
  }

  void relaxPoints() {
    vector<Vec2f> newPoints(NUM_POINTS);
    vector<int> counts(NUM_POINTS, 0);

    for (int x = 0; x < GRID_RES; ++x) {
      for (int y = 0; y < GRID_RES; ++y) {
        float fx = (float)x / (GRID_RES - 1) * DOMAIN_SIZE - 1.0f;
        float fy = (float)y / (GRID_RES - 1) * DOMAIN_SIZE - 1.0f;
        Vec2f pos(fx, fy);

        int nearestIdx = 0;
        float minDist = std::numeric_limits<float>::max();
        for (int i = 0; i < points.size(); ++i) {
          float d = (pos - points[i]).magSqr();
          if (d < minDist) {
            minDist = d;
            nearestIdx = i;
          }
        }

        newPoints[nearestIdx] += pos;
        counts[nearestIdx]++;
      }
    }

    for (int i = 0; i < NUM_POINTS; ++i) {
      if (counts[i] > 0)
        points[i] = newPoints[i] / float(counts[i]);
    }
  }

  void computeVoronoiCells() {
    cells.clear();
    cells.resize(NUM_POINTS);

    for (int x = 0; x < GRID_RES; ++x) {
      for (int y = 0; y < GRID_RES; ++y) {
        float fx = (float)x / (GRID_RES - 1) * DOMAIN_SIZE - 1.0f;
        float fy = (float)y / (GRID_RES - 1) * DOMAIN_SIZE - 1.0f;
        Vec2f pos(fx, fy);

        int nearestIdx = 0;
        float minDist = std::numeric_limits<float>::max();
        for (int i = 0; i < points.size(); ++i) {
          float d = (pos - points[i]).magSqr();
          if (d < minDist) {
            minDist = d;
            nearestIdx = i;
          }
        }
        cells[nearestIdx].push_back(pos);
      }
    }
  }

  void onDraw(Graphics& g) override {
    g.clear(1);

    for (int i = 0; i < NUM_POINTS; ++i) {
      Mesh mesh(Mesh::TRIANGLE_FAN);
      for (auto& p : cells[i]) {
        mesh.vertex(p);
      }
      g.color(colors[i]);
      g.draw(mesh);
    }
  }

  bool onKeyDown(Keyboard const& k) override {
    if (k.key() == ' ') {
      isDrifting = !isDrifting;  // Toggle drift animation
      return true;
    }
    return false;
  }
};

int main() {
  VoronoiApp app;
  app.configureAudio(44100, 512, 2, 0);
  app.dimensions(1024, 768);
  app.title("Drifting Voronoi");
  app.fps(60);
  app.displayMode(Window::DEFAULT_BUF);
  app.start();
}
