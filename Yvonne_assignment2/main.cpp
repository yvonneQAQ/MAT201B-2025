#include "al/app/al_App.hpp"
#include "al/graphics/al_Image.hpp"
#include "al/math/al_Random.hpp"
#include <fstream>
#include <string>

using namespace al;

struct PixelPoint {
    Vec3f currentPos;
    Vec3f targetPos;
    Color color;
}; //current pixel position, target pixel position, their color

struct MyApp : App {
    std::vector<PixelPoint> pixels;
    Mesh mesh{Mesh::POINTS};
    Image img;
    int mode = 1; 

    void onCreate() override {
        nav().pos(0, 0, 3);

        if (!img.load("../colorful.png")) {
            std::cerr << "Failed to load image\n";
            exit(1);
        }

        int w = img.width();
        int h = img.height();//fetching the width and height of the image

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) { //getting all the (x,y) of the pixels
                auto pixel = img.at(x, h - 1 - y); // manually flip Y so it is in correct orientation
                Color c(pixel.r / 255.0, pixel.g / 255.0, pixel.b / 255.0); //change max 255 to max 1.(range)

                Vec3f pos = Vec3f((float)x / w - 0.5, (float)y / h - 0.5, 0); //size the picture to fit the screen 

                PixelPoint p;
                p.currentPos = pos;
                p.targetPos = pos;
                p.color = c;

                pixels.push_back(p);
                mesh.vertex(pos);
                mesh.color(c);
            }
        }
    }

    void onAnimate(double dt) override {
        for (int i = 0; i < pixels.size(); i++) { //go over all the pixels 
            
            pixels[i].currentPos = lerp(pixels[i].currentPos, pixels[i].targetPos, 0.07f); //lerp here
            mesh.vertices()[i] = pixels[i].currentPos;
        }
    }

    void onDraw(Graphics& g) override {
        g.clear(0);
        g.pointSize(2.0);
        g.meshColor();
        g.draw(mesh);
    }

    bool onKeyDown(const Keyboard& k) override {
        if (k.key() == '1') {
            mode = 1;
            setOriginalLayout();
        } else if (k.key() == '2') {
            mode = 2;
            setRGBCube();
        } else if(k.key() == '3'){
            mode = 3;
            setHSVCylinder();  
        }
        else if (k.key() == '4') {
            mode = 4;
            setColorSphere();
        }
        return true;
    }

    void setOriginalLayout() {
        int w = img.width();
        int h = img.height();
        int index = 0;
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                Vec3f pos = Vec3f((float)x / w - 0.5, (float)y / h - 0.5, 0);
                pixels[index++].targetPos = pos;
            }
        }
    }

    void setRGBCube() {
        for (auto& p : pixels) {
            p.targetPos = Vec3f(p.color.r - 0.5, p.color.g - 0.5, p.color.b - 0.5);
        }
    }

    HSV rgbToHsv(Color c) {
        float r = c.r;
        float g = c.g;
        float b = c.b;
    
        float max = std::max({r, g, b});
        float min = std::min({r, g, b});
        float delta = max - min;
    
        HSV out;
        out.v = max;
    
        if (delta < 0.00001f) {
            out.s = 0.0f;
            out.h = 0.0f;
            return out;
        }
    
        out.s = (max > 0.0f) ? delta / max : 0.0f;
    
        if (r >= max)
            out.h = (g - b) / delta;
        else if (g >= max)
            out.h = 2.0f + (b - r) / delta;
        else
            out.h = 4.0f + (r - g) / delta;
    
        out.h /= 6.0f;
        if (out.h < 0.0f)
            out.h += 1.0f;
    
        return out;
    }
    

    void setHSVCylinder() {
        for (auto& p : pixels) {
            HSV hsv = rgbToHsv(p.color);
            float angle = hsv.h * 2 * M_PI; // hue as angle in radians
            float radius = hsv.s;           // saturation as radius
            float y = (1 - hsv.v) - 0.5;          // value as height

            float x = cos(angle) * radius;
            float z = sin(angle) * radius;

            p.targetPos = Vec3f(x, y, z);
        }
    }

    void setColorSphere() {
        int w = img.width();
        int h = img.height();
        int index = 0;
    
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                auto& p = pixels[index++];
    
                float brightness = (p.color.r + p.color.g + p.color.b) / 3.0f;
    
                // 用 x 和 y 映射到球面方向
                float theta = float(x) / w * 2.0f * M_PI;       // 横向角度
                float phi   = float(y) / h * M_PI;              // 纵向角度
    
                float r = brightness; // radius = brightness
    
                float sx = r * sin(phi) * cos(theta);
                float sy = r * sin(phi) * sin(theta);
                float sz = r * cos(phi);
    
                p.targetPos = Vec3f(sx, sy, sz);
            }
        }
    } 
    
};

int main() {
    MyApp app;
    app.start();
}