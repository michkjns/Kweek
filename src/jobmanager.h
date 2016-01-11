#include <Windows.h>

#define NUMTHREADS 3
#define MAXJOBS 1024

class Job
{
public:
	Job(void(*func) (void*), void* a_param) { JobFunc = func; param = a_param; }

	void (*JobFunc)(void*);
	void* param;
};

class JThread
{
public:
	JThread();
	~JThread();
	void BackgroundTask();

	HANDLE m_threadHandle;
	unsigned int m_iD;
	
};

class JobManager
{
public:
	static JobManager* m_jobManager;
	static JobManager* CreateManager(unsigned int a_NumThreads);
	void AddJobb( void(*FunctionPtr)(void*), void* a_Param);
	Job* NextJob();
	void FinishThread(unsigned int id);
	void Wait();

protected:
	JobManager(unsigned int a_numThreads);
	~JobManager();
	CRITICAL_SECTION m_cS;
	HANDLE m_threadHandles[NUMTHREADS];
	HANDLE hSempahore;
	Job* m_jobList[MAXJOBS];
	JThread* m_threadList[NUMTHREADS];
	unsigned int m_numThreads;
	unsigned int m_jobCount;
	friend class JThread;
};