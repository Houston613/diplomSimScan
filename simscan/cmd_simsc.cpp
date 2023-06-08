#include "StdAfx.h"

#include "resource.h"
#include <dbSubD.h>
#include "util.h"
#include "simsc.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <cmath>

//#define DEBUGFILE

#ifdef DEBUGFILE
FILE* g_stream = nullptr;
#endif

class CDlgOptSS : public CAcUiDialog
{
    DECLARE_DYNAMIC(CDlgOptSS)

public:
    TCHAR m_path[_MAX_PATH];
    double m_step_lin, m_step_angl;
    double m_sigma, m_mu;
    bool checkBox;

public:
    CDlgOptSS();   // standard constructor
    virtual ~CDlgOptSS();

    // Dialog Data
    enum { IDD = IDD_DIALOG2 };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnBnClickedButton1();
};

// CDlgHstgrm dialog
IMPLEMENT_DYNAMIC(CDlgOptSS, CAcUiDialog)

BEGIN_MESSAGE_MAP(CDlgOptSS, CAcUiDialog)
    ON_BN_CLICKED(IDC_BUTTON1, &CDlgOptSS::OnBnClickedButton1)
END_MESSAGE_MAP()

CDlgOptSS::CDlgOptSS() : CAcUiDialog(CDlgOptSS::IDD, acedGetAcadDwgView())
{
}

CDlgOptSS::~CDlgOptSS()
{
}

void CDlgOptSS::DoDataExchange(CDataExchange* pDX)
{
    CAcUiDialog::DoDataExchange(pDX);
}

