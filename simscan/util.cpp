
#include "StdAfx.h"
#include <dbSubD.h>

#include "util.h"

#define TSNOPT L"General"

//-----------------------------------------------------------------------------
Acad::ErrorStatus util::AddEntityToModelSpace(AcDbObjectId& objId, AcDbEntity* pEntity,
	Adesk::Boolean isClose, AcDbDatabase* pDb)
{
	Acad::ErrorStatus     es = Acad::eOk;
	AcDbBlockTable* pBlockTable;
	AcDbBlockTableRecord* pSpaceRecord;

	if (pDb == NULL) pDb = acdbCurDwg();
	pDb->getSymbolTable(pBlockTable, AcDb::kForRead);

	pBlockTable->getAt(ACDB_MODEL_SPACE, pSpaceRecord, AcDb::kForWrite);

	es = pSpaceRecord->appendAcDbEntity(objId, pEntity);
	pSpaceRecord->close();
	pBlockTable->close();

	if (es == Acad::eOk)
	{
		if (isClose == Adesk::kTrue) pEntity->close();
	}
	else delete pEntity;
	return es;
}
int util::GetStatusLayerEntity(const AcDbObjectId& entId)
{
	AcDbEntity* pEntity = NULL;
	if (acdbOpenObject(pEntity, entId, AcDb::kForRead) == Acad::eOk)
	{
		int result = util::GetStatusLayerEntity(pEntity);
		pEntity->close();
		return result;
	}
	return EL_ERROR;
}

int util::GetStatusLayerEntity(AcDbEntity* pEntity)
{
	AcDbLayerTableRecord* pLayer = NULL;
	if (pEntity && (acdbOpenObject(pLayer, pEntity->layerId(), AcDb::kForRead) == Acad::eOk))
	{
		int result = 0;
		if (pLayer->isLocked())	result |= EL_IS_LOCKED;
		if (pLayer->isOff())		result |= EL_IS_OFF;
		if (pLayer->isFrozen())	result |= EL_IS_FROZEN;
		pLayer->close();
		return result;
	}
	return EL_ERROR;
}

int util::GetMeshFromDb(AcDbObjectIdArray& ids, AcDbDatabase* pDb)
{
	ids.setLogicalLength(0);
	if (pDb == nullptr) pDb = acdbCurDwg();

	AcDbBlockTable* pBlockTable;
	pDb->getSymbolTable(pBlockTable, AcDb::kForRead);
	AcDbBlockTableRecord* pSpaceRecord;
	pBlockTable->getAt(ACDB_MODEL_SPACE, pSpaceRecord, AcDb::kForRead);
	pBlockTable->close();
	AcDbBlockTableRecordIterator* pIterator = nullptr;
	Acad::ErrorStatus es = pSpaceRecord->newIterator(pIterator);
	pSpaceRecord->close();
	if (es != Acad::eOk) return 0;//Ошибка создания итератора

	AcDbEntity* pEntity; 
	for (pIterator->start(); !pIterator->done(); pIterator->step()) {
		if (pIterator->getEntity(pEntity, AcDb::kForRead) == Acad::eOk)
		{
			if (pEntity->isKindOf(AcDbSubDMesh::desc()) &&
				(util::GetStatusLayerEntity(pEntity) <= EL_IS_OFF)) ids.append(pEntity->objectId());
			pEntity->close();
		}
	}
	delete pIterator;
	return ids.length();
}

