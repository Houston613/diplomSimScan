#include "StdAfx.h"

#include "resource.h"
#include <dbSubD.h>
#include "util.h"
#include "simsc.h"
#include "dbgroup.h"
#include <fstream>


//#define DEBUGFILE

// – - точка с координатами расположени€ сканера в момент сканировани€, принадлежаща€ множеству точек траектории сканера
// v - радиус вектор луча сканировани€, начинающийс€ в точке –сканера и лежащий в плоскости, перпендикул€рной траектории движени€ сканера
// ids - ћассив индексов отдельных сеток, из которых состоит описываема€ сцена
// ret - результат, содержит координаты точки сканера и параметрическа€ запись луча
bool getPointScan1(const AcGePoint3d& p, const AcGeVector3d& v, 
    const AcDbObjectIdArray& ids, AcGePoint3d& ret, AcString& resultedGroup)
{
    AcDbObjectId objectId;
    double zmin = DBL_MAX;
    AcGeMatrix3d mat = AcGeMatrix3d::worldToPlane(AcGePlane(p,v));//матрица дает переход к —  точки сканера и плоскостью с нормалью, выраженной вектором луча сканера

    for (int i = 0; i < ids.length(); i++)
    {
        AcGePoint3dArray vrtx;
        AcArray<Adesk::Int32> face;
        AcDbSubDMesh* mesh = nullptr;


        if (acdbOpenObject(mesh, ids[i], AcDb::kForRead) == Acad::eOk)//чтение меша из Ѕƒ
        {
            mesh->getVertices(vrtx);
            mesh->getFaceArray(face);
            mesh->close();
            
        }
        for (int j = 0; j < vrtx.length(); j++) vrtx[j] = mat * vrtx[j]; // переход в —  сканера
        AcGeVector3d v[3];
        for (int k = 0; k < face.length(); k += 4)//рассматриваем грани
        {
            for (int u = 0; u < 3; u++) v[u] = vrtx[face[k + 1 + u]].asVector();//расписать че происходит


            if ((v[0].z < 0.) && (v[1].z < 0.) && (v[2].z < 0.)) continue;
#if 0
            if ((v[0].x < 0.) && (v[1].x < 0.) && (v[2].x < 0.)) continue;
            if ((v[0].x > 0.) && (v[1].x > 0.) && (v[2].x > 0.)) continue;
            if ((v[0].y < 0.) && (v[1].y < 0.) && (v[2].y < 0.)) continue;
            if ((v[0].y > 0.) && (v[1].y > 0.) && (v[2].y > 0.)) continue;
#endif
            double f[3], zp;
            f[0] = v[1].x * v[2].y - v[1].y * v[2].x;
            if (f[0] > 0.) continue;
            f[1] = v[2].x * v[0].y - v[2].y * v[0].x;
            if (f[1] > 0.) continue;
            f[2] = v[0].x * v[1].y - v[0].y * v[1].x;
            if (f[2] > 0.) continue;
            zp = (f[0] * v[0].z + f[1] * v[1].z + f[2] * v[2].z) / (f[0] + f[1] + f[2]);
            if (zp < zmin)
            {
                zmin = zp;
                objectId = ids.at(i);

            }
        }
    }
    AcDbDictionary* pDict;
    acdbHostApplicationServices()->workingDatabase()->getGroupDictionary(pDict, AcDb::kForRead);


    AcDbDictionaryIterator* pDictIter = pDict->newIterator();
    std::FILE* outFile = std::fopen("C:\\Output\\output.txt", "a"); // Open the file in append mode

    for (; !pDictIter->done(); pDictIter->next())
    {
        AcDbGroup* pGroup;
        acdbOpenObject(pGroup, pDictIter->objectId(), AcDb::kForRead);
        AcDbObjectIdArray idsInGroup;
        pGroup->allEntityIds(idsInGroup);
        AcString groupName;
        pGroup->getName(groupName);

        if (idsInGroup.contains(objectId)) {
            // Write the groupName to the file
            //fprintf(outFile, "%s\n", groupName.constPtr());
            pGroup->getName(resultedGroup);

        }

        pGroup->close();
    }
    delete pDictIter;
    pDict->close();
    std::fclose(outFile);
    ret = p + v * zmin;
    return (zmin != DBL_MAX);
}