BOOL CDlgOptSS::OnInitDialog()
{
    CAcUiDialog::OnInitDialog();
    const TCHAR* curDwg = NULL;
    TCHAR dir[_MAX_DIR], drive[_MAX_DRIVE], file[_MAX_FNAME];
    acdbCurDwg()->getFilename(curDwg);
    _tsplitpath_s(curDwg, drive, _MAX_DRIVE, dir, _MAX_DIR, file, _MAX_FNAME, NULL, 0);
    _tmakepath_s(m_path, _MAX_PATH, drive, dir, file, L".ply");
    GetDlgItem(IDC_EDIT1)->SetWindowText(m_path);
    
    m_step_lin = util::fromProfile((double)0.01, L"StepLin", L"SimScan");
    m_step_angl = util::fromProfile((double)30.0, L"StepAngl", L"SimScan");
    CString str;
    str.Format(_T("%.4f"), m_step_lin); GetDlgItem(IDC_EDIT2)->SetWindowText((LPCTSTR)str);
    str.Format(_T("%.4f"), m_step_angl); GetDlgItem(IDC_EDIT3)->SetWindowText((LPCTSTR)str);

    return TRUE;// return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgOptSS::OnBnClickedButton1()
{
    //Запросить файла для вывода точек
    TCHAR szFilters[] = _T("Текстовый файл для вывода (*.ply)|*.ply|Все файлы (*.*)|*.*||");
    CFileDialog fileDlg(FALSE, _T("ply"), NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilters);
    if (fileDlg.DoModal() != IDOK) return;
    _tcscpy_s(m_path, _MAX_PATH, fileDlg.GetPathName());
    GetDlgItem(IDC_EDIT1)->SetWindowText(m_path);
}


void CDlgOptSS::OnOK()
{
    GetDlgItem(IDC_EDIT1)->GetWindowTextW(m_path, _MAX_PATH);
    CString str;
    //BOOL isChecked;
    GetDlgItem(IDC_EDIT2)->GetWindowText(str);
    m_step_lin = _tstof(str); util::toProfile(m_step_lin, L"StepLin", L"SimScan");

    GetDlgItem(IDC_EDIT3)->GetWindowText(str);
    m_step_angl = _tstof(str); util::toProfile(m_step_angl, L"StepAngl", L"SimScan");

    GetDlgItem(IDC_EDIT4)->GetWindowText(str);
    m_sigma = _tstof(str); util::toProfile(m_sigma, L"Sigma", L"SimScan");

    GetDlgItem(IDC_EDIT5)->GetWindowText(str);
    m_mu = _tstof(str); util::toProfile(m_mu, L"Mu", L"SimScan");

    //GetDlgItem(IDC_CHECKBOX)->IsDlgButtonChecked(isChecked);

    //m_step_angl = _tstof(str); util::toProfile(m_step_angl, L"StepAngl", L"SimScan");
    //if (isChecked == BST_CHECKED) {
      //  checkBox = true;
    //}
    //else if (isChecked == BST_UNCHECKED) {
    //    checkBox = false;
   // }

    CAcUiDialog::OnOK();
}


//Конец диалога===============================

struct Vertex {
    double x, y, z;
    AcString scalar;
};

bool getPointScan1(const AcGePoint3d& p, const AcGeVector3d& v,
    const AcDbObjectIdArray& ids, AcGePoint3d& ret, AcString& resultedGroup);


void distorter(std::vector<Vertex>& vertices, double mu, double sigma) {

    std::default_random_engine rng;
    std::normal_distribution<double> dist(mu, sigma);


        for (auto& v : vertices) {
        v.x += dist(rng);
        v.y += dist(rng);
        v.z += dist(rng);
    }

}





void writePLYFile(FILE* file, const std::vector<Vertex>& vertices);

std::FILE* outFile = std::fopen("C:\\Output\\output.txt", "a"); // Open the file in append mode


void writePLYFile(FILE* file, const std::vector<Vertex>& vertices) {
    fprintf(file, "ply\n");
    fprintf(file, "format ascii 1.0\n");
    fprintf(file, "element vertex %d\n", static_cast<int>(vertices.size()));
    fprintf(file, "property float x\n");
    fprintf(file, "property float y\n");
    fprintf(file, "property float z\n");
    fprintf(file, "property float scalar\n");
    fprintf(file, "end_header\n");

    for (const Vertex& v : vertices) {
        fprintf(file, "%f %f %f %s\n", v.x, v.y, v.z, v.scalar);
    }
}





int quad2tri(AcArray<Adesk::Int32>& face);

void CsimscanApp::Houston_standartLidar()
{
    std::vector<Vertex> vertices;
    AcDbObjectIdArray ids;
    int n = util::GetMeshFromDb(ids);
    if (n == 0)
    {
        acutPrintf(_T("\n В рисунке нет сеток..."));
        return;
    }


    AcDbObjectId trjId;
    if (!selectEntity<AcDbCurve>(L"\nУкажите примитив-траекторию сканера:", trjId))
    {
        acutPrintf(_T("\n Прервано пользователем..."));
        return;
    }
    CDlgOptSS dlg;
    if (dlg.DoModal() != IDOK)
    {
        acutPrintf(_T("\n Прервано пользователем..."));
        return;
    }

    //Открыть файл для вывода точек
    FILE* stream_output = NULL;
    if (_tfopen_s(&stream_output, dlg.m_path, L"wb"))
    {
        acutPrintf(_T("\nНе могу создать файл для вывода '%s'\nПроверте источник для записи"), dlg.m_path);
        return;
    }
    DWORD st = ::GetTickCount();


#ifdef DEBUGFILE
    {
        const TCHAR* curDwg = NULL;
        TCHAR path[_MAX_PATH], dir[_MAX_DIR], drive[_MAX_DRIVE], file[_MAX_FNAME];
        acdbCurDwg()->getFilename(curDwg);
        _tsplitpath(curDwg, drive, dir, file, NULL);
        _tmakepath(path, drive, dir, file, L".log");
        g_stream = nullptr;
        if (_tfopen_s(&g_stream, path, L"wb")) g_stream = nullptr;
    }
#endif


    double step = dlg.m_step_lin;
    double angl = dlg.m_step_angl/ 206264.806247096;
    double cof = 6.28318530717959 / angl;
    double delta = step / cof;

    double sigma = dlg.m_sigma;
    double mu = dlg.m_mu;

    //AcGePoint3dArray pts;
    AcDbCurve* curve = nullptr;
    uint64_t numAll = 0, numPoint = 0;
    if (acdbOpenObject(curve, trjId, AcDb::kForRead) == Acad::eOk)
    {
        AcGePoint3d ep, p, ret;
        AcDbObjectId objId;
        AcGeVector3d norm, vp, v;
        AcGeMatrix3d mat;
        double dist;
        curve->getEndPoint(ep);
        curve->getDistAtPoint(ep, dist);
        
        //Установки прогрессбара --------------------------
        acedSetStatusBarProgressMeter(L"Сканирую...", 0, 100);
        int cur_pos, prev_pos = 0;
        double percent = 100. / dist;
        //Конец установки прогрессбара---------------------
        
        for (double cur = 0; cur < dist; cur += step)
        {
            curve->getPointAtDist(cur, p);
            curve->getFirstDeriv(p, norm);
            mat = AcGeMatrix3d::planeToWorld(norm);
            AcGeVector3d vdelta = norm.normal()* delta;
            for (double a = 0.; a <= 6.28318530717959; a += angl)
            {
                v = mat * AcGeVector3d(cos(a), sin(a), 0.);
                numAll++;
                AcString groupName;
                if (getPointScan1(p, v, ids, ret, groupName))
                {
                    Vertex v = { ret.x, ret.y, ret.z, groupName};
                    vertices.push_back(v);
                    //writePLYFile(stream_output, v);
                    //fprintf(stream_output, "%.3f %.3f %.3f %s \r\n", ret.x, ret.y, ret.z, groupName);
                    fprintf(outFile, "%.3f %.3f \r\n", sigma, mu);

                    numPoint++;
                }
                p += vdelta;
            }
            //Изменение позиции прогрессбара------------
            if ( (cur_pos = (int)(cur * percent)) > prev_pos)
            {
                acedSetStatusBarProgressMeterPos(cur_pos);
                prev_pos = cur_pos;
            }
            //------------------------------------------
        }
        acedRestoreStatusBar();//Удалить прогрессбар

        curve->close();
    }
    distorter(vertices, mu, sigma);


    writePLYFile(stream_output, vertices);

    fclose(stream_output);//Закрыть файл для записи

    acutPrintf(_T("\n Записано  %d точек из %d, %.3f сек..."), (int)numPoint, (int)numAll, 0.001 * (::GetTickCount()-st));


#ifdef DEBUGFILE
    if(g_stream) 
    {
        fclose(g_stream);
        g_stream = nullptr;
    }
#endif
}