AcGeCurve3d* util::convertDbCurveToGeCurve3d(AcDbCurve* pDbCurve)
{
	AcGeCurve3d* pGeCurve = NULL;
	// for Line: is very simple;
	if (pDbCurve->isKindOf(AcDbLine::desc()))
	{
		AcDbLine* pL = AcDbLine::cast(pDbCurve);
		AcGeLineSeg3d* pGL = new AcGeLineSeg3d;
		pGL->set(pL->startPoint(), pL->endPoint());
		pGeCurve = (AcGeCurve3d*)pGL;
	}
	// for Arc;
	else if (pDbCurve->isKindOf(AcDbArc::desc()))
	{
		AcDbArc* pArc = AcDbArc::cast(pDbCurve);
		double ans, ane;
		ans = pArc->startAngle();
		ane = pArc->endAngle();
		AcGeCircArc3d* pGArc = new AcGeCircArc3d;
		pGArc->setCenter(pArc->center());
		pGArc->setRadius(pArc->radius());
		pGArc->setAngles(ans, ane);
		pGeCurve = (AcGeCurve3d*)pGArc;
	}
	// for Circle;
	else if (pDbCurve->isKindOf(AcDbCircle::desc()))
	{
		AcDbCircle* pCir = AcDbCircle::cast(pDbCurve);
		AcGeCircArc3d* pGCir = new AcGeCircArc3d;
		pGCir->setCenter(pCir->center());
		pGCir->setRadius(pCir->radius());
		pGeCurve = (AcGeCurve3d*)pGCir;
	}
	//for Ellipse
	else if (pDbCurve->isKindOf(AcDbEllipse::desc()))
	{
		AcDbEllipse* pEli = AcDbEllipse::cast(pDbCurve);
		AcGePoint3d center = pEli->center();
		AcGeVector3d pv1 = pEli->majorAxis();
		double majorRadius = sqrt(pv1.x * pv1.x + pv1.y * pv1.y + pv1.z * pv1.z);
		double param1, param2;
		pEli->getStartParam(param1);
		pEli->getEndParam(param2);

		AcGeEllipArc3d* pGEli = new AcGeEllipArc3d;
		pGEli->setCenter(center);
		pGEli->setAxes(pv1, pEli->minorAxis());
		pGEli->setMajorRadius(majorRadius);
		pGEli->setMinorRadius(majorRadius * (pEli->radiusRatio()));
		pGEli->setAngles(param1, param2);
		pGeCurve = (AcGeCurve3d*)pGEli;
	}
	//for Spline
	else if (pDbCurve->isKindOf(AcDbSpline::desc()))
	{
		AcDbSpline* pSL = AcDbSpline::cast(pDbCurve);
		if (!pSL || pSL->isNull())	return NULL;

		int degree;
		Adesk::Boolean rational;
		Adesk::Boolean closed;
		Adesk::Boolean periodic;
		AcGePoint3dArray controlPoints;
		AcGeDoubleArray knots;
		AcGeDoubleArray weights;
		double controlPtTol;
		double knotTol;
		AcGeTol tol;
		Acad::ErrorStatus es;
		es = pSL->getNurbsData(degree, rational, closed, periodic, controlPoints, knots, weights, controlPtTol, knotTol);

		if (es != Acad::eOk) return NULL;

		if (rational == Adesk::kTrue)
		{
			AcGeNurbCurve3d* pNurb = new AcGeNurbCurve3d(degree, knots, controlPoints, weights, periodic);
			pGeCurve = (AcGeCurve3d*)pNurb;
		}
		else
		{
			AcGeNurbCurve3d* pNurb = new AcGeNurbCurve3d(degree, knots, controlPoints, periodic);
			pGeCurve = (AcGeCurve3d*)pNurb;
		}
	}
	// for Polyline
	else if (pDbCurve->isKindOf(AcDbPolyline::desc()))
	{
		AcDbPolyline* pPoly = AcDbPolyline::cast(pDbCurve);
		AcGeLineSeg3d line;
		AcGeCircArc3d arc;
		AcGeVoidPointerArray geCurves;
		int nSegs = pPoly->numVerts() - 1;
		if (pPoly->isClosed()) ++nSegs;
		for (int i = 0; i < nSegs; i++)
		{
			if (pPoly->segType(i) == AcDbPolyline::kLine)
			{
				pPoly->getLineSegAt(i, line);
				geCurves.append(new AcGeLineSeg3d(line));
			}
			else if (pPoly->segType(i) == AcDbPolyline::kArc)
			{
				pPoly->getArcSegAt(i, arc);
				geCurves.append(new AcGeCircArc3d(arc));
			}
		}
		if (geCurves.length() == 1)
		{
			pGeCurve = (AcGeCurve3d*)(geCurves[0]);
		}
		else
		{
			pGeCurve = new AcGeCompositeCurve3d(geCurves);
		}
	}
	// for Polyline 3d
	else if (pDbCurve->isKindOf(AcDb3dPolyline::desc()))
	{
		//int err = 0;
		AcDb3dPolyline* pPoly3d = AcDb3dPolyline::cast(pDbCurve);
		AcGePoint3dArray arrayPnts;
		AcDb3dPolylineVertex* vrtx3;
		AcDbObjectIterator* iter = pPoly3d->vertexIterator();
		for (iter->start(); !iter->done(); iter->step())
		{
			Acad::ErrorStatus es = pPoly3d->openVertex(vrtx3, iter->objectId(), AcDb::kForRead);
			if (es == Acad::eOk)
			{
				arrayPnts.append(vrtx3->position());
				vrtx3->close();
			} //else err++;
		}
		delete iter;
		if (arrayPnts.length() < 2) return pGeCurve;
		int nSegs;
		AcGeLineSeg3d* pLine;
		AcGeVoidPointerArray geCurves;
		if (pPoly3d->isClosed())
		{
			nSegs = arrayPnts.length();
			arrayPnts.append(arrayPnts.first());
		}
		else
		{
			nSegs = arrayPnts.length() - 1;
		}
		for (int i = 0; i < nSegs; i++)
		{
			pLine = new AcGeLineSeg3d;
			pLine->set(arrayPnts[i], arrayPnts[i + 1]);
			geCurves.append(pLine);
		}

		pGeCurve = (geCurves.length() == 1) ? (AcGeCurve3d*)(geCurves[0]) : new AcGeCompositeCurve3d(geCurves);
	}
	return pGeCurve;
}

