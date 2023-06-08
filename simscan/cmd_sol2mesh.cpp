#include "StdAfx.h"
//////////////////////////////////////
#include <dbents.h>
#include <dbpl.h>
#include <math.h>
#include "axlock.h"
#include <dblayout.h>
#include <dbSubD.h>

#include <dbsurf.h>
#include <dbextrudedsurf.h>
#include <dbloftedsurf.h>
#include <dbplanesurf.h>
#include <dbrevolvedsurf.h>
#include <dbsweptsurf.h>

#include <brbrep.h>
#include <brent.h>
#include <brface.h>
#include <bredge.h>
#include <brm2dctl.h>
#include <brmesh2d.h>
#include <brmetrav.h>
#include <brentrav.h>
#include <brnode.h>
#include <brbftrav.h>

#include <brbctrav.h>
#include <brbstrav.h>
#include <brbetrav.h>
#include <brbvtrav.h>

#include <set>

#include "resource.h"
#include <dbSubD.h>
#include "util.h"
#include "simsc.h"
#include "StdAfx.h"



#include "axlock.h"

#include "DocData.h"
#include "Resource.h"


#ifndef UNUSED32
#define UNUSED32 0xffffffff
#endif

class CDlgSol2Mesh;
CDlgSol2Mesh* gpDlgSol2Mesh = NULL;

class CDlgSol2Mesh : public CDialog
{
	//DECLARE_DYNAMIC(CDlgSol2Mesh)
public:
	CDlgSol2Mesh(CWnd* pParent = NULL) : CDialog(CDlgSol2Mesh::IDD, pParent) {}
	// Dialog Data
	enum { IDD = IDD_DIALOG1 };

	void SaveParam()
	{
		CString str;
		GetDlgItem(IDC_EDIT1)->GetWindowText(str);	util::toProfile((int)_tstol(str), L"MaxSubdivisions", L"Solid2Mesh");
		GetDlgItem(IDC_EDIT2)->GetWindowText(str);	util::toProfile((double)_tstof(str), L"MaxNodeSpacing", L"Solid2Mesh");
		GetDlgItem(IDC_EDIT3)->GetWindowText(str);	util::toProfile((double)(0.0174563292519943295 * _tstof(str)), L"AngTol", L"Solid2Mesh");
		GetDlgItem(IDC_EDIT4)->GetWindowText(str);	util::toProfile((double)_tstof(str), L"DistTol", L"Solid2Mesh");
		GetDlgItem(IDC_EDIT5)->GetWindowText(str);	util::toProfile((double)_tstof(str), L"MaxAspectRatio", L"Solid2Mesh");
		util::toProfile((int)(IsDlgButtonChecked(IDC_CHECK1) ? TRUE : FALSE), L"IsDeleteSourceSol2Mesh", L"Solid2Mesh");
	}

	static BOOL startDlg()
	{
		BOOL b = TRUE;
		if (!gpDlgSol2Mesh)
		{
			CAcModuleResourceOverride resOverride;
			gpDlgSol2Mesh = new CDlgSol2Mesh(acedGetAcadFrame());
			b = gpDlgSol2Mesh->Create(IDD_DIALOG1);
			acedGetAcadDwgView()->SetFocus();
		}
		return b;
	}
	static BOOL endDlg()
	{
		if (!gpDlgSol2Mesh) return TRUE;
		gpDlgSol2Mesh->SaveParam();
		BOOL b = gpDlgSol2Mesh->DestroyWindow();
		if (b) gpDlgSol2Mesh = NULL;
		return b;
	}
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnClose() { SaveParam(); CDialog::OnClose(); endDlg(); }
	afx_msg LRESULT onAcadKeepFocus(WPARAM, LPARAM) { return TRUE; }
	virtual BOOL OnInitDialog()
	{
		CDialog::OnInitDialog();
		CString str;
		str.Format(_T("%d"), (long)util::fromProfile((int)0, _T("MaxSubdivisions"), _T("Solid2Mesh")));			GetDlgItem(IDC_EDIT1)->SetWindowText((LPCTSTR)str);
		str.Format(_T("%.3f"), util::fromProfile((double)0.0, _T("MaxNodeSpacing"), _T("Solid2Mesh")));			GetDlgItem(IDC_EDIT2)->SetWindowText((LPCTSTR)str);
		str.Format(_T("%.3f"), 57.2957795130823 * util::fromProfile((double)5.0, _T("AngTol"), _T("Solid2Mesh"))); GetDlgItem(IDC_EDIT3)->SetWindowText((LPCTSTR)str);
		str.Format(_T("%.3f"), util::fromProfile((double)0.1, _T("DistTol"), _T("Solid2Mesh")));					GetDlgItem(IDC_EDIT4)->SetWindowText((LPCTSTR)str);
		str.Format(_T("%.3f"), util::fromProfile((double)0.0, _T("MaxAspectRatio"), _T("Solid2Mesh")));			GetDlgItem(IDC_EDIT5)->SetWindowText((LPCTSTR)str);
		CheckDlgButton(IDC_CHECK1, util::fromProfile((int)0, L"IsDeleteSourceSol2Mesh", L"Solid2Mesh") ? BST_CHECKED : BST_UNCHECKED);
		return TRUE;  // return TRUE unless you set the focus to a control EXCEPTION: OCX Property Pages should return FALSE
	}
	DECLARE_MESSAGE_MAP()
};
//IMPLEMENT_DYNAMIC(CDlgSol2Mesh, CDialog)

