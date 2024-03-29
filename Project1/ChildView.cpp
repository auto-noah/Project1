
// ChildView.cpp : implementation of the CChildView class
//

#include "pch.h"
#include "framework.h"
#include "Project1.h"
#include "ChildView.h"
#include "graphics/OpenGLRenderer.h"
#include "CMyRaytraceRenderer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CChildView

CChildView::CChildView()
{
	// Set camera pos
	m_camera.Set(30., 15., 80., 0., 0., 0., 0., 1., 0.);

	// Init raytracing values
	m_raytrace = false;
	m_rayimage = NULL;

	//
	// Compose the Scene
	//

	// Init scene
	CGrPtr<CGrComposite> scene = new CGrComposite;
	m_scene = scene;

	//
	// Add a Tetrahedron
	//
	
	// Tetrahedron vertices
	double t0[] = {  5, -5, 10 }; // Base vertex 1
	double t1[] = {  0, -5,  5 };  // Base vertex 2
	double t2[] = { 10, -5,  5 }; // Base vertex 3
	double t3[] = {  5,  4,  5 };   // Apex

	// Base
	CGrPtr<CGrPolygon> tetraBase = new CGrPolygon;
	tetraBase->Texture(&m_woodtex);
	tetraBase->AddTexVertex3d(t0[0], t0[1], t0[2], 0.0, 0.0); 
	tetraBase->AddTexVertex3d(t1[0], t1[1], t1[2], 1.0, 0.0); 
	tetraBase->AddTexVertex3d(t2[0], t2[1], t2[2], 0.5, 1.0); 
	tetraBase->ComputeNormal(); 
	scene->Child(tetraBase); 
	// Face 1
	CGrPtr<CGrPolygon> tetraFace1 = new CGrPolygon;
	tetraFace1->Texture(&m_woodtex);
	tetraFace1->AddTexVertex3d(t1[0], t1[1], t1[2], 0.0, 0.0); 
	tetraFace1->AddTexVertex3d(t0[0], t0[1], t0[2], 1.0, 0.0); 
	tetraFace1->AddTexVertex3d(t3[0], t3[1], t3[2], 0.5, 1.0);
	tetraFace1->ComputeNormal();
	scene->Child(tetraFace1);
	// Face 2 
	CGrPtr<CGrPolygon> tetraFace2 = new CGrPolygon;
	tetraFace2->Texture(&m_woodtex);
	tetraFace2->AddTexVertex3d(t2[0], t2[1], t2[2], 0.0, 0.0); 
	tetraFace2->AddTexVertex3d(t1[0], t1[1], t1[2], 1.0, 0.0); 
	tetraFace2->AddTexVertex3d(t3[0], t3[1], t3[2], 0.5, 1.0);
	tetraFace2->ComputeNormal();
	scene->Child(tetraFace2);
	// Face 3 
	CGrPtr<CGrPolygon> tetraFace3 = new CGrPolygon;
	tetraFace3->Texture(&m_woodtex);
	tetraFace3->AddTexVertex3d(t0[0], t0[1], t0[2], 0.0, 0.0);
	tetraFace3->AddTexVertex3d(t2[0], t2[1], t2[2], 1.0, 0.0);
	tetraFace3->AddTexVertex3d(t3[0], t3[1], t3[2], 0.5, 1.0);
	tetraFace3->ComputeNormal();
	scene->Child(tetraFace3);

	//
	// Add a floor
	//

	// Load floor texture
	m_rwtiletex.LoadFile(L"textures/redwhitetile.bmp");
	
	// Define the vertices of the floor
	double f0[] = { -22, -5, -15 }; // Bottom-left corner 
	double f1[] = {  15, -5, -15 }; // Bottom-right corner 
	double f2[] = {  15, -5,  15 }; // Top-right corner 
	double f3[] = { -22, -5,  15 }; // Top-left corner 

	// Floor
	CGrPtr<CGrPolygon> floor = new CGrPolygon;
	floor->Texture(&m_rwtiletex);
	floor->AddTexVertex3d(f0[0], f0[1], f0[2], 0.0, 0.0); 
	floor->AddTexVertex3d(f3[0], f3[1], f3[2], 0.0, 1.0); 
	floor->AddTexVertex3d(f2[0], f2[1], f2[2], 1.0, 1.0); 
	floor->AddTexVertex3d(f1[0], f1[1], f1[2], 1.0, 0.0); 
	floor->ComputeNormal(); 
	scene->Child(floor); 

	//
	// Make boxes
	// 
	
	// Load textures for boxes
	m_worldtex.LoadFile(L"textures/worldmap.bmp");
	m_woodtex.LoadFile(L"textures/plank01.bmp");

	// A red box
	CGrPtr<CGrMaterial> redpaint = new CGrMaterial;
	redpaint->AmbientAndDiffuse(0.8f, 0.0f, 0.0f);
	redpaint->Specular(1.0f, 1.0f, 1.0f); // Make red box mirror-like.
	redpaint->Shininess(100);
	scene->Child(redpaint);

	CGrPtr<CGrComposite> redbox = new CGrComposite;
	redpaint->Child(redbox);
	redbox->Box(-15, -5, 2, 5, 5, 5);

	// A white box
	CGrPtr<CGrMaterial> whitepaint = new CGrMaterial;
	whitepaint->AmbientAndDiffuse(0.8f, 0.8f, 0.8f);
	scene->Child(whitepaint);

	CGrPtr<CGrComposite> whitebox = new CGrComposite;
	whitepaint->Child(whitebox);
	whitebox->Box(-8, -5, -8, 5, 5, 5, &m_worldtex);
}

