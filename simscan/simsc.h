#pragma once

class CsimscanApp : public AcRxArxApp {

public:
	CsimscanApp() : AcRxArxApp() {}
	virtual AcRx::AppRetCode On_kInitAppMsg(void* pkt);
	virtual AcRx::AppRetCode On_kUnloadAppMsg(void* pkt);
	virtual void RegisterServerComponents();
	static void Houston_standartLidar();
	static void Houston_pfm2msh();
	static void Houston_sol2msh(); //Конверировать солид в сеть


};
