#include "StdAfx.h"
//////////////////////////////////////
#include <dbents.h>
#include <dbpl.h>
#include <math.h>
#include "axlock.h"
#include <dblayout.h>
#include <dbSubD.h>
#include "util.h"
#include "simsc.h"

void CsimscanApp::Houston_pfm2msh(void) //Конверировать многогранную сеть в сеть
{
	ads_name ssObj;//Набор AutoCAD выбранных объектов
	acutPrintf(_T("\nВыберите объекты для преобразования..."));//Вывести приглашение
	//Выполнить интерактивный запрос
	if(acedSSGet(NULL, NULL, NULL, NULL, ssObj) != RTNORM)
	{
		acutPrintf(_T("\n Прервано пользователем..."));
		return;
	}
	Adesk::Int32 numEnt;
	acedSSLength(ssObj, &numEnt);	//Считать количество выбранных примитивов
	if (numEnt == 0)//Если набор пустой
	{
		acedSSFree(ssObj);//Очистить набор
		acutPrintf(_T("\nНичего не выбрано..."));
		return;	
	}
	int numPfc = 0; //Количество в наборе примитивов AcDbPolyFaceMesh (многогранная сеть)
	int numConv = 0; //Количество удачно конвертированных примитивов в сетку
	//Цикл по выбранным объектам
	for (int i = 0; i < numEnt; i++) //Чтение примитивов из набора 
	{
		ads_name entName;
		AcDbObjectId entId;
		AcDbPolyFaceMesh* pfmesh = nullptr;
		if ((acedSSName(ssObj, i, entName) == RTNORM) &&
			(acdbGetObjectId(entId, entName) == Acad::eOk) &&
			(acdbOpenObject(pfmesh, entId, AcDb::kForRead) == Acad::eOk))
		{
			numPfc++;
			AcGePoint3dArray vertex;
			AcArray<Adesk::Int32> face;
			AcDbObjectIterator* iter = pfmesh->vertexIterator();
			for (iter->start(); !iter->done(); iter->step())
			{
				AcDbEntity* subEnt;
				if (acdbOpenObject(subEnt, iter->objectId(), AcDb::kForRead) == Acad::eOk)
				{
					if (subEnt->isKindOf(AcDbPolyFaceMeshVertex::desc()))
						vertex.append((AcDbPolyFaceMeshVertex::cast(subEnt))->position());
					else if (subEnt->isKindOf(AcDbFaceRecord::desc()))
					{
						AcDbFaceRecord* pFaceRecord = AcDbFaceRecord::cast(subEnt);

						face.append(3);
						for (Adesk::Int16 q, i = 0; i < 3; ++i)
						{
							pFaceRecord->getVertexAt(i, q);
							face.append((Adesk::Int32)(q < 0 ? -q - 1 : q - 1));
						}
					}
					subEnt->close();
				}
			}
			delete iter;//Удалить итератор
			//Массивы вершин (vertex) и индексов граней (face) сформированы
			// Теперь по ним формируем сетку
			int subDLevel = 0;
			AcDbObjectId mId;
			AcDbSubDMesh* mesh = new AcDbSubDMesh();
			if ((mesh->setSubDMesh(vertex, face, subDLevel) == Acad::eOk) &&
				(util::AddEntityToModelSpace(mId, mesh, false) == Acad::eOk))
			{
				numConv++;
				mesh->setLayer(pfmesh->layerId());//Установить слой
				mesh->setColor(pfmesh->color());//Установить цвет
				mesh->setTransparency(pfmesh->transparency());//Установить прозрачность
				mesh->close();
				//Удалить исходную многогранную сеть
				pfmesh->upgradeOpen();
				pfmesh->erase();
			}
			else delete mesh;
			pfmesh->close();
		}
	}
	acedSSFree(ssObj);//Очистить набор AutoCAD
	if(numPfc) acutPrintf(L"\nВыполнил преобразование многогранной сети в сеть %d из %d", numConv, numPfc);
	else acutPrintf(L"\nВ наборе нет примитивов многогранная сеть");
}
