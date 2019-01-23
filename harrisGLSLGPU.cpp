//---------------------------------------------------------------------------
//                               www.GPGPU.org 
//                                Sample Code
//---------------------------------------------------------------------------
// Copyright (c) 2004 Mark J. Harris and GPGPU.org
// Copyright (c) 2004 3Dlabs Inc. Ltd.
//---------------------------------------------------------------------------
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any
// damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any
// purpose, including commercial applications, and to alter it and
// redistribute it freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you
//    must not claim that you wrote the original software. If you use
//    this software in a product, an acknowledgment in the product
//    documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and
//    must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.
//
//---------------------------------------------------------------------------
// Author: Mark Harris (harrism@gpgpu.org) - original helloGPGPU
// Author: Mike Weiblen (mike.weiblen@3dlabs.com) - GLSL version
//---------------------------------------------------------------------------
// GPGPU Lesson 0: "helloGPGPU_GLSL" (a GLSL version of "helloGPGPU")
//---------------------------------------------------------------------------
//
// GPGPU CONCEPTS Introduced: 
//
//      1.) Texture = Array
//      2.) Fragment Program = Computational Kernel.
//      3.) One-to-one Pixel to Texel Mapping: 
//          a) Data-Dimensioned Viewport, and
//          b) Orthographic Projection.
//      4.) Viewport-Sized Quad = Data Stream Generator.
//      5.) Copy To Texture = feedback.
//
//      For details of each of these concepts, see the explanations in the 
//      inline "GPGPU CONCEPT" comments in the code below.
//
// APPLICATION Demonstrated: A simple post-process edge detection filter. 
//
//---------------------------------------------------------------------------
// Notes regarding this "helloGPGPU_GLSL" source code:
//
// This example was derived from the original "helloGPGPU.cpp" v1.0.1
// by Mark J. Harris.  It demonstrates the same simple post-process edge
// detection filter, but instead implemented using the OpenGL Shading Language
// (also known as "GLSL"), an extension of the OpenGL v1.5 specification.
// Because the GLSL compiler is an integral part of a vendor's OpenGL driver,
// no additional GLSL development tools are required.
// This example was developed/tested on 3Dlabs Wildcat Realizm.
//
// I intentionally minimized changes to the structure of the original code
// to support a side-by-side comparison of the implementations.
//
// Thanks to Mark for making the original helloGPGPU example available!
//
// -- Mike Weiblen, May 2004
//
// 
// [MJH:]
// This example has also been tested on NVIDIA GeForce FX and GeForce 6800 GPUs.
//
// Note that the example requires glew.h and glew32s.lib, available at 
// http://glew.sourceforge.net.
//
// Thanks to Mike for modifying the example.  I have actually changed my 
// original code to match; for example the inline Cg code instead of an 
// external file.
//
// -- Mark Harris, June 2004
//
// References:
//    http://gpgpu.sourceforge.net/
//    http://glew.sourceforge.net/
//    http://www.xmission.com/~nate/glut.html
//---------------------------------------------------------------------------

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#define GLEW_STATIC 1
#include "GL/glew.h"
#include "GL/glut.h"
#include "gdal_priv.h"
#include <memory>
#include <vector>
#include "GLSLOperation.h"

using namespace std;
#pragma  comment(lib, "gdal_i.lib")
// forward declarations
class HelloGPGPU;
void reshape(int w, int h);

// globals
HelloGPGPU  *g_pHello;
CGLSLOperation* pOper;


