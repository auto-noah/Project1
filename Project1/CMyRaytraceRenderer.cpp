#include "pch.h"
#include "CMyRaytraceRenderer.h"

void CMyRaytraceRenderer::SetWindow(CWnd* p_window)
{
    m_window = p_window;
}

bool CMyRaytraceRenderer::RendererStart()
{
	m_intersection.Initialize();

	m_mstack.clear();

	// We have to do all of the matrix work ourselves.
	// Set up the matrix stack.
	CGrTransform t;
	t.SetLookAt(Eye().X(), Eye().Y(), Eye().Z(),
		Center().X(), Center().Y(), Center().Z(),
		Up().X(), Up().Y(), Up().Z());

	m_mstack.push_back(t);

	m_material = NULL;

	return true;
}

void CMyRaytraceRenderer::RendererMaterial(CGrMaterial* p_material)
{
	m_material = p_material;
}

// These 4 are done already
void CMyRaytraceRenderer::RendererPushMatrix()
{
	m_mstack.push_back(m_mstack.back());
}
void CMyRaytraceRenderer::RendererPopMatrix()
{
	m_mstack.pop_back();
}
void CMyRaytraceRenderer::RendererRotate(double a, double x, double y, double z)
{
	CGrTransform r;
	r.SetRotate(a, CGrPoint(x, y, z));
	m_mstack.back() *= r;
}
void CMyRaytraceRenderer::RendererTranslate(double x, double y, double z)
{
	CGrTransform r;
	r.SetTranslate(x, y, z);
	m_mstack.back() *= r;
}

//
// Name : CMyRaytraceRenderer::RendererEndPolygon()
// Description : End definition of a polygon. The superclass has
// already collected the polygon information
//

void CMyRaytraceRenderer::RendererEndPolygon()
{
    const std::list<CGrPoint>& vertices = PolyVertices();
    const std::list<CGrPoint>& normals = PolyNormals();
    const std::list<CGrPoint>& tvertices = PolyTexVertices();

    // Allocate a new polygon in the ray intersection system
    m_intersection.PolygonBegin();
    m_intersection.Material(m_material);

    if (PolyTexture())
    {
        m_intersection.Texture(PolyTexture());
    }

    std::list<CGrPoint>::const_iterator normal = normals.begin();
    std::list<CGrPoint>::const_iterator tvertex = tvertices.begin();

    for (std::list<CGrPoint>::const_iterator i = vertices.begin(); i != vertices.end(); i++)
    {
        if (normal != normals.end())
        {
            m_intersection.Normal(m_mstack.back() * *normal);
            normal++;
        }

        if (tvertex != tvertices.end())
        {
            m_intersection.TexVertex(*tvertex);
            tvertex++;
        }

        m_intersection.Vertex(m_mstack.back() * *i);
    }

    m_intersection.PolygonEnd();
}

bool CMyRaytraceRenderer::RendererEnd()
{
	m_intersection.LoadingComplete();

	double ymin = -tan(ProjectionAngle() / 2 * GR_DTOR);
	double yhit = -ymin * 2;

	double xmin = ymin * ProjectionAspect();
	double xwid = -xmin * 2;

	for (int r = 0; r < m_rayimageheight; r++)
	{
		for (int c = 0; c < m_rayimagewidth; c++)
		{
			double colorTotal[3] = { 0, 0, 0 };

			double x = xmin + (c + 0.5) / m_rayimagewidth * xwid;
			double y = ymin + (r + 0.5) / m_rayimageheight * yhit;

			// Construct a Ray
			CRay ray(CGrPoint(0, 0, 0), Normalize3(CGrPoint(x, y, -1, 0)));

			double t;                                   // Will be distance to intersection
			CGrPoint intersect;                         // Will by x,y,z location of intersection
			const CRayIntersection::Object* nearest;    // Pointer to intersecting object
			if (m_intersection.Intersect(ray, 1e20, NULL, nearest, t, intersect))
			{
				// We hit something...
				// Determine information about the intersection
				CGrPoint N;
				CGrMaterial* material;
				CGrTexture* texture;
				CGrPoint texcoord;
				m_intersection.IntersectInfo(ray, nearest, t, N, material, texture, texcoord);

				//
				// Custom Raytracer Lighting Code (BROKEN ATM)
				//

				CGrPoint color = material->Ambient(); // Start with the ambient light

				// Iterate over all lights in the renderer
				for (int i = 0; i < LightCnt(); ++i) 
				{
					// Get current light and its direction
					const Light& light = GetLight(i); 
					CGrPoint lightDir = light.m_pos - intersect; 

					// Calculate the length of the light direction vector
					double lightDistance = sqrt(lightDir.X() * lightDir.X() + lightDir.Y() * lightDir.Y() + lightDir.Z() * lightDir.Z()); 

					// Normalize the light direction vector
					if (lightDistance != 0) // Avoid division by zero 
					{
						lightDir = CGrPoint(lightDir.X() / lightDistance, lightDir.Y() / lightDistance, lightDir.Z() / lightDistance); 
					}

					// Offset the origin to avoid self-intersection
					CRay shadowRay(intersect + N * 0.001, lightDir); 

					// Check if the shadow ray hits any object before reaching the light (ShadowFeeler)
					const CRayIntersection::Object* shadowNearest; 
					if (!m_intersection.Intersect(shadowRay, lightDistance, nearest, shadowNearest, t, intersect)) 
					{
						// If no intersection, the point is not in shadow for this light
						color += CalculateLighting(N, material, light, lightDir);
					}
				}

				if (material != NULL)
				{
					// Convert color to bytes and write to image buffer
					m_rayimage[r][c * 3] = BYTE(material->Diffuse(0) * 255);
					m_rayimage[r][c * 3 + 1] = BYTE(material->Diffuse(1) * 255);
					m_rayimage[r][c * 3 + 2] = BYTE(material->Diffuse(2) * 255);
				}
			}
			else
			{
				// We hit nothing...
				m_rayimage[r][c * 3] = 0;
				m_rayimage[r][c * 3 + 1] = 0;
				m_rayimage[r][c * 3 + 2] = 0;
			}
		}
		if ((r % 50) == 0)
		{
			m_window->Invalidate();
			MSG msg;
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				DispatchMessage(&msg);
		}
	}

	return true;
}

CGrPoint CMyRaytraceRenderer::CalculateLighting(const CGrPoint& N, CGrMaterial* material, const Light& light, const CGrPoint& lightDir)
{
	// Calculate the diffuse and specular contribution from the light
	// ... (implement lighting calculation here)

	// return lightingColor;

	return CGrPoint(0., 0., 0., 1.0);
}
