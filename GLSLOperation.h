#pragma once
//#include "GL/GL.h"
#include "GL/glew.h"
#include "GL/glut.h"


class CGLSLOperation
{
public:
	CGLSLOperation(void);
	virtual ~CGLSLOperation(void);

	bool CreateShader(const char** shader);
	bool InputData(void* pImg, int wid, int hei, int fomat, int type);
	bool GetOutputData(void* pImg);
	void Display();
protected:
	void FitViewPort(int width, int height);
	void DrawQuad();
protected:
	unsigned int  _textureID;         // The texture used as a data array
	int _iWidth, _iHeight, _iBand;
	GLhandleARB _programObject;
	GLhandleARB   _fragmentShader;
	GLint         _texUnit;          // a parameter to the fragment program
	GLint		 _texSize;
};

// forward declarations
struct HarrisPt{
	int x, y;
	float value;
	HarrisPt(int s, int t, float v):x(s),y(t),value(v){};
};

