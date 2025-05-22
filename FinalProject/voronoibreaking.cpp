#include "al/app/al_App.hpp"
#include "al/math/al_Random.hpp"
#include <vector>
#include <float.h>
#include <algorithm>
using namespace al;

struct Points : App {
    Mesh voronoiShapes;
    Mesh seedDots;
    std::vector<Vec3f> seeds;
    std::vector<Color> colors;
    
    void onCreate() override {
        nav().pos(0, 0, 5);
        
        rnd::Random<> rng;
        
        // Create seeds in a circular pattern
        // First seed at center
        seeds.push_back(Vec3f(0, 0, 0));
        colors.push_back(Color(rng.uniform(), rng.uniform(), rng.uniform()));
        
        // Create ring of seeds around center
        int numSeeds = 8;  // Number of seeds in the outer ring
        float radius = 0.5;  // Radius of the ring
        for (int i = 0; i < numSeeds; i++) {
            float angle = (2 * M_PI * i) / numSeeds;
            float x = radius * cos(angle);
            float y = radius * sin(angle);
            seeds.push_back(Vec3f(x, y, 0));
            colors.push_back(Color(rng.uniform(), rng.uniform(), rng.uniform()));
        }

        // Create the shapes
        createVoronoiShapes();
    }

    void createVoronoiShapes() {
        voronoiShapes.primitive(Mesh::TRIANGLES);
        seedDots.primitive(Mesh::POINTS);

        // Number of sample points around the circle
        const int numSamplePoints = 360;
        const float outerRadius = 1.0f;

        // For each seed
        for (int seedIdx = 0; seedIdx < seeds.size(); seedIdx++) {
            Vec3f seedPos = seeds[seedIdx];
            std::vector<Vec3f> cellPoints;

            // Sample points around the circle
            for (int i = 0; i < numSamplePoints; i++) {
                float angle = (2 * M_PI * i) / numSamplePoints;
                float x = outerRadius * cos(angle);
                float y = outerRadius * sin(angle);
                Vec3f samplePoint(x, y, 0);

                // Find closest seed to this sample point
                float minDist = FLT_MAX;
                int closestSeed = 0;
                for (int s = 0; s < seeds.size(); s++) {
                    float dist = (samplePoint - seeds[s]).mag();
                    if (dist < minDist) {
                        minDist = dist;
                        closestSeed = s;
                    }
                }

                // If this sample point belongs to current seed, add it
                if (closestSeed == seedIdx) {
                    cellPoints.push_back(samplePoint);
                }
            }

            // If we found points for this cell
            if (cellPoints.size() > 0) {
                // Create triangles fan from seed to boundary points
                Color cellColor = colors[seedIdx];
                cellColor.a = 0.5;

                for (int i = 0; i < cellPoints.size(); i++) {
                    int next = (i + 1) % cellPoints.size();
                    
                    voronoiShapes.vertex(seedPos);
                    voronoiShapes.vertex(cellPoints[i]);
                    voronoiShapes.vertex(cellPoints[next]);
                    
                    voronoiShapes.color(cellColor);
                    voronoiShapes.color(cellColor);
                    voronoiShapes.color(cellColor);
                }
            }

            // Add seed point
            seedDots.vertex(seedPos);
            seedDots.color(colors[seedIdx]);
        }
    }

    void onDraw(Graphics& g) override {
        g.clear(0);
        g.camera(nav());
        g.depthTesting(true);
        g.blending(true);
        g.blendAdd();
        
        // Draw voronoi shapes
        g.draw(voronoiShapes);
        
        // Draw seed points
        g.pointSize(10);
        g.draw(seedDots);
    }
};

int main() {
    Points app;
    app.configureAudio(44100, 512, 2, 0);
    app.dimensions(1024, 768);
    app.title("Voronoi Cells");
    app.fps(60);
    app.displayMode(Window::DEFAULT_BUF);
    app.start();
}