#ifndef WM_ACAD_MFC_BASE
#define WM_ACAD_MFC_BASE        (1000)
#endif

#ifndef WM_ACAD_KEEPFOCUS
#define WM_ACAD_KEEPFOCUS       (WM_ACAD_MFC_BASE + 1)
#endif

BEGIN_MESSAGE_MAP(CDlgSol2Mesh, CDialog)
	ON_WM_CLOSE()
	ON_MESSAGE(WM_ACAD_KEEPFOCUS, &CDlgSol2Mesh::onAcadKeepFocus)
END_MESSAGE_MAP()

void CDlgSol2Mesh::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}
//#define USESET


AcBr::ErrorStatus addFaces(AcBrFace& face, AcBrMesh2dControl& meshCtrl, AcGePoint3dArray& vertexArray, AcArray<Adesk::Int32>& faceArray, const AcGeTol& tol = AcGeContext::gTol)
{
	AcBr::ErrorStatus ret = AcBr::eOk;
	// создать фильтр для сети из топологии примитива и примитива для контроля сети
	AcBrMesh2dFilter meshFilter;
	AcBrEntity* pBrEntity = (AcBrEntity*)&face;
	meshFilter.insert(make_pair((const AcBrEntity*&)pBrEntity, (const AcBrMesh2dControl)meshCtrl));
	//------------------------------------------------------------------------------
	AcBrMesh2d mesh;
	if (ret = mesh.generate(meshFilter)) return ret; // Генерирую сеть
	AcBrEntity* meshOwner = NULL;
	if (ret = mesh.getEntityAssociated(meshOwner)) return ret;// Проверяю
	// Сделать глобальный проводник по элементам граниям
	AcBrMesh2dElement2dTraverser meshElemTrav;
	if (ret = meshElemTrav.setMesh(mesh)) return ret;
	//пройти все елементы
	for (; (!meshElemTrav.done() && !ret); ret = meshElemTrav.next())
	{
		AcBrElement2dNodeTraverser faceTrav;
		if (ret = faceTrav.setElement(meshElemTrav)) return ret;

		
		faceArray.append(3);
		

		for (int kf = 0; ((kf < 3) && !faceTrav.done() && !ret); ++kf, ret = faceTrav.next())
		{
			AcBrNode node; if (ret = faceTrav.getNode(node)) return ret;
			// Добавить точку d массив
			AcGePoint3d p;
			if (ret = node.getPoint(p)) return ret;
			int nv = vertexArray.length(), id = nv;
			for (int i = 0; i < nv; i++) if (vertexArray[i].isEqualTo(p, tol)) { id = i; break; }
			if (id == nv) vertexArray.append(p);
			faceArray.append(id);
		} // конец просмотра координат грани
	}
	return AcBr::eOk;
}

AcBr::ErrorStatus solidToSubDMesh(const AcDbObjectId& objId, AcBrMesh2dControl& meshCtrl, AcGePoint3dArray& vertexArray, AcArray<Adesk::Int32>& faceArray, const AcGeTol& tol = AcGeContext::gTol)
{
	//return AcBr::eOk;
	AcBr::ErrorStatus ret = AcBr::eOk;
	AcBrBrep brep;
	AcDbFullSubentPath subPath(objId, AcDb::kNullSubentType, 0);

	// Установка идентификатора объекта через 
	if (ret = brep.set(subPath)) return ret;
	if (ret = brep.setValidationLevel(AcBr::kFullValidation)) return ret;//AcBr::kNoValidation задать уровень контроля

	// Создать глобальный проводник по граням
	AcBrBrepFaceTraverser brepFaceTrav;
	if (ret = brepFaceTrav.setBrep(brep))
	{
		if (ret == AcBr::eDegenerateTopology)
		{
			AcBrFace face;
			face.set(subPath);// Установка идентификатора объекта через 
			face.setValidationLevel(AcBr::kFullValidation);//AcBr::kNoValidation задать уровень контроля
			return addFaces(face, meshCtrl, vertexArray, faceArray, tol);
		}
		return ret;
	}

	int faceCount = 0; // количество граней
	
	while (!brepFaceTrav.done() && (ret == AcBr::eOk)) {
		AcBrFace face;
		brepFaceTrav.getFace(face);

		if (ret = addFaces(face, meshCtrl, vertexArray, faceArray, tol)) return ret;

		faceCount++;
		if (ret = brepFaceTrav.next()) return ret;
	}
	return ret;
}