// This shader performs a 9-tap Laplacian edge detection filter.
// (converted from the separate "edges.cg" file to embedded GLSL string)
static const char *edgeFragSource = {
"uniform sampler2D texUnit;"
"void main(void)"
"{"
"   const float offset = 1.0 / 512.0;"
"   vec2 texCoord = gl_TexCoord[0].xy;"
"   vec4 c  = texture2D(texUnit, texCoord);"
"   vec4 bl = texture2D(texUnit, texCoord + vec2(-offset, -offset));"
"   vec4 l  = texture2D(texUnit, texCoord + vec2(-offset,     0.0));"
"   vec4 tl = texture2D(texUnit, texCoord + vec2(-offset,  offset));"
"   vec4 t  = texture2D(texUnit, texCoord + vec2(    0.0,  offset));"
"   vec4 ur = texture2D(texUnit, texCoord + vec2( offset,  offset));"
"   vec4 r  = texture2D(texUnit, texCoord + vec2( offset,     0.0));"
"   vec4 br = texture2D(texUnit, texCoord + vec2( offset,  -offset));"
"   vec4 b  = texture2D(texUnit, texCoord + vec2(    0.0, -offset));"
"   gl_FragColor.rgb = vec3((c.r+c.g+c.b)/3, (c.r+c.g+c.b)/3, (c.r+c.g+c.b)/3);"
"}"
};
static const char* grayFragSource = {
	"uniform sampler2D texUnit;"
	"const vec3 W = vec3(0.2125,0.7154,0.0721);"
	"void main(void)"
	"{"
	"	vec2 texCoord = gl_TexCoord[0].xy;"
	"   vec4 c  = texture2D(texUnit, texCoord);"
	"	float gray = dot(c.rgb, W);"
	"	gl_FragColor = vec4(gray, gray, gray, 1.0);"
	"}"	
};
static const char* testSource = {
	"uniform sampler2DRect texUnit;"
	"void main(void)"
	"{"
	"	vec4 ccc = texture2DRect(texUnit, gl_TexCoord[0].xy);\n"
	"	vec4 clc = texture2DRect(texUnit, gl_TexCoord[1].xy);\n"
	"	vec4 crc = texture2DRect(texUnit, gl_TexCoord[2].xy);\n"
	"	vec4 ccd = texture2DRect(texUnit, gl_TexCoord[3].xy);\n"
	"	vec4 ccu = texture2DRect(texUnit, gl_TexCoord[4].xy);\n"
	"	gl_FragColor = abs(clc-crc) +abs(ccd-ccu);"
	"}"	
};
//	"	vec4 cld = texture2D(texUnit, gl_TexCoord[5].xy);\n"
//	"	vec4 clu = texture2D(texUnit, gl_TexCoord[6].xy);\n"
//	"	vec4 crd = texture2D(texUnit, gl_TexCoord[7].xy);\n"
//	"	vec2 TexRU = vec2(gl_TexCoord[2].x, gl_TexCoord[4].y); \n"
//	"	vec4 cru = texture2D(texUnit, TexRU.xy);\n"
//	
//"   gl_FragColor = 8.0 * (c + -0.125 * (bl + l + tl + t + ur + r + br + b));"
static const char* harrisFragSource = {
	"uniform sampler2D texUnit;"
	"const vec3 W = vec3(0.2125,0.7154,0.0721);"
	"void main(void)"
	"{"
	"	const float offset = 1.0 / 512;"
	"	vec2 texCoord = gl_TexCoord[0].xy;"
	"   vec4 c  = texture2D(texUnit, texCoord);"
	"   vec4 bl = texture2D(texUnit, texCoord + vec2(-offset, -offset));"
	"   vec4 l  = texture2D(texUnit, texCoord + vec2(-offset,     0.0));"
	"   vec4 tl = texture2D(texUnit, texCoord + vec2(-offset,  offset));"
	"   vec4 t  = texture2D(texUnit, texCoord + vec2(    0.0,  offset));"
	"   vec4 tr = texture2D(texUnit, texCoord + vec2( offset,  offset));"
	"   vec4 r  = texture2D(texUnit, texCoord + vec2( offset,     0.0));"
	"   vec4 br = texture2D(texUnit, texCoord + vec2( offset,  -offset));"
	"   vec4 b  = texture2D(texUnit, texCoord + vec2(    0.0, -offset));"
	"	float fc = dot(c.rgb,W);"
	"	float fbl = dot(bl.rgb,W);"
	"	float fl = dot(l.rgb, W);"
	"	float ftl = dot(tl.rgb, W);"
	"	float ft = dot(t.rgb, W);"
	"	float ftr = dot(tr.rgb, W);"
	"	float fr = dot(r.rgb, W);"
	"	float fbr = dot(br.rgb, W);"
	"	float fb = dot(b.rgb, W);"
	"	float dx = ((fr+fbr+ftr) - (fl+fbl+ftl))/9;"
	"	float dy = ((fbl+fb+fbr) - (ftl+ft+ftr))/9;"
	"	float xx = dx * dx;"
	"	float yy = dy * dy;"
	"	float xy = dx * dy;"
	"	float vv = xx * yy - xy * xy;"
	"	gl_FragColor.rgb = vec3(xx+yy-vv, xx+yy-vv, xx+yy-vv);"
	"	gl_FragColor.a = 1.0;"
	"}"
};
/* 
"	const float offset = 1.0 / 512.0;"
n
"	float xn = (vxn.r + vxn.g + vxn.b) / 3.0;"
	"	float xp = (vxp.r + vxp.g + vxp.b) / 3.0;"
	"	float yn = (vyn.r + vyn.g + vyn.b) / 3.0;"
	"	float yp = (vyn.r + vyn.g + vyn.b) / 3.0;"
*/			