bool util::GetPoint3dArrayFromCurve(AcDbCurve* pCurve, AcGePoint3dArray& pArray, Adesk::Boolean& clos)
{
	if (pCurve == NULL) return false;

	// Если 3М полилиния типа AcDb3dPolyline
	if (pCurve->isKindOf(AcDb3dPolyline::desc())) {
		AcDb3dPolyline* pline3d = AcDb3dPolyline::cast(pCurve);
		clos = pline3d->isClosed();
		AcDb3dPolylineVertex* vrtx3;
		AcDbObjectIterator* iter = pline3d->vertexIterator();
		for (iter->start(); !iter->done(); iter->step()) {
			acdbOpenObject(vrtx3, iter->objectId(), AcDb::kForRead);
			pArray.append(vrtx3->position());
			vrtx3->close();
		}
		delete iter;
	}
	// Если отрезок AcDbLine (LINE)
	else if (pCurve->isKindOf(AcDbLine::desc())) {
		AcDbLine* line = AcDbLine::cast(pCurve);
		clos = Adesk::kFalse;
		pArray.append(line->startPoint());
		pArray.append(line->endPoint());
	}
	// Если 2М полилиния типа AcDb2dPolyline
	else if (pCurve->isKindOf(AcDb2dPolyline::desc())) {
		AcDb2dPolyline* pline2d = AcDb2dPolyline::cast(pCurve);
		clos = pline2d->isClosed();
		AcDb2dVertex* vrtx2;
		AcDbObjectIterator* iter = pline2d->vertexIterator();
		for (iter->start(); !iter->done(); iter->step()) {
			acdbOpenObject(vrtx2, iter->objectId(), AcDb::kForRead);
			pArray.append(pline2d->vertexPosition(*vrtx2));
			vrtx2->close();
		}
		delete iter;
	}
	// Если 2М полилиния типа AcDbPolyline (LWPOLYLINE)
	else if (pCurve->isKindOf(AcDbPolyline::desc())) {
		AcDbPolyline* plineLw = AcDbPolyline::cast(pCurve);
		clos = plineLw->isClosed();
		AcGePoint3d v;
		for (unsigned int i = 0; i < plineLw->numVerts(); i++) {
			plineLw->getPointAt(i, v);
			pArray.append(v);
		}
	}
	// Если сплайн AcDbSpline (SPLINE)
	else if (pCurve->isKindOf(AcDbSpline::desc())) {
		AcDbSpline* spline = AcDbSpline::cast(pCurve);
		AcGePoint3d v;
		clos = spline->isClosed();
		for (int i = 0; i < spline->numFitPoints(); i++) {
			spline->getFitPointAt(i, v);
			pArray.append(v);
		}
	}

	//Если конченая и начальные точки совпадают, то удалить последнюю
	// точку и сделать полилинию замкнутой
	if (pArray[0].isEqualTo(pArray[pArray.length() - 1])) {
		pArray.removeLast();
		clos = Adesk::kTrue;
	}
	//Удалить дублированные точки из полилинии
	int i, k;
	for (i = 1, k = 0; i < pArray.length(); i++) {
		if (pArray[i].isEqualTo(pArray[k])) continue;
		else k++;
		if (i != k) pArray[k] = pArray[i];
	}
	k++; while (k < pArray.length()) pArray.removeLast();
	return (pArray.length() > 1);
}

