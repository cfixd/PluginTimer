#include <Windows.h>
#include "../../API/RainmeterAPI.h"
#include<vector>

struct Measure
{
	void* skin;
	double dCount;
	double dStep;
	double dFinish;
	DWORD nUpdate;
	HANDLE hTimer;
	std::wstring sCountAction;
	std::wstring sFinishAction;
	Measure() : hTimer(NULL), dCount(0), dStep(1), dFinish(0), nUpdate(1000){}

	void StartTimer(void* measure);
	void StopTimer(void);
};

VOID CALLBACK TimerRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
	Measure* measure = (Measure*)lpParam;

	measure->dCount += measure->dStep;

	if (measure->dCount <= 0)
	{
		measure->dCount = 0;
		measure->StopTimer();
		RmExecute(measure->skin, measure->sFinishAction.c_str());
	}
	else if (measure->dFinish && ((measure->dStep > 0 && measure->dCount >= measure->dFinish) || (measure->dStep < 0 && measure->dCount <= measure->dFinish)))
	{
		measure->dCount = measure->dFinish;
		measure->StopTimer();
		RmExecute(measure->skin, measure->sFinishAction.c_str());
	}

	RmExecute(measure->skin, measure->sCountAction.c_str());
}

void Measure::StartTimer(void* measure)
{
	dCount++;
	if (dCount < 1) return;

	dCount = 0;

	if (hTimer)
	{
		if (!ChangeTimerQueueTimer(NULL, hTimer, nUpdate, nUpdate))
			RmLog(LOG_WARNING, L"Timer.dll: Reset Timer failed");
	}
	else if (!CreateTimerQueueTimer(&hTimer, NULL,
		(WAITORTIMERCALLBACK)TimerRoutine, measure, nUpdate, nUpdate, 0))
	{
		hTimer = NULL;
		dCount = -1;
		RmLog(LOG_WARNING, L"Timer.dll: Create Timer failed");
	}
}


void Measure::StopTimer(void)
{
	if (hTimer)
	{
		DeleteTimerQueueTimer(NULL, hTimer, NULL);
		hTimer = NULL;
	}
}


PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	Measure* measure = new Measure;
	*data = measure;

	measure->skin = RmGetSkin(rm);

	measure->dCount = RmReadInt(rm, L"StartOnLoad", 0);

	measure->dStep = RmReadDouble(rm, L"Step", 1);
	if (measure->dStep < 0)
	{
		measure->dStep = 0;
		RmLog(LOG_ERROR, L"Timer.dll: Step value error");
	}

	int i = RmReadInt(rm, L"Update", 1000);

	if (i > 3600000 || i < 10)
	{
		RmLog(LOG_ERROR, L"Timer.dll: Update value is out of range (10~3600000 ms)");
		measure->dCount = -1;
		return;
	}
	else measure->nUpdate = i;

	if (measure->dCount > 0) measure->StartTimer(measure);
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	Measure* measure = (Measure*)data;

	measure->sCountAction = RmReadString(rm, L"CountAction", L"", FALSE);
	measure->sFinishAction = RmReadString(rm, L"FinishAction", L"", FALSE);

	int i = RmReadInt(rm, L"Update", 1000);

	if (i > 3600000 || i < 10)
	{
		RmLog(LOG_ERROR, L"Timer.dll: Update value is out of range (10~3600000 ms)");
		measure->dCount = -1;
		return;
	}
	else measure->nUpdate = i;
}

PLUGIN_EXPORT double Update(void* data)
{
	Measure* measure = (Measure*)data;
	return measure->dCount;
}

//PLUGIN_EXPORT LPCWSTR GetString(void* data)
//{
//	Measure* measure = (Measure*)data;
//	return L"";
//}

PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args)
{
	Measure* measure = (Measure*)data;

	WCHAR sBang[30] = { NULL };
	WCHAR *sFind;
	BOOL setvalue = false;
	double dCount = 0, dFinish = 0;

	wcscpy_s(sBang, args);

	if (sFind = wcschr(sBang, L'@'))
	{
		swscanf_s(sFind + 1, L"%lf", &measure->dStep);
		*sFind = NULL;
	}

	measure->dFinish = 0;
	if (sFind = wcschr(sBang, L'_'))
	{
		if (BYTE b = swscanf_s(sFind + 1, L"%lf_%lf", &dCount, &dFinish))
		{
			if (b && dCount > 0) setvalue = true;
			if (b == 2 && dFinish > 0) measure->dFinish = dFinish;
		}
		*sFind = NULL;		
	}


	if (_wcsicmp(sBang, L"Start") == 0)
	{
		measure->StartTimer(measure);
		if (setvalue)measure->dCount = dCount;
	}
	else if (_wcsicmp(sBang, L"Continue") == 0)
	{
		if (measure->hTimer)
		{
			if (setvalue)measure->dCount = dCount;
		}
		else measure->StartTimer(measure);
	}
	else if (_wcsicmp(sBang, L"Stop") == 0)
	{
		measure->StopTimer();
	}
	else RmLog(LOG_WARNING, L"Timer.dll: Unknown Bang");
}

PLUGIN_EXPORT void Finalize(void* data)
{
	Measure* measure = (Measure*)data;
	measure->StopTimer();
	delete measure;
}