const char* harrisFragS = {
	"uniform sampler2D texUnit;"
	"uniform vec2 texSize;"
	"const vec3 W = vec3(0.2125,0.7154,0.0721);"
	"void main(void)"
	"{"
	"	float gray[9]; "
	"	vec2 offset = 1.0 / texSize;"
	"	gray[0] = dot(texture2D(texUnit, gl_TexCoord[0].st + vec2(-offset.x, -offset.y)).rgb, W);"
	"	gray[1] = dot(texture2D(texUnit, gl_TexCoord[0].st + vec2(-offset.x, 0)).rgb, W);"
	"	gray[2] = dot(texture2D(texUnit, gl_TexCoord[0].st + vec2(-offset.x, offset.y)).rgb, W);"
	"	gray[3] = dot(texture2D(texUnit, gl_TexCoord[0].st + vec2(0, -offset.y)).rgb, W);"
	"	gray[4] = dot(texture2D(texUnit, gl_TexCoord[0].st + vec2(0, 0)).rgb, W);"
	"	gray[5] = dot(texture2D(texUnit, gl_TexCoord[0].st + vec2(0, offset.y)).rgb, W);"
	"	gray[6] = dot(texture2D(texUnit, gl_TexCoord[0].st + vec2(offset.x, -offset.y)).rgb, W);"
	"	gray[7] = dot(texture2D(texUnit, gl_TexCoord[0].st + vec2(offset.x, 0)).rgb, W);"
	"	gray[8] = dot(texture2D(texUnit, gl_TexCoord[0].st + vec2(offset.x, offset.y)).rgb, W);"
	"	float dx = ((gray[6]+gray[7]+gray[8]) - (gray[0]+gray[1]+gray[2]))/9.;"
	"	float dy = ((gray[2]+gray[5]+gray[8]) - (gray[0]+gray[3]+gray[6]))/9.;"
	"	float xx = dx * dx;"
	"	float yy = dy * dy;"
	"	float xy = dx * dy;"
	"	float vv = xx * yy - xy * xy;"
	"	gl_FragColor = vec4(xx+yy-vv, xx+yy-vv, xx+yy-vv, 1.0);"
	"}"	
};