int util::GetDbCurvePoints(AcGePoint3dArray& pnt, AcDbCurve* pDbCurve, double approxEps)
{
	pnt.setLogicalLength(0);

	if (pDbCurve->isKindOf(AcDbLine::desc()))
	{
		AcDbLine* pL = AcDbLine::cast(pDbCurve);
		pnt.append(pL->startPoint());
		pnt.append(pL->endPoint());
	}
	// for Polyline 3d
	else if (pDbCurve->isKindOf(AcDb3dPolyline::desc()))
	{
		AcDb3dPolyline* pPoly3d = AcDb3dPolyline::cast(pDbCurve);
		//AcGePoint3dArray arrayPnts;
		AcDb3dPolylineVertex* vrtx3;
		AcDbObjectIterator* iter = pPoly3d->vertexIterator();
		for (iter->start(); !iter->done(); iter->step()) {
			acdbOpenObject(vrtx3, iter->objectId(), AcDb::kForRead);
			pnt.append(vrtx3->position());
			vrtx3->close();
		}
		delete iter;

		if (pPoly3d->isClosed()) pnt.append(pnt.first());
	}
	//for Spline
	else if (pDbCurve->isKindOf(AcDbSpline::desc()))
	{
		double startParam, endParam;
		AcGeDoubleArray  params;
		AcDbSpline* pSL = AcDbSpline::cast(pDbCurve);
		if (!pSL || pSL->isNull())	return NULL;
		pSL->getStartParam(startParam);
		pSL->getEndParam(endParam);

		int degree;
		Adesk::Boolean rational;
		Adesk::Boolean closed;
		Adesk::Boolean periodic;
		AcGePoint3dArray controlPoints;
		AcGeDoubleArray knots;
		AcGeDoubleArray weights;
		double controlPtTol;
		double knotTol;
		AcGeTol tol;
		Acad::ErrorStatus es;
		es = pSL->getNurbsData(degree, rational, closed, periodic, controlPoints, knots, weights, controlPtTol, knotTol);

		if (es != Acad::eOk) return NULL;
		AcGeNurbCurve3d* pNurb = (rational == Adesk::kTrue) ?
			new AcGeNurbCurve3d(degree, knots, controlPoints, weights, periodic) :
			new AcGeNurbCurve3d(degree, knots, controlPoints, periodic);
		pNurb->getSamplePoints(startParam, endParam, approxEps, pnt, params);
		delete pNurb;
		if (pSL->isClosed() && !pnt.first().isEqualTo(pnt.last())) pnt.append(pnt.first());
	}
	// for Polyline
	else if (pDbCurve->isKindOf(AcDbPolyline::desc()))
	{
		AcGeDoubleArray  params;
		AcGeCircArc3d arc;
		AcGePoint3dArray pointArray;
		AcGePoint3d p;
		AcDbPolyline* pPoly = AcDbPolyline::cast(pDbCurve);
		int numV = (int)pPoly->numVerts() - 1;
		for (int i = 0; i < numV; ++i)
		{
			if (pPoly->segType(i) == AcDbPolyline::kArc)
			{
				pPoly->getArcSegAt(i, arc);
				arc.getSamplePoints(0.0, arc.endAng(), approxEps, pointArray, params);
				int nt = pointArray.length();
				if (nt > 0) { pointArray.setLogicalLength(nt - 1); pnt.append(pointArray); }
			}
			else {
				pPoly->getPointAt(i, p);
				pnt.append(p);
			}
		}
		if (pPoly->isClosed())
		{
			if (pPoly->segType(numV) == AcDbPolyline::kArc)
			{
				pPoly->getArcSegAt(numV, arc);
				arc.getSamplePoints(0.0, arc.endAng(), approxEps, pointArray, params);
				pnt.append(pointArray);
			}
			else {
				pPoly->getPointAt(numV, p);
				pnt.append(p);
				pnt.append(pnt.first());
			}
		}
		else {
			pPoly->getPointAt(numV, p);
			pnt.append(p);
		}
	}
	// for 2d Polyline
	else if (pDbCurve->isKindOf(AcDb2dPolyline::desc()))
	{
		AcGeCircArc3d arc;
		AcGePoint3dArray pointArray;
		AcGePoint3d p;
		AcDb2dPolyline* pline2d = AcDb2dPolyline::cast(pDbCurve);

		AcDb2dVertex* vrtx2;
		AcDbObjectIterator* iter = pline2d->vertexIterator();
		for (iter->start(); !iter->done(); iter->step()) {
			acdbOpenObject(vrtx2, iter->objectId(), AcDb::kForRead);
			pnt.append(pline2d->vertexPosition(*vrtx2));
			vrtx2->close();
		}
		delete iter;
		if (pline2d->isClosed()) pnt.append(pnt.first());
	}
	// for Arc;
	else if (pDbCurve->isKindOf(AcDbArc::desc()))
	{
		AcGeDoubleArray  params;
		AcDbArc* pArc = AcDbArc::cast(pDbCurve);
		double ans, ane;
		ans = pArc->startAngle();
		ane = pArc->endAngle();
		AcGeCircArc3d* pGArc = new AcGeCircArc3d;
		pGArc->setCenter(pArc->center());
		pGArc->setRadius(pArc->radius());
		pGArc->setAngles(ans, ane);
		pGArc->getSamplePoints(ans, ane, approxEps, pnt, params);
		delete pGArc;
	}
	// for Circle;
	else if (pDbCurve->isKindOf(AcDbCircle::desc()))
	{
		double startParam, endParam;
		AcGeDoubleArray  params;
		AcDbCircle* pCir = AcDbCircle::cast(pDbCurve);
		pCir->getStartParam(startParam);
		pCir->getEndParam(endParam);
		AcGeCircArc3d* pGCir = new AcGeCircArc3d;
		pGCir->setCenter(pCir->center());
		pGCir->setRadius(pCir->radius());
		pGCir->getSamplePoints(startParam, endParam, approxEps, pnt, params);
		delete pGCir;
		if (!pnt.first().isEqualTo(pnt.last())) pnt.append(pnt.first());
	}
	//for Ellipse
	else if (pDbCurve->isKindOf(AcDbEllipse::desc()))
	{
		double startParam, endParam;
		AcGeDoubleArray  params;
		AcDbEllipse* pEli = AcDbEllipse::cast(pDbCurve);
		pEli->getStartParam(startParam);
		pEli->getEndParam(endParam);
		AcGePoint3d center = pEli->center();
		AcGeVector3d pv1 = pEli->majorAxis();
		double majorRadius = sqrt(pv1.x * pv1.x + pv1.y * pv1.y + pv1.z * pv1.z);

		AcGeEllipArc3d* pGEli = new AcGeEllipArc3d;
		pGEli->setCenter(center);
		pGEli->setAxes(pv1, pEli->minorAxis());
		pGEli->setMajorRadius(majorRadius);
		pGEli->setMinorRadius(majorRadius * (pEli->radiusRatio()));
		pGEli->setAngles(pEli->startAngle(), pEli->endAngle());
		pGEli->getSamplePoints(startParam, endParam, approxEps, pnt, params);
		delete pGEli;
		if (!pnt.first().isEqualTo(pnt.last())) pnt.append(pnt.first());
	}
	return pnt.length();
}

