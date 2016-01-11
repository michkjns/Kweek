
#include "jobmanager.h"

JobManager* JobManager::m_jobManager = 0;

DWORD WINAPI dBackgroundTask(LPVOID lpParam)
{
	JThread* param = (JThread*) lpParam;
	param->BackgroundTask();
	return 0;
}


JThread::JThread()
{
	m_threadHandle = CreateThread(NULL, 0, &dBackgroundTask, this, NULL, NULL);
}

JThread::~JThread()
{
	CloseHandle(m_threadHandle);
}

void JThread::BackgroundTask()
{
	while(1)
	{
		WaitForSingleObject(JobManager::m_jobManager->hSempahore, INFINITE); // Prevent spinlock if there are no jobs
		Job* job = JobManager::m_jobManager->NextJob();
		if(job) 
		{
			if(job->JobFunc == nullptr && job->param == nullptr)
			{
				JobManager::m_jobManager->FinishThread(m_iD);
				return;
			}

			job->JobFunc(job->param);
			delete job;
		}
	}
}

void JobManager::FinishThread(unsigned int id)
{
	SetEvent(m_threadHandles[id]);
}

void JobManager::Wait()
{
	for(unsigned int i = 0; i < m_numThreads; i++)
	{
		AddJobb(m_jobList[i]->JobFunc, m_jobList[i]->param);
		m_jobList[i]->JobFunc = nullptr;
		m_jobList[i]->param = nullptr;
	}
	WaitForMultipleObjects(m_numThreads, m_threadHandles, true, INFINITE);

	for(unsigned int i = 0; i < m_numThreads; i++)
	{
		CloseHandle(m_threadList[i]->m_threadHandle);
		m_threadList[i]->m_threadHandle = CreateThread(NULL, 0, &dBackgroundTask, m_threadList[i], NULL, NULL);
	}
}

JobManager::JobManager(unsigned int a_NumThreads)
{
	m_numThreads = a_NumThreads;
	m_jobCount = 0;
	InitializeCriticalSection(&m_cS);
	hSempahore = CreateSemaphore(NULL, 0, MAXJOBS, NULL);
	for(unsigned int i = 0; i < m_numThreads; i++)
	{
		JThread* thread = new JThread();
		thread->m_iD = i;
		m_threadList[i] = thread;
		m_threadHandles[i] = CreateEvent(NULL, false, false, NULL);
	}
}

JobManager::~JobManager()
{
	DeleteCriticalSection( &m_cS );
}
void JobManager::AddJobb(void(*FunctionPtr)(void*), void* a_Param)
{
	EnterCriticalSection(&m_cS);
	if(m_jobCount < MAXJOBS)
	{
		m_jobList[m_jobCount++] = new Job(FunctionPtr, a_Param);
	}
	LeaveCriticalSection(&m_cS);
	ReleaseSemaphore(hSempahore, 1, NULL);
}

Job* JobManager::NextJob()
{
	Job* nextJob = 0;
	EnterCriticalSection(&m_cS);
	if(m_jobCount > 0)	nextJob = m_jobList[--m_jobCount];	
	LeaveCriticalSection(&m_cS);
	return nextJob;
}
JobManager* JobManager::CreateManager(unsigned int a_NumThreads)
{
	if(!m_jobManager)
	{
		m_jobManager = new JobManager(a_NumThreads);
	}

	return m_jobManager;
}