CChildView::~CChildView()
{
	// delete image allocation
	if (m_raytrace)
		DeleteRaytraceImage();
}

void CChildView::DeleteRaytraceImage()
{
	// deallocate memory
	delete[] m_rayimage[0];
	delete[] m_rayimage;
}

BEGIN_MESSAGE_MAP(CChildView, COpenGLWnd)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_COMMAND(ID_RENDER_RAYTRACE, &CChildView::OnRenderRaytrace)
	ON_UPDATE_COMMAND_UI(ID_RENDER_RAYTRACE, &CChildView::OnUpdateRenderRaytrace)
END_MESSAGE_MAP()



// CChildView message handlers

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!COpenGLWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), nullptr);

	return TRUE;
}

void CChildView::OnGLDraw(CDC* pDC)
{
	if (m_raytrace)
	{
		// Clear the color buffer
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Set up for parallel projection
		int width, height;
		GetSize(width, height);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, width, 0, height, -1, 1);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		// If we got it, draw it
		if (m_rayimage)
		{
			glRasterPos3i(0, 0, 0);
			glDrawPixels(m_rayimagewidth, m_rayimageheight,
				GL_RGB, GL_UNSIGNED_BYTE, m_rayimage[0]);
		}

		glFlush();
	}
	else
	{
		//
		// Instantiate a renderer
		//

		COpenGLRenderer renderer;

		// Configure the renderer
		ConfigureRenderer(&renderer);

		//
		// Render the scene
		//

		renderer.Render(m_scene);
	}
}

void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_camera.MouseDown(point.x, point.y);

	COpenGLWnd::OnLButtonDown(nFlags, point);
}


void CChildView::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_camera.MouseMove(point.x, point.y, nFlags))
		Invalidate();

	COpenGLWnd::OnMouseMove(nFlags, point);
}


void CChildView::OnRButtonDown(UINT nFlags, CPoint point)
{
	m_camera.MouseDown(point.x, point.y, 2);

	COpenGLWnd::OnRButtonDown(nFlags, point);
}

//
// Name :         CChildView::ConfigureRenderer()
// Description :  Configures our renderer so it is able to render the scene.
//                Indicates how we'll do our projection, where the camera is,
//                and where any lights are located.
//

void CChildView::ConfigureRenderer(CGrRenderer* p_renderer)
{
	// Determine the screen size so we can determine the aspect ratio
	int width, height;
	GetSize(width, height);
	double aspectratio = double(width) / double(height);

	//
	// Set up the camera in the renderer
	//

	p_renderer->Perspective(m_camera.FieldOfView(),
		aspectratio, // The aspect ratio.
		20., // Near clipping
		1000.); // Far clipping

	// m_camera.FieldOfView is the vertical field of view in degrees.

	//
	// Set the camera location
	//

	p_renderer->LookAt(m_camera.Eye()[0], m_camera.Eye()[1], m_camera.Eye()[2],
		m_camera.Center()[0], m_camera.Center()[1], m_camera.Center()[2],
		m_camera.Up()[0], m_camera.Up()[1], m_camera.Up()[2]);

	//
	// Set the light locations and colors
	//

	float dimAmbient = 0.3f; 
	GLfloat ambientColor[] = { dimAmbient, dimAmbient, dimAmbient, 1.0f }; 
	GLfloat lightDiffuse[] = { 0.6f, 0.6f, 0.6f, 1.0f };
	GLfloat lightSpecular[] = { 0.7f, 0.7f, 0.7f, 1.0f };

	p_renderer->AddLight(CGrPoint(20, 5, 25, 0),   // Light 1
		ambientColor, lightDiffuse, lightSpecular);
	p_renderer->AddLight(CGrPoint(-20, 5, -25, 0), // Light 2
		ambientColor, lightDiffuse, lightSpecular);
}


void CChildView::OnRenderRaytrace()
{
	m_raytrace = !m_raytrace;
	Invalidate();
	if (!m_raytrace)
	{
		DeleteRaytraceImage();
		return;
	}

	GetSize(m_rayimagewidth, m_rayimageheight);

	m_rayimage = new BYTE * [m_rayimageheight];

	int rowwid = m_rayimagewidth * 3;
	while (rowwid % 4)
		rowwid++;

	m_rayimage[0] = new BYTE[m_rayimageheight * rowwid];
	for (int i = 1; i < m_rayimageheight; i++)
	{
		m_rayimage[i] = m_rayimage[0] + i * rowwid;
	}

	for (int i = 0; i < m_rayimageheight; i++)
	{
		// Fill the image with blue
		for (int j = 0; j < m_rayimagewidth; j++)
		{
			m_rayimage[i][j * 3] = 0;               // red
			m_rayimage[i][j * 3 + 1] = 0;           // green
			m_rayimage[i][j * 3 + 2] = BYTE(255);   // blue
		}
	}
	
	// Instantiate a raytrace object
	CMyRaytraceRenderer raytrace;

	// Generic configurations for all renderers
	ConfigureRenderer(&raytrace);

	//
	// Render the Scene
	//
	raytrace.SetImage(m_rayimage, m_rayimagewidth, m_rayimageheight);
	raytrace.SetWindow(this);
	raytrace.Render(m_scene);
	Invalidate();
}


void CChildView::OnUpdateRenderRaytrace(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_raytrace);
}