// Функция установки текущего слоя
// layer - (входной) имя слоя.
// Устанавливает слой layer текущим. Если слоя layer нет, то создает его и делает текущим.
AcDbObjectId util::SetCurLayer(const ACHAR* layer, Adesk::UInt16* colorIndex, AcDbDatabase* pDb)
{
	if (_tcslen(layer) < 1) return AcDbObjectId::kNull;
	AcDbLayerTableRecord* pRecord;
	AcDbLayerTable* pLayerTable;
	AcDbObjectId recordId;
	if (pDb == NULL) pDb = acdbCurDwg();
	pDb->getSymbolTable(pLayerTable, AcDb::kForWrite);
	if (!pLayerTable->has(layer)) {
		pRecord = new AcDbLayerTableRecord;
		pRecord->setName(layer);
		if (colorIndex)
		{
			AcCmColor color;
			color.setColorIndex(*colorIndex);
			pRecord->setColor(color);
		}
		pLayerTable->add(recordId, pRecord);
		pRecord->close();
	}
	else
		pLayerTable->getAt(layer, recordId);
	pLayerTable->close();
	pDb->setClayer(recordId);
	return recordId;
}

void util::getProfile(LPTSTR buf, int ccMax)
{
	TCHAR dir[_MAX_DIR], drive[_MAX_DRIVE];
	_tsplitpath_s(acedGetAppName(), drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, 0, NULL, 0);
	_tmakepath_s(buf, ccMax, drive, dir, L"simopt", L".inf");
}

