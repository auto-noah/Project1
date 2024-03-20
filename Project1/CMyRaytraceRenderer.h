#pragma once
#include "graphics/GrRenderer.h"
#include "graphics/RayIntersection.h"

class CMyRaytraceRenderer :
	public CGrRenderer
{
public:
    CMyRaytraceRenderer() { m_window = NULL; }
    int     m_rayimagewidth;
    int     m_rayimageheight;
    BYTE** m_rayimage;
    void SetImage(BYTE** image, int w, int h) { m_rayimage = image; m_rayimagewidth = w;  m_rayimageheight = h; }

    CWnd* m_window;

    CRayIntersection m_intersection;

    std::list<CGrTransform> m_mstack;
    CGrMaterial* m_material;

    void SetWindow(CWnd* p_window);
    bool RendererStart();
    bool RendererEnd();
    void RendererMaterial(CGrMaterial* p_material);

    virtual void RendererPushMatrix();
    virtual void RendererPopMatrix();
    virtual void RendererRotate(double a, double x, double y, double z);
    virtual void RendererTranslate(double x, double y, double z);
    void RendererEndPolygon();

    void RayColor(const CRay& p_ray, CGrPoint& p_color, int p_recurse, const CRayIntersection::Object* p_ignore);

    CGrPoint CalculateLighting(const CGrPoint& N, CGrMaterial* material, const Light& light, const CGrPoint& lightDir, const CGrPoint& intersectionPoint, CGrPoint color);

    CGrPoint CalculateIndirectSpecular(const CRay& ray, const CGrPoint& N, const CGrPoint& intersectionPoint, int recurse);

    double* blinnPhongDir(const CGrPoint& lightDir, const CGrPoint& normal, float lightInt, float Ka, float Kd, float Ks, float shininess, const CGrPoint& intersectionPoint);
};