// This class encapsulates all of the GPGPU functionality of the example.
class HelloGPGPU
{
public: // methods
    HelloGPGPU(int w, int h, int b, unsigned char* data) 
    : _iWidth(w),
      _iHeight(h),
	  _iBand(b),
	  _data(NULL)
    {
        // Create a simple 2D texture.  This example does not use
        // render to texture -- it just copies from the framebuffer to the
        // texture.

		_data = new float[_iWidth* _iHeight* _iBand];
		float* ptr = (float*)_data;
		for(int i = 0; i <_iWidth* _iHeight* _iBand; i++)
			ptr[i] = data[i] / 255.;



        // GPGPU CONCEPT 1: Texture = Array.
        // Textures are the GPGPU equivalent of arrays in standard 
        // computation. Here we allocate a texture large enough to fit our
        // data (which is arbitrary in this example).
        glGenTextures(1, &_iTexture);
        glBindTexture(GL_TEXTURE_2D, _iTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _iWidth, _iHeight, 
                     0, GL_RGB, GL_FLOAT, _data);

       
        // GPGPU CONCEPT 2: Fragment Program = Computational Kernel.
        // A fragment program can be thought of as a small computational 
        // kernel that is applied in parallel to many fragments 
        // simultaneously.  Here we load a kernel that performs an edge 
        // detection filter on an image.

        _programObject = glCreateProgramObjectARB();

        // Create the edge detection fragment program
        _fragmentShader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
        glShaderSourceARB(_fragmentShader, 1, &harrisFragSource, NULL);
        glCompileShaderARB(_fragmentShader);
        glAttachObjectARB(_programObject, _fragmentShader);

        // Link the shader into a complete GLSL program.
        glLinkProgramARB(_programObject);
        GLint progLinkSuccess;
        glGetObjectParameterivARB(_programObject, GL_OBJECT_LINK_STATUS_ARB,
                &progLinkSuccess);
        if (!progLinkSuccess)
        {
            fprintf(stderr, "Filter shader could not be linked\n");
			exit(1);
        }
		// Get location of the sampler uniform
        _texUnit = glGetUniformLocationARB(_programObject, "texUnit");
    }

	~HelloGPGPU()
    {
		if(_data) delete[] _data;
    }