double util::fromProfile(double def, LPCTSTR key, LPCTSTR app)
{
	TCHAR profFile[_MAX_PATH];
	util::getProfile(profFile, _MAX_PATH);

	TCHAR buf[64], sdef[64];
	_stprintf_s(sdef, _countof(sdef), _T("%g"), def);
	if (GetPrivateProfileString(app ? app : TSNOPT, key, sdef, buf, _countof(buf), profFile) < 1)
		_tcscpy_s(buf, _countof(buf), sdef);
	return _tstof(buf);
}
int util::fromProfile(int def, LPCTSTR key, LPCTSTR app)
{
	TCHAR profFile[_MAX_PATH];
	util::getProfile(profFile, _MAX_PATH);

	return (int)GetPrivateProfileInt(app ? app : TSNOPT, key, (INT)def, profFile);
}

__int64 util::fromProfile(__int64 def, LPCTSTR key, LPCTSTR app)
{
	TCHAR profFile[_MAX_PATH];
	util::getProfile(profFile, _MAX_PATH);

	TCHAR buf[64], sdef[64];
	_stprintf_s(sdef, _countof(sdef), _T("%I64u"), def);
	if (GetPrivateProfileString(app ? app : TSNOPT, key, sdef, buf, _countof(buf), profFile) < 1)
		_tcscpy_s(buf, _countof(buf), sdef);

	return _tcstoui64(buf, NULL, 10);
}

int util::fromProfile(AcGeIntArray& arr, LPCTSTR key, LPCTSTR app)
{
	arr.setLogicalLength(0);

	TCHAR buff[2048];
	util::fromProfile((LPTSTR)buff, _countof(buff), _T(""), key, app);
	TCHAR* next_token = NULL, * token = _tcstok_s(buff, _T(","), &next_token);
	while (token != NULL)
	{
		arr.append(_tstoi(token));
		token = _tcstok_s(NULL, _T(","), &next_token);
	}
	return arr.length();
}

COLORREF util::fromProfileColor(COLORREF def, LPCTSTR key, LPCTSTR app)
{
	AcGeIntArray arr;
	int n = util::fromProfile(arr, key, app);
	if (n == 3) return RGB(arr[0], arr[1], arr[2]);
	return def;
}


