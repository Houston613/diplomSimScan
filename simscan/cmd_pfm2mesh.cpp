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

void CsimscanApp::Houston_pfm2msh(void) //������������� ������������ ���� � ����
{
	ads_name ssObj;//����� AutoCAD ��������� ��������
	acutPrintf(_T("\n�������� ������� ��� ��������������..."));//������� �����������
	//��������� ������������� ������
	if(acedSSGet(NULL, NULL, NULL, NULL, ssObj) != RTNORM)
	{
		acutPrintf(_T("\n �������� �������������..."));
		return;
	}
	Adesk::Int32 numEnt;
	acedSSLength(ssObj, &numEnt);	//������� ���������� ��������� ����������
	if (numEnt == 0)//���� ����� ������
	{
		acedSSFree(ssObj);//�������� �����
		acutPrintf(_T("\n������ �� �������..."));
		return;	
	}
	int numPfc = 0; //���������� � ������ ���������� AcDbPolyFaceMesh (������������ ����)
	int numConv = 0; //���������� ������ ���������������� ���������� � �����
	//���� �� ��������� ��������
	for (int i = 0; i < numEnt; i++) //������ ���������� �� ������ 
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
			delete iter;//������� ��������
			//������� ������ (vertex) � �������� ������ (face) ������������
			// ������ �� ��� ��������� �����
			int subDLevel = 0;
			AcDbObjectId mId;
			AcDbSubDMesh* mesh = new AcDbSubDMesh();
			if ((mesh->setSubDMesh(vertex, face, subDLevel) == Acad::eOk) &&
				(util::AddEntityToModelSpace(mId, mesh, false) == Acad::eOk))
			{
				numConv++;
				mesh->setLayer(pfmesh->layerId());//���������� ����
				mesh->setColor(pfmesh->color());//���������� ����
				mesh->setTransparency(pfmesh->transparency());//���������� ������������
				mesh->close();
				//������� �������� ������������ ����
				pfmesh->upgradeOpen();
				pfmesh->erase();
			}
			else delete mesh;
			pfmesh->close();
		}
	}
	acedSSFree(ssObj);//�������� ����� AutoCAD
	if(numPfc) acutPrintf(L"\n�������� �������������� ������������ ���� � ���� %d �� %d", numConv, numPfc);
	else acutPrintf(L"\n� ������ ��� ���������� ������������ ����");
}
