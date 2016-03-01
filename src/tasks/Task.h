#ifndef TASK_H_
#define TASK_H_

#include <string>
#include <pthread.h>
#undef GetUserName //God dammit microsoft!
#include "TaskListener.h"
#include "Config.h"

class TaskListener;
class Task {
public:
	void AddTaskListener(TaskListener * listener);
	void Start();
	int GetProgress();
	bool GetDone();
	bool GetSuccess();
	std::string GetError();
	std::wstring GetWError();
	std::string GetStatus();
	std::wstring GetWStatus();
	void Poll();
	Task() : listener(NULL) { progress = 0; thProgress = 0; }
	virtual ~Task();
protected:
	int progress;
	bool done;
	bool success;
	std::wstring status;
	std::wstring error;

	int thProgress;
	bool thDone;
	bool thSuccess;
	std::wstring thStatus;
	std::wstring thError;

	TaskListener * listener;
	pthread_t doWorkThread;
	pthread_mutex_t taskMutex;
	pthread_cond_t taskCond;


	virtual void before();
	virtual void after();
	virtual bool doWork();
	TH_ENTRY_POINT static void * doWork_helper(void * ref);

	virtual void notifyProgress(int progress);
	virtual void notifyError(std::string error);
	virtual void notifyError(std::wstring error);
	virtual void notifyStatus(std::string status);
	virtual void notifyStatus(std::wstring status);

	virtual void notifyProgressMain();
	virtual void notifyErrorMain();
	virtual void notifyStatusMain();
	virtual void notifyDoneMain();
};

#endif /* TASK_H_ */