int util::fromProfile(CListCtrl& lst, LPCTSTR key, LPCTSTR app)
{
	AcGeIntArray aw;
	CHeaderCtrl* pHeaderCtrl = lst.GetHeaderCtrl();
	int m = pHeaderCtrl->GetItemCount(), n = util::fromProfile(aw, key, app);
	if (n != m)
	{
		CRect rc; lst.GetClientRect(&rc);
		aw.setLogicalLength(m);
		aw.setAll((int)(rc.Width() / ((double)m)));
		return 0;
	}
	for (int i = 0; i < m; i++) lst.SetColumnWidth(i, aw[i]);
	return m;
}


LPCTSTR util::fromProfile(LPTSTR buf, int ccMax, LPCTSTR def, LPCTSTR key, LPCTSTR app)
{
	TCHAR profFile[_MAX_PATH];
	util::getProfile(profFile, _MAX_PATH);

	if (GetPrivateProfileString(app ? app : TSNOPT, key, def, buf, ccMax, profFile) < 1) _tcscpy_s(buf, ccMax, def);
	return (LPCTSTR)buf;
}

BOOL util::toProfile(double val, LPCTSTR key, LPCTSTR app)
{
	TCHAR profFile[_MAX_PATH];
	util::getProfile(profFile, _MAX_PATH);

	TCHAR buf[64];
	_stprintf_s(buf, _countof(buf), _T("%g"), val);
	return WritePrivateProfileString(app ? app : TSNOPT, key, buf, profFile);
}

BOOL util::toProfile(int val, LPCTSTR key, LPCTSTR app)
{
	TCHAR profFile[_MAX_PATH];
	util::getProfile(profFile, _MAX_PATH);

	TCHAR buf[64];
	_stprintf_s(buf, _countof(buf), _T("%d"), val);
	return WritePrivateProfileString(app ? app : TSNOPT, key, buf, profFile);
}

BOOL util::toProfile(__int64 val, LPCTSTR key, LPCTSTR app)
{
	TCHAR profFile[_MAX_PATH];
	util::getProfile(profFile, _MAX_PATH);

	TCHAR buf[64];
	_stprintf_s(buf, _countof(buf), _T("%I64u"), val);
	return WritePrivateProfileString(app ? app : TSNOPT, key, buf, profFile);
}


BOOL util::toProfile(LPCTSTR str, LPCTSTR key, LPCTSTR app)
{
	TCHAR profFile[_MAX_PATH];
	util::getProfile(profFile, _MAX_PATH);

	return WritePrivateProfileString(app ? app : TSNOPT, key, str, profFile);
}

BOOL util::toProfile(AcGeIntArray& arr, LPCTSTR key, LPCTSTR app)
{
	TCHAR buff[2048]; buff[0] = _T('\0');
	int n = arr.length();
	for (int i = 0; i < n; i++)
	{
		TCHAR vol[32];
		_stprintf_s(vol, 32, _T("%d"), arr[i]);
		if (i) _tcscat_s(buff, _countof(buff), _T(","));
		_tcscat_s(buff, _countof(buff), vol);
	}
	TCHAR profFile[_MAX_PATH];
	util::getProfile(profFile, _MAX_PATH);
	return WritePrivateProfileString(app ? app : TSNOPT, key, buff, profFile);
}

BOOL util::toProfileColor(COLORREF rgb, LPCTSTR key, LPCTSTR app)
{
	AcGeIntArray arr;
	arr.setLogicalLength(3);
	arr[0] = GetRValue(rgb);
	arr[1] = GetGValue(rgb);
	arr[2] = GetBValue(rgb);
	return util::toProfile(arr, key, app);
}


int util::toProfile(CListCtrl& lst, LPCTSTR key, LPCTSTR app)
{
	AcGeIntArray aw;
	CHeaderCtrl* pHeaderCtrl = lst.GetHeaderCtrl();
	int m = pHeaderCtrl->GetItemCount();
	aw.setLogicalLength(m);
	for (int i = 0; i < m; i++)
	{
		CRect rc;
		pHeaderCtrl->GetItemRect(i, &rc);
		aw[i] = rc.Width();
	}
	return util::toProfile(aw, key, app);
}
