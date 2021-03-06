#include <sstream>

#include "client/Client.h"
#include "LocalBrowserController.h"
#include "gui/interface/Engine.h"
#include "gui/dialogues/ConfirmPrompt.h"
#include "tasks/TaskWindow.h"
#include "tasks/Task.h"

#include "LocalBrowserModel.h"
#include "LocalBrowserView.h"

#include "Format.h"
#include "Lang.h"

LocalBrowserController::LocalBrowserController(ControllerCallback * callback):
	HasDone(false)
{
	browserModel = new LocalBrowserModel();
	browserView = new LocalBrowserView();
	browserView->AttachController(this);
	browserModel->AddObserver(browserView);

	this->callback = callback;

	browserModel->UpdateSavesList(1);
}

void LocalBrowserController::OpenSave(SaveFile * save)
{
	browserModel->SetSave(save);
}

SaveFile * LocalBrowserController::GetSave()
{
	return browserModel->GetSave();
}

void LocalBrowserController::RemoveSelected()
{
	class RemoveSelectedConfirmation: public ConfirmDialogueCallback {
	public:
		LocalBrowserController * c;
		RemoveSelectedConfirmation(LocalBrowserController * c_) {	c = c_;	}
		virtual void ConfirmCallback(ConfirmPrompt::DialogueResult result) {
			if (result == ConfirmPrompt::ResultOkay)
				c->removeSelectedC();
		}
		virtual ~RemoveSelectedConfirmation() { }
	};

	std::wstringstream desc;
	desc << TEXT_GUI_LOCAL_BROWSE_CONF_DELETE_MSG1 << browserModel->GetSelected().size() << TEXT_GUI_LOCAL_BROWSE_CONF_DELETE_MSG2;
	if(browserModel->GetSelected().size()>1)
		desc << TEXT_GUI_LOCAL_BROWSE_CONF_DELETE_MSG_MULTI;
	desc << L"?";
	new ConfirmPrompt(TEXT_GUI_LOCAL_BROWSE_CONF_DELETE_TITLE, desc.str(), new RemoveSelectedConfirmation(this));
}

void LocalBrowserController::removeSelectedC()
{
	class RemoveSavesTask : public Task
	{
		std::vector<std::string> saves;
		LocalBrowserController * c;
	public:
		RemoveSavesTask(LocalBrowserController * c, std::vector<std::string> saves_) : c(c) { saves = saves_; }
		virtual bool doWork()
		{
			for (size_t i = 0; i < saves.size(); i++)
			{
				std::wstringstream saveName;
				saveName << TEXT_GUI_LOCAL_BROWSE_STAT_DELETE_MSG1 << format::StringToWString(saves[i]) << TEXT_GUI_LOCAL_BROWSE_STAT_DELETE_MSG2;
 				notifyStatus(saveName.str());
 				Client::Ref().DeleteStamp(saves[i]);
				notifyProgress((float(i+1)/float(saves.size())*100));
			}
			return true;
		}
		virtual void after()
		{
			Client::Ref().updateStamps();
			c->RefreshSavesList();
		}
	};

	std::vector<std::string> selected = browserModel->GetSelected();
	new TaskWindow(TEXT_GUI_LOCAL_BROWSE_CONF_DELETE_WIN_TITLE, new RemoveSavesTask(this, selected)); 
}

void LocalBrowserController::RescanStamps()
{
	class RescanConfirmation: public ConfirmDialogueCallback {
	public:
		LocalBrowserController * c;
		RescanConfirmation(LocalBrowserController * c_) {	c = c_;	}
		virtual void ConfirmCallback(ConfirmPrompt::DialogueResult result) {
			if (result == ConfirmPrompt::ResultOkay)
				c->rescanStampsC();
		}
		virtual ~RescanConfirmation() { }
	};

	std::wstringstream desc;
	desc << TEXT_GUI_LOCAL_BROWSE_CONF_RESCAN_MSG;
	new ConfirmPrompt(TEXT_GUI_LOCAL_BROWSE_CONF_RESCAN_TITLE, desc.str(), new RescanConfirmation(this));
}

void LocalBrowserController::rescanStampsC()
{
	browserModel->RescanStamps();
	browserModel->UpdateSavesList(browserModel->GetPageNum());
}

void LocalBrowserController::RefreshSavesList()
{
	ClearSelection();
	browserModel->UpdateSavesList(browserModel->GetPageNum());
}

void LocalBrowserController::ClearSelection()
{
	browserModel->ClearSelected();
}

void LocalBrowserController::NextPage()
{
	if(browserModel->GetPageNum() < browserModel->GetPageCount())
		browserModel->UpdateSavesList(browserModel->GetPageNum()+1);
}

void LocalBrowserController::PrevPage()
{
	if(browserModel->GetPageNum()>1)
		browserModel->UpdateSavesList(browserModel->GetPageNum()-1);
}

void LocalBrowserController::SetPage(int page)
{
	if (page != browserModel->GetPageNum() && page > 0 && page <= browserModel->GetPageCount())
		browserModel->UpdateSavesList(page);
}

void LocalBrowserController::Update()
{
	if(browserModel->GetSave())
	{
		Exit();
	}
}

void LocalBrowserController::Selected(std::string saveName, bool selected)
{
	if(selected)
		browserModel->SelectSave(saveName);
	else
		browserModel->DeselectSave(saveName);
}

bool LocalBrowserController::GetMoveToFront()
{
	return browserModel->GetMoveToFront();
}

void LocalBrowserController::SetMoveToFront(bool move)
{
	browserModel->SetMoveToFront(move);
}

void LocalBrowserController::Exit()
{
	browserView->CloseActiveWindow();
	if(callback)
		callback->ControllerExit();
	HasDone = true;
}

LocalBrowserController::~LocalBrowserController()
{
	browserView->CloseActiveWindow();
	delete callback;
	delete browserModel;
	delete browserView;
}

