#include <UT/UT_Math.h>
#include <GU/GU_Detail.h>

static float
sphere(const UT_Vector3 &p)
{
    float        x, y, z;
    x = p.x();
    y = p.y();
    z = p.z();
    
    

    return x*x + y*y + z*z- 2;
}




int
main()
{
    GU_Detail                gdp;
    UT_BoundingBox        bbox;



    bbox.initBounds(-2, -2, -2);
    bbox.enlargeBounds(2, 2, 2);
    gdp.polyIsoSurface(sphere, bbox, 20, 80, 20);
    gdp.save("sphere.bgeo");
    gdp.clearAndDestroy();
    gdp.polymeshCube(4,4,4);
    gdp.save("cube.bgeo");

    
    return 0;
}
