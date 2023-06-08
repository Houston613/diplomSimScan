#pragma once

//Статус слоя примиива
#define EL_IS_LOCKED	1	//Слой защищен от изменений
#define EL_IS_OFF		2	//Слой выключен
#define EL_IS_FROZEN	4	//Слой заморожен
#define EL_CANCEL		8	//Ошибка прервано пользователем
#define EL_ERROR		16	//Ошибка в данных или примитиве

struct util
{
	static Acad::ErrorStatus AddEntityToModelSpace(AcDbObjectId& objId, AcDbEntity* pEntity,
		Adesk::Boolean isClose = true, AcDbDatabase* pDb = NULL);
	// Функция запрашивает статус слоя для указанного примитива
	// entId   - (выходной) идентификатор примитива
	static int GetStatusLayerEntity(const AcDbObjectId& entId);
	static int GetStatusLayerEntity(AcDbEntity* pEntity);
	//cоздать/Установить текущий сллой
	static AcDbObjectId SetCurLayer(LPCTSTR layer, Adesk::UInt16* colorIndex = NULL, AcDbDatabase* pDb = NULL);

	static int GetMeshFromDb(AcDbObjectIdArray& ids, AcDbDatabase* pDb = NULL);
	static AcGeCurve3d* convertDbCurveToGeCurve3d(AcDbCurve* pDbCurve);
	//Gолучить координаты вершин кривой (отрезки, все виды полилиний и вершины сплайна
	static bool GetPoint3dArrayFromCurve(AcDbCurve* pCurve, AcGePoint3dArray& pArray, Adesk::Boolean& clos);
	//Координаты линеаризированной кривой с точностью approxEps
	static int GetDbCurvePoints(AcGePoint3dArray& pnt, AcDbCurve* pDbCurve, double approxEps);

	//Сохранить/или читать в файле настроек
	static double	fromProfile(double def, LPCTSTR key, LPCTSTR app = NULL);
	static int		fromProfile(int def, LPCTSTR key, LPCTSTR app = NULL);
	static __int64	fromProfile(__int64 def, LPCTSTR key, LPCTSTR app = NULL);
	static LPCTSTR	fromProfile(LPTSTR buf, int ccMax, LPCTSTR def, LPCTSTR key, LPCTSTR app = NULL);
	static int		fromProfile(AcGeIntArray& arr, LPCTSTR key, LPCTSTR app = NULL);
	static int		fromProfile(CListCtrl& lst, LPCTSTR key, LPCTSTR app = NULL);
	static COLORREF	fromProfileColor(COLORREF def, LPCTSTR key, LPCTSTR app = NULL);

	static BOOL		toProfile(double val, LPCTSTR key, LPCTSTR app = NULL);
	static BOOL		toProfile(int val, LPCTSTR key, LPCTSTR app = NULL);
	static BOOL		toProfile(__int64 val, LPCTSTR key, LPCTSTR app = NULL);
	static BOOL		toProfile(LPCTSTR str, LPCTSTR key, LPCTSTR app = NULL);
	static BOOL		toProfile(AcGeIntArray& arr, LPCTSTR key, LPCTSTR app = NULL);
	static BOOL		toProfileColor(COLORREF clr, LPCTSTR key, LPCTSTR app = NULL);
	static int		toProfile(CListCtrl& lst, LPCTSTR key, LPCTSTR app = NULL);
	static void		getProfile(LPTSTR buf, int ccMax);
};

template<class T> bool selectEntity(ACHAR* promt, AcDbObjectId& id, AcGePoint3d* p = nullptr)
{
	while (true) {
		AcGePoint3d org;
		ads_name entName;
		T* ent = nullptr;
		int result = acedEntSel(promt, entName, asDblArray(org));
		switch (result)
		{
		case RTCAN:
			acutPrintf(_T("\n Прервано пользователем..."));
			return false;
		case RTNORM:
			if ((acdbGetObjectId(id, entName) == Acad::eOk) && (acdbOpenObject(ent, id, AcDb::kForRead) == Acad::eOk))
			{
				ent->close();
				if (p) *p = org;
				return true;
			}
			else acutPrintf(_T("\n Это не тот примитив. Повторите выбор..."));
			break;
		default:
			acutPrintf(_T("\n Ничего не выбрано. Повторите выбор..."));
			break;
		}
	}
	return false;
}
