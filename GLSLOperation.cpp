#include "GLSLOperation.h"
#include <memory>
#include <vector>
#include "gdal_priv.h"
#pragma  comment(lib, "gdal_i.lib")

using namespace std;


CGLSLOperation::CGLSLOperation(void)
{
	_textureID = 0;
	_iWidth = 0;
	_iHeight = 0;
	_iBand = 0;
	_texUnit = -1;
	_texSize = -1;
	FitViewPort(1, 1);
}


CGLSLOperation::~CGLSLOperation(void)
{
}

bool CGLSLOperation::InputData( void* pImg, int wid, int hei, int fomat, int type )
{
	if(!pImg || wid < 1 || hei < 1) return false;
   
	_iWidth = wid; 
	_iHeight = hei;
	_iBand = fomat == GL_RGB ? 3 : 1;

	glGenTextures(1, &_textureID);
	glBindTexture(GL_TEXTURE_2D, _textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, wid, hei, 0,fomat, type, pImg);
	//绘制纹理	
	FitViewPort(wid, hei);
	DrawQuad();

	glUseProgramObjectARB(_programObject);
	//传入uniform参数
	glUniform1iARB(_texUnit, 0);	
	glUniform2fARB(_texSize, wid, hei);
	//重新绘制
	DrawQuad();
	//绑定绘制后的纹理
	glBindTexture(GL_TEXTURE_2D, _textureID);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, _iWidth, _iHeight);

	float* p = new float[_iWidth*_iHeight*_iBand];
	memset(p, 0, _iWidth*_iHeight*_iBand*sizeof(float));
	//拷贝数据
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,
		GL_TEXTURE_RECTANGLE_ARB, _textureID, 0);
	glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
	glRasterPos2i(0, 0);
	glReadPixels(0, 0, _iWidth, _iHeight, GL_LUMINANCE, GL_FLOAT, p);
	 //处理后的影像
	_iBand = 1;
	unsigned char* ptr = new unsigned char[_iWidth*_iHeight*_iBand];
	for(int i = 0, j= 0; i < _iWidth*_iHeight*_iBand; i++, j+=1){
		ptr[i] = 255*p[j];
	}
	int bm[] = {1, 2, 3};
	GDALDriver* pDrv =(GDALDriver*) GDALGetDriverByName("GTIFF");
	GDALDataset* pSet = (GDALDataset*)pDrv->Create("..\\resfilter.tif", _iWidth, _iHeight, _iBand, GDT_Byte, NULL);
	pSet->RasterIO(GF_Write, 0, 0, _iWidth, _iHeight, ptr, _iWidth, _iHeight, GDT_Byte, _iBand, bm, _iBand, _iBand*_iWidth, 1);
	GDALClose(pSet);
	if(ptr) delete[] ptr;
    return true;
  	//
	//Harris角点
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

	FILE* file = fopen("GLharris.txt", "w");
	for(int i = 0; i < (int)pt.size(); i++)
		fprintf(file, "%d %d\n", pt[i].x, pt[i].y);
	fclose(file);
	return true;
}

bool CGLSLOperation::GetOutputData( void* pImg )
{
/*	float* p = new float[_iWidth*_iHeight*_iBand];
	memset(p, 0, _iWidth*_iHeight*_iBand*sizeof(float));

	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,
		GL_TEXTURE_RECTANGLE_ARB,  _textureID, 0);

	glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
	glRasterPos2i(0, 0);
	glReadPixels(0, 0, _iWidth, _iHeight, GL_RGB, GL_FLOAT, p);

   */

   return true;

}


bool CGLSLOperation::CreateShader( const char** shader )
{
	_programObject = glCreateProgramObjectARB();

	// Create the edge detection fragment program
	_fragmentShader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glShaderSourceARB(_fragmentShader, 1, shader, NULL);
	glCompileShaderARB(_fragmentShader);
	glAttachObjectARB(_programObject, _fragmentShader);

	// Link the shader into a complete GLSL program.
	glLinkProgramARB(_programObject);
	GLint progLinkSuccess;
	glGetObjectParameterivARB(_programObject, GL_OBJECT_LINK_STATUS_ARB,
		&progLinkSuccess);
	if (!progLinkSuccess){
		GLint len = 0;	
		glGetObjectParameterivARB(_programObject, GL_INFO_LOG_LENGTH , &len);
		if(len <=1) return false;

		char * compileLog = new char[len+1];
		if(compileLog == NULL) return false;

		glGetInfoLogARB(_programObject, len, &len, compileLog);
		printf("Compile Log:%s\n", compileLog);
		delete[] compileLog;
		system("pause");
		exit(0);
		return false;
	}
	// Get location of the sampler uniform
	_texUnit = glGetUniformLocationARB(_programObject, "texUnit");
	_texSize = glGetUniformLocationARB(_programObject, "texSize");
	return true;
}



void CGLSLOperation::FitViewPort(int width, int height)
{
	GLint port[4];
	glGetIntegerv(GL_VIEWPORT, port);
	if(port[2] !=width || port[3] !=height){
		glViewport(0, 0, width, height);      
		glMatrixMode(GL_PROJECTION);    
		glLoadIdentity();               
		//glOrtho(-1, 1, -1, 1,  0, 1);		
		glOrtho(0, width, 0, height, 0, 1);
		glMatrixMode(GL_MODELVIEW);     
		glLoadIdentity();  
	}
}


void CGLSLOperation::DrawQuad(){
	/*glBegin(GL_QUADS);
	{            
		glTexCoord2f(0, 0); glVertex2f(-1, -1);
		glTexCoord2f(1, 0); glVertex2f( 1, -1);
		glTexCoord2f(1, 1); glVertex2f( 1,  1);
		glTexCoord2f(0, 1); glVertex2f(-1,  1);
	}  
	glEnd();
	glFlush();
	return;	 */
	glBegin (GL_QUADS);
		glTexCoord2i ( 0,   0);	glVertex2i   ( 0,	0 ); 
		glTexCoord2i ( 0,   1);	glVertex2i   ( 0,	_iHeight); 
 		glTexCoord2i ( 1,   1); 	glVertex2i   ( _iWidth,	_iHeight); 
		glTexCoord2i ( 1,   0);	glVertex2i   ( _iWidth,	0); 
	glEnd ();
	glFlush();
}

void CGLSLOperation::Display()
{
	glBindTexture(GL_TEXTURE_2D, _textureID);
	glEnable(GL_TEXTURE_2D);
	DrawQuad();
	glDisable(GL_TEXTURE_2D);
	return;
	
	glBindTexture(GL_TEXTURE_2D, _textureID);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	{            
		glTexCoord2f(0, 0); glVertex2f(-1, -1);
		glTexCoord2f(1, 0); glVertex2f( 1, -1);
		glTexCoord2f(1, 1); glVertex2f( 1,  1);
		glTexCoord2f(0, 1); glVertex2f(-1,  1);
	}  
	glEnd();
	glDisable(GL_TEXTURE_2D);

}
