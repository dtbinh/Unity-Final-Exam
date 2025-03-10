
#include <FL/Fl_Gl_Window.h>
#include <Fl/Fl.h>
#include <Fl/Fl_Value_Slider.H>
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <Fl/Fl_Double_Window.h>
#include <list>

#include "stdio.h"
#include "math.h"
#include "3DUtils.h"

#include "Vec3f.h"
#include "Viewer.h"
#include <deque>


#include "BVH.h"
#include "RunButton.H"

struct Particle {
    Vec3f pos;  //position
    Vec3f velocity; //current velocity
    Vec3f color; //current color
    float timeAlive; //The amount of time that this particle has been alive.
    float lifespan;  //The total amount of time that this particle is to live.
};

class MyGlWindow : public Fl_Gl_Window {
    public:
        MyGlWindow(int x, int y, int w, int h);
        

        std::vector<Vec3f *> history;
        

        BVH * m_bvh;
        RunSlider * m_slider;
        
        Fl_Slider* time;	
        int run;
        int loadply();
        void drawStuff();
        
    private:
        std::deque<Particle> m_particleList;
        float                m_direction;
        float                m_range;

        void draw();					// standard FlTk
        
        
        int handle(int);				// standard FlTk
        
        float fieldOfView;
        Viewer *m_viewer;
        void setProjection(int clearProjection = 1);
        void getMouseNDC(float& x, float& y);
        void setupLight();
};