    // This method updates the texture by rendering the geometry (a teapot 
    // and 3 rotating tori) and copying the image to a texture.  
    // It then renders a second pass using the texture as input to an edge 
    // detection filter.  It copies the results of the filter to the texture.
    // The texture is used in HelloGPGPU::display() for displaying the 
    // results.
    void update()
    {       
        // store the window viewport dimensions so we can reset them,
        // and set the viewport to the dimensions of our texture
        int vp[4];
        glGetIntegerv(GL_VIEWPORT, vp);

        // GPGPU CONCEPT 3a: One-to-one Pixel to Texel Mapping: A Data-
        //                   Dimensioned Viewport.
        // We need a one-to-one mapping of pixels to texels in order to 
        // ensure every element of our texture is processed. By setting our 
        // viewport to the dimensions of our destination texture and drawing 
        // a screen-sized quad (see below), we ensure that every pixel of our 
        // texel is generated and processed in the fragment program.
        glViewport(0, 0, _iWidth, _iHeight);
        
        // Render a teapot and 3 tori
        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_MODELVIEW);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _iWidth, _iHeight, 0, GL_RGB, GL_FLOAT, _data);
        glBegin(GL_QUADS);
        {            
            glTexCoord2f(0, 1); glVertex2f(-1, -1);
            glTexCoord2f(1, 1); glVertex2f( 1, -1);
            glTexCoord2f(1, 0); glVertex2f( 1,  1);
            glTexCoord2f(0, 0); glVertex2f(-1,  1);
        }
        glEnd();

		//return;
        // run the edge detection filter over the geometry texture
        // Activate the edge detection filter program
        glUseProgramObjectARB(_programObject);
            
        // identify the bound texture unit as input to the filter
        glUniform1iARB(_texUnit, 0);
            
        // GPGPU CONCEPT 4: Viewport-Sized Quad = Data Stream Generator.
        // In order to execute fragment programs, we need to generate pixels.
        // Drawing a quad the size of our viewport (see above) generates a 
        // fragment for every pixel of our destination texture. Each fragment
        // is processed identically by the fragment program. Notice that in 
        // the reshape() function, below, we have set the frustum to 
        // orthographic, and the frustum dimensions to [-1,1].  Thus, our 
        // viewport-sized quad vertices are at [-1,-1], [1,-1], [1,1], and 
        // [-1,1]: the corners of the viewport.
        glBegin(GL_QUADS);
        {            
            glTexCoord2f(0, 0); glVertex2f(-1, -1);
            glTexCoord2f(1, 0); glVertex2f( 1, -1);
            glTexCoord2f(1, 1); glVertex2f( 1,  1);
            glTexCoord2f(0, 1); glVertex2f(-1,  1);
        }
        glEnd();
         
        // disable the filter
        glUseProgramObjectARB(0);
        
        // GPGPU CONCEPT 5: Copy To Texture (CTT) = Feedback.
        // We have just invoked our computation (edge detection) by applying 
        // a fragment program to a viewport-sized quad. The results are now 
        // in the frame buffer. To store them, we copy the data from the 
        // frame buffer to a texture.  This can then be fed back as input
        // for display (in this case) or more computation (see 
        // more advanced samples.)

        // update the texture again, this time with the filtered scene
        glBindTexture(GL_TEXTURE_2D, _iTexture);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, _iWidth, _iHeight);
        
		//return;
		float* p = new float[_iWidth*_iHeight*_iBand];
		memset(p, 0, _iWidth*_iHeight*_iBand*sizeof(float));

		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,
			GL_TEXTURE_RECTANGLE_ARB,_iTexture,0);

		glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
		glRasterPos2i(0, 0);
		glReadPixels(0, 0, _iWidth, _iHeight, GL_RGB, GL_FLOAT, p);

		int blk = 21;
		float* pij = NULL;
		float tmp;
		int ii, jj, ie, je;
		bool flag = false;
		vector<HarrisPt> pt;
		for(int i = 0; i < (_iHeight+blk-1) / blk; i++){
			for(int j = 0; j < (_iWidth+blk-1)/blk; j++){
				pij = p + (blk*i*_iWidth + blk*j)*_iBand;
				tmp = 0;	
				flag = false;
				ie = (i == _iHeight / blk) ? _iHeight % blk : blk;
				je = (j == _iWidth / blk) ? _iWidth % blk : blk;
				for(int si = 0; si < ie; si++){
					for(int sj = 0; sj < je; sj++){
						if((*pij) > tmp){
							flag = true;
							tmp = (*pij);
							ii = blk*i + si;
							jj = blk*j + sj;
						}
						pij+=_iBand;
					}
					pij+=(_iWidth- je)*_iBand;
				}
				if(flag)pt.push_back(HarrisPt(jj, ii, tmp));
			}
		}

		FILE* file = fopen("harris.txt", "w");
		for(int i = 0; i < (int)pt.size(); i++)
			fprintf(file, "%d %d\n", pt[i].x, pt[i].y);
		fclose(file);


		float upper = 0, lower = 1;
		for(int i = 0; i < _iWidth*_iHeight*_iBand; i++){
			if(lower > abs(p[i])) lower = abs(p[i]);
			if(upper < abs(p[i])) upper = abs(p[i]);
		}
	
		unsigned char* ptr = new unsigned char[_iWidth*_iHeight*_iBand];
		for(int i = 0; i < _iWidth*_iHeight*_iBand; i++){
			ptr[i] = 255*(abs(p[i])-lower) / (upper - lower);
		}
		int bm[] = {1, 2, 3};
		GDALDriver* pDrv =(GDALDriver*) GDALGetDriverByName("GTIFF");
		GDALDataset* pSet = (GDALDataset*)pDrv->Create("resfilter.tif", _iWidth, _iHeight, _iBand, GDT_Byte, NULL);
		pSet->RasterIO(GF_Write, 0, 0, _iWidth, _iHeight, ptr, _iWidth, _iHeight, GDT_Byte, _iBand, bm, _iBand, _iBand*_iWidth, 1);
		GDALClose(pSet);
        // restore the stored viewport dimensions
		if(p) delete[] p;	 
 		if(ptr) delete[] ptr;	 
		glViewport(vp[0], vp[1], vp[2], vp[3]);
    }

    void display()
    {
        // Bind the filtered texture
        glBindTexture(GL_TEXTURE_2D, _iTexture);
        glEnable(GL_TEXTURE_2D);

        // render a full-screen quad textured with the results of our 
        // computation.  Note that this is not part of the computation: this
        // is only the visualization of the results.
        glBegin(GL_QUADS);
        {
            glTexCoord2f(0, 1); glVertex2f(-1, -1);
            glTexCoord2f(1, 1); glVertex2f( 1, -1);
            glTexCoord2f(1, 0); glVertex2f( 1,  1);
            glTexCoord2f(0, 0); glVertex2f(-1,  1);
        }
        glEnd();

        glDisable(GL_TEXTURE_2D);
    }