// This is command 'SOL2MSH'
void CsimscanApp::Houston_sol2msh(void) //Конверировать солид в сеть
{
	TCHAR dir[_MAX_DIR], drive[_MAX_DRIVE], path[_MAX_PATH];
	_tsplitpath_s(acedGetAppName(), drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, 0, NULL, 0);
	_tmakepath_s(path, _MAX_PATH, drive, dir, L"simopt", L".inf");
	acutPrintf(_T("\nОтладка [%s]"), path);

	class CommandDialog
	{
	public:
		CommandDialog() { CDlgSol2Mesh::startDlg(); }
		~CommandDialog() { CDlgSol2Mesh::endDlg(); }
	} copt;

	ads_name ssObj, entName;
	//Ввод данных---------------------------------------------
	acutPrintf(_T("\nВыберите объекты для преобразования..."));
	if (acedSSGet(NULL, NULL, NULL, NULL, ssObj) != RTNORM) return;

	if (gpDlgSol2Mesh) gpDlgSol2Mesh->SaveParam();


	//--------------------------------------------------------------------------------------------------
	Adesk::Int32 numEnt;//Длина набора примитивов
	acedSSLength(ssObj, &numEnt);	//Считать длину набора примитивов
	if (!numEnt) { acedSSFree(ssObj); acutPrintf(_T("\nНичего не выбрано ...")); return; }
	int  solid = 0, body = 0, region = 0, surface = 0;
	AcDbObjectIdArray solidIds, surfaceIds;
	for (int ie = 0; ie < numEnt; ++ie)//Чтение примитивов из набора
	{
		AcDbObjectId entId;
		AcDbEntity* pEnt = NULL;
		if ((acedSSName(ssObj, ie, entName) == RTNORM) && (acdbGetObjectId(entId, entName) == Acad::eOk) &&
			(acdbOpenObject(pEnt, entId, AcDb::kForRead) == Acad::eOk))
		{
			if (pEnt->isKindOf(AcDb3dSolid::desc())) { solidIds.append(entId); solid++; }
			else if (pEnt->isKindOf(AcDbBody::desc())) { solidIds.append(entId); body++; }
			else if (pEnt->isKindOf(AcDbRegion::desc())) { solidIds.append(entId); region++; }
			else if (pEnt->isKindOf(AcDbSurface::desc())) { surfaceIds.append(entId); surface++; }
			pEnt->close();
		}
	}

	int numObj = solidIds.length() + surfaceIds.length();
	if (numObj) acutPrintf(_T("\nНайдено %d объектов для конвертации (Solid = %d, Body  = %d, Region = %d, Surface = %d)"), numObj, solid, body, region, surface);
	else acutPrintf(_T("\nНет объектов для конвертации (Solid, Body, Region)"));
	AcBrMesh2dControl brMeshCtrl;
	brMeshCtrl.setElementShape(AcBr::kAllTriangles);//Формировать треугольники
	brMeshCtrl.setMaxSubdivisions(util::fromProfile((int)0, L"MaxSubdivisions", L"Solid2Mesh"));
	brMeshCtrl.setMaxNodeSpacing(util::fromProfile((double)5.0, L"MaxNodeSpacing", L"Solid2Mesh"));
	brMeshCtrl.setAngTol(0.0174563292519943295 * util::fromProfile((double)0.0, L"AngTol", L"Solid2Mesh"));
	brMeshCtrl.setDistTol(util::fromProfile((double)0.05, L"DistTol", L"Solid2Mesh"));
	brMeshCtrl.setMaxAspectRatio(util::fromProfile((double)0.0, L"MaxAspectRatio", L"Solid2Mesh"));

	AcGeTol tol(AcGeContext::gTol);
	double eps = util::fromProfile(((double)0.001), L"AccuracyPoint", L"Solid2Mesh");
	tol.setEqualPoint(eps);
	bool isDeleteSource = util::fromProfile((int)0, L"IsDeleteSourceSol2Mesh", L"Solid2Mesh") ? true : false;

	int numSurface = 0;
	acedSetStatusBarProgressMeter(L"Конвертирую тела и сети...", 0, solidIds.length() + surfaceIds.length());
	for (int i = 0; i < solidIds.length(); i++) //Чтение тел из набора
	{
		acedSetStatusBarProgressMeterPos(i);
		AcDbEntity* pEnt = NULL;
		if (acdbOpenObject(pEnt, solidIds[i], AcDb::kForRead) != Acad::eOk) continue;
		Adesk::UInt16 color = pEnt->colorIndex();
		AcCmTransparency transparency = pEnt->transparency();
		acutPrintf(_T("\n Обрабатываю примитив '%s'"), pEnt->isA()->name());
		pEnt->close();

		AcArray<Adesk::Int32> faceArray;
		AcGePoint3dArray vertexArray;
		if (solidToSubDMesh(solidIds[i], brMeshCtrl, vertexArray, faceArray, tol) == AcBr::eOk)
		{
			int subDLevel = 0;
			AcDbObjectId cId;
			AcDbSubDMesh* pSubDMesh = new AcDbSubDMesh();
			if (pSubDMesh->setSubDMesh(vertexArray, faceArray, subDLevel) == Acad::eOk)
			{
				pSubDMesh->setColorIndex(color);
				pSubDMesh->setTransparency(transparency);
				if (util::AddEntityToModelSpace(cId, pSubDMesh) == Acad::eOk)
				{
					if (isDeleteSource)
					{
						AcDbEntity* pEnt = NULL;
						if (acdbOpenObject(pEnt, solidIds[i], AcDb::kForWrite) == Acad::eOk)
						{
							pEnt->erase();
							pEnt->close();
						}
					}
					numSurface++;
				}
			}
			else { delete pSubDMesh; /* Ошибка */ }
		}
	}
	int numSolid = solidIds.length();


	for (int i = 0; i < surfaceIds.length(); i++) //Чтение поверхностей из набора
	{
		acedSetStatusBarProgressMeterPos(i + numSolid);

		AcDbSurface* pSurface = NULL;
		if (acdbOpenObject(pSurface, surfaceIds[i], AcDb::kForRead) == Acad::eOk)
		{
			Adesk::UInt16 color = pSurface->colorIndex();
			AcCmTransparency transparency = pSurface->transparency();
			acutPrintf(_T("\n Обрабатываю примитив '%s'"), pSurface->isA()->name());
			AcDbBody* pBody = new AcDbBody();
			if (pBody->setASMBody(pSurface->ASMBodyCopy()) == Acad::eOk)
			{
				AcBrBrep* pBrep = new AcBrBrep();
				if (AcBr::eOk == pBrep->set(*pBody))
				{
					AcArray<Adesk::Int32> faceArray;
					AcGePoint3dArray vertexArray;
					AcBrBrepFaceTraverser* pFaceTrav = new AcBrBrepFaceTraverser;
					if (AcBr::eOk == pFaceTrav->setBrep(*pBrep))
					{
						for (pFaceTrav->restart(); !pFaceTrav->done(); pFaceTrav->next())
						{
							AcBrFace face;
							if (AcBr::eOk == pFaceTrav->getFace(face))
							{
								addFaces(face, brMeshCtrl, vertexArray, faceArray, tol);
							}
						}
					}
					delete pFaceTrav;
					if (faceArray.length() && vertexArray.length())
					{
						int subDLevel = 0;
						AcDbObjectId cTd;
						AcDbSubDMesh* pSubDMesh = new AcDbSubDMesh();
						if (pSubDMesh->setSubDMesh(vertexArray, faceArray, subDLevel) == Acad::eOk)
						{
							pSubDMesh->setColorIndex(color);
							pSubDMesh->setTransparency(transparency);
							if (util::AddEntityToModelSpace(cTd, pSubDMesh) == Acad::eOk)
							{
								if (isDeleteSource)
								{
									AcDbEntity* pEnt = NULL;
									if (acdbOpenObject(pEnt, surfaceIds[i], AcDb::kForWrite) == Acad::eOk)
									{
										pEnt->erase();
										pEnt->close();
									}
								}
								numSurface++;
							}
						}
						else { delete pSubDMesh; /* Ошибка */ }
					}
				}
				delete pBrep;
			}
			pSurface->close();
		}
	}
	acedRestoreStatusBar();//Удалить прогрессбар
	}