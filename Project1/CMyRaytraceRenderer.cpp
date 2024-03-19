#include "pch.h"
#include "CMyRaytraceRenderer.h"
#include "graphics/GrTexture.h"
#include <algorithm>
#include <cmath>

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

void CMyRaytraceRenderer::RayColor(const CRay& ray, CGrPoint& color, int recurse, const CRayIntersection::Object* ignore)
{
    double t; // Will be distance to intersection
    CGrPoint intersect; // Will by x,y,z location of intersection
    const CRayIntersection::Object* nearest; // Pointer to intersecting object

    if (m_intersection.Intersect(ray, 1e20, ignore, nearest, t, intersect))
    {
        // We hit something...
        CGrPoint N; // Normal at the intersection
        CGrMaterial* material; // Material at the intersection
        CGrTexture* texture; // Texture at the intersection (if any)
        CGrPoint texcoord; // Texture coordinates at the intersection (if any)
        m_intersection.IntersectInfo(ray, nearest, t, N, material, texture, texcoord);

        // The color computation starts here
        if (texture != NULL)
        {
            // Use texture coordinates to sample the texture color
            CGrPoint textureColor = texture->Sample(texcoord.X(), texcoord.Y());
            color = textureColor; // Start with the texture color
        }
        else if (material != NULL)
        {
            // Use the ambient color of the material if there's no texture
            color = material->Ambient();
        }
        else
        {
            // Default to white if there's neither texture nor material
            color = CGrPoint(1.0, 1.0, 1.0);
        }
        

        // Now apply lighting to the texture or material color
        for (int i = 0; i < LightCnt(); ++i)
        {
            const Light& light = GetLight(i);
            CGrPoint lightDir = light.m_pos - intersect;

            double length = sqrt(lightDir.X() * lightDir.X() + lightDir.Y() * lightDir.Y() + lightDir.Z() * lightDir.Z());
            if (length != 0) // Avoid division by zero 
            {
                lightDir = lightDir / length;
            }

            CRay shadowRay(intersect + N * 0.001, lightDir);
            const CRayIntersection::Object* shadowNearest;
            if (!m_intersection.Intersect(shadowRay, length, nearest, shadowNearest, t, intersect))
            {
                // If no intersection, the point is not in shadow for this light
                color += CalculateLighting(N, material, light, lightDir, intersect, color);
            }
        }
    }
    else
    {
        // No intersection: return black as background color
        color = CGrPoint(0, 0, 0); 
    }
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
            double x = xmin + (c + 0.5) / m_rayimagewidth * xwid;
            double y = ymin + (r + 0.5) / m_rayimageheight * yhit;

            // Construct a Ray
            CRay ray(CGrPoint(0, 0, 0), Normalize3(CGrPoint(x, y, -1)));
            CGrPoint color;

            // Compute the color for the ray
            RayColor(ray, color, 0, NULL);

            // Convert the color to bytes and write to the image buffer
            m_rayimage[r][c * 3] = static_cast<BYTE>((((255) < (color.X() * 255)) ? (255) : (color.X() * 255 *0.1)));     // TODO: Remove "* 0.1" from the end of all three of 
            m_rayimage[r][c * 3 + 1] = static_cast<BYTE>((((255) < (color.Y() * 255)) ? (255) : (color.Y() * 255 * 0.1))); // these and figure out why everything is washed out
            m_rayimage[r][c * 3 + 2] = static_cast<BYTE>((((255) < (color.Z() * 255)) ? (255) : (color.Z() * 255 * 0.1))); // (most likely issue with blinn phong color calc)
        }

        // Refresh the window every 50 rows to show progress
        if ((r % 50) == 0)
        {
            m_window->Invalidate();
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    return true;
}

double* CMyRaytraceRenderer::blinnPhongDir(const CGrPoint& lightDir, const CGrPoint& normal, float lightInt, float Ka, float Kd, float Ks, float shininess, const CGrPoint& intersectionPoint)
{
	// Calculate diffuse and specular components

    // View direction
    CGrPoint viewDir = Normalize3(Eye() - intersectionPoint);

    // Halfway direction
    CGrPoint halfwayDir = Normalize3(lightDir + viewDir);

    // Diffuse component
    float diffuse = Ka + Kd * lightInt * max(Dot3(lightDir, normal), 0.0);

    // Specular component
    float spec = Ks * pow(max(Dot3(halfwayDir, normal), 0.0), shininess);

    return new double[2] { diffuse, spec };
}

CGrPoint CMyRaytraceRenderer::CalculateLighting(const CGrPoint& N, CGrMaterial* material, const Light& light, const CGrPoint& lightDir, const CGrPoint& intersectionPoint, CGrPoint color)
{
    // Default values for lighting components
    const float defaultKa = 0.3;
    const float defaultKd = 0.7;
    const float defaultKs = 0.3;
    const float defaultShininess = 5; // This is a guess, adjust as needed

    // Check if the material is not NULL, if it is, use the default values
    float Ka = (material != nullptr) ? *material->Ambient() : defaultKa;
    float Kd = (material != nullptr) ? *material->Diffuse() : defaultKd;
    float Ks = (material != nullptr) ? *material->Specular() : defaultKs;
    float shininess = (material != nullptr) ? material->Shininess() : defaultShininess;

    // Call blinnPhongDir with either the material's properties or the default values
    double* diffuseAndSpecular = blinnPhongDir(lightDir, N, 0.25, Ka, Kd, Ks, shininess, intersectionPoint); // Light Intesity needs to be figured out (1.0 value whites out the scene)


    // Create the lighting color based on diffuse and specular components
    //CGrPoint lightingColor = color * diffuseAndSpecular[0] + color * diffuseAndSpecular[1];

    CGrPoint color2 = color;
    if(material != NULL)
        color2 = CGrPoint(material->Diffuse(0), material->Diffuse(1), material->Diffuse(2));

    CGrPoint lightingColor = color2 * diffuseAndSpecular[0] + color2 * diffuseAndSpecular[1];


    // Clean up the allocated array to prevent memory leaks
    delete[] diffuseAndSpecular;

    return lightingColor;
}