protected: // data
    int           _iWidth, _iHeight, _iBand; // The dimensions of our array
    void*		_data;
    unsigned int  _iTexture;         // The texture used as a data array

    GLhandleARB   _programObject;    // the program used to update
    GLhandleARB   _fragmentShader;

    GLint         _texUnit;          // a parameter to the fragment program
};

// GLUT idle function
void idle()
{
    glutPostRedisplay();
}

// GLUT display function
void display()
{
   // g_pHello->update();  // update the scene and run the edge detect filter
    //g_pHello->display(); // display the results
 	pOper->Display();
	glutSwapBuffers();
}

// GLUT reshape function
void reshape(int w, int h)
{
    if (h == 0) h = 1;
    
    glViewport(0, 0, w, h);
    
    // GPGPU CONCEPT 3b: One-to-one Pixel to Texel Mapping: An Orthographic 
    //                   Projection.
    // This code sets the projection matrix to orthographic with a range of 
    // [-1,1] in the X and Y dimensions. This allows a trivial mapping of 
    // pixels to texels.
    glMatrixMode(GL_PROJECTION);    
    glLoadIdentity();               
    //gluOrtho2D(-1, 1, -1, 1);       
	glOrtho(0, w, 0, h, 0, 1);
    glMatrixMode(GL_MODELVIEW);     
    glLoadIdentity();               
}

// Called at startup
void initialize()
{
    // Initialize the "OpenGL Extension Wrangler" library
    glewInit();

    // Ensure we have the necessary OpenGL Shading Language extensions.
    if (glewGetExtension("GL_ARB_fragment_shader")      != GL_TRUE ||
        glewGetExtension("GL_ARB_vertex_shader")        != GL_TRUE ||
        glewGetExtension("GL_ARB_shader_objects")       != GL_TRUE ||
        glewGetExtension("GL_ARB_shading_language_100") != GL_TRUE)
    {
        fprintf(stderr, "Driver does not support OpenGL Shading Language\n");
        exit(1);
    }

	GDALAllRegister();
	GDALDataset* pSet = (GDALDataset*)GDALOpen("..\\test.tif",GA_ReadOnly);
	int width = pSet->GetRasterXSize();
	int height = pSet->GetRasterYSize();
	int band = pSet->GetRasterCount();
	int bm[] = {1, 2, 3};
	unsigned char* ptr = new unsigned char[width*height*band];
	pSet->RasterIO(GF_Read, 0, 0, width, height, ptr, width, height, GDT_Byte, band, bm, band, band*width, 1);
	GDALClose(pSet);

    // Create the example object
   // g_pHello = new HelloGPGPU(width, height, band, ptr);


	float* data = new float[width* height* band];
	for(int i = 0; i <width* height* band; i++)
		data[i] = ptr[i] / 255.;

	pOper = new CGLSLOperation;
	if(pOper->CreateShader(&testSource)){
		pOper->InputData(data, width, height, GL_LUMINANCE, GL_FLOAT);
	}
	
	if(ptr) delete[] ptr;
	//g_pHello->update();
}

// The main function
int main()
{
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(572, 572);
    glutCreateWindow("Hello, GPGPU! (GLSL version)");


    glutIdleFunc(idle);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);

    initialize();

    glutMainLoop();
    return 0;
}
