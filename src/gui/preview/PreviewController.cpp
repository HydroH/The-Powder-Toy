#include <sstream>
#include "client/Client.h"
#include "PreviewController.h"
#include "PreviewView.h"
#include "PreviewModel.h"
#include "PreviewModelException.h"
#include "gui/dialogues/InformationMessage.h"
#include "gui/dialogues/ErrorMessage.h"
#include "gui/login/LoginController.h"
#include "Controller.h"
#include "Platform.h"
#include "Format.h"
#include "Lang.h"

PreviewController::PreviewController(int saveID, int saveDate, bool instant, ControllerCallback * callback):
	saveId(saveID),
	saveDate(saveDate),
	loginWindow(NULL),
	HasExited(false)
{
	previewModel = new PreviewModel();
	previewView = new PreviewView();
	previewModel->AddObserver(previewView);
	previewView->AttachController(this);
	previewModel->SetDoOpen(instant);

	previewModel->UpdateSave(saveID, saveDate);

	if(Client::Ref().GetAuthUser().ID)
	{
		previewModel->SetCommentBoxEnabled(true);
	}

	Client::Ref().AddListener(this);

	this->callback = callback;
	(void)saveDate; //pretend this is used
}

PreviewController::PreviewController(int saveID, bool instant, ControllerCallback * callback):
	saveId(saveID),
	saveDate(0),
	loginWindow(NULL),
	HasExited(false)
{
	previewModel = new PreviewModel();
	previewView = new PreviewView();
	previewModel->AddObserver(previewView);
	previewView->AttachController(this);

	previewModel->UpdateSave(saveID, 0);

	if(Client::Ref().GetAuthUser().ID)
	{
		previewModel->SetCommentBoxEnabled(true);
	}

	Client::Ref().AddListener(this);

	this->callback = callback;
	(void)saveDate; //pretend this is used
}

void PreviewController::Update()
{
	previewModel->Update();
	if (loginWindow && loginWindow->HasExited == true)
	{
		delete loginWindow;
		loginWindow = NULL;
	}
	if (previewModel->GetDoOpen() && previewModel->GetSaveInfo() && previewModel->GetSaveInfo()->GetGameSave())
	{
		Exit();
	}
}

bool PreviewController::SubmitComment(std::string comment)
{
	if(comment.length() < 4)
	{
		new ErrorMessage(TEXT_GUI_SAVE_PRE_COMM_ERR_TITLE, TEXT_GUI_SAVE_PRE_COMM_ERR_MSG);
		return false;
	}
	else
	{
		RequestStatus status = Client::Ref().AddComment(saveId, comment);
		if(status != RequestOkay)
		{
			new ErrorMessage(TEXT_GUI_SAVE_PRE_COMM_SUB_ERR_TITLE, Client::Ref().GetWLastError());
			return false;
		}
		else
		{
			previewModel->CommentAdded();
			previewModel->UpdateComments(1);
		}
	}
	return true;
}

void PreviewController::ShowLogin()
{
	loginWindow = new LoginController();
	loginWindow->GetView()->MakeActiveWindow();
}

void PreviewController::NotifyAuthUserChanged(Client * sender)
{
	previewModel->SetCommentBoxEnabled(sender->GetAuthUser().ID);
}

SaveInfo * PreviewController::GetSaveInfo()
{
	return previewModel->GetSaveInfo();
}

bool PreviewController::GetDoOpen()
{
	return previewModel->GetDoOpen();
}

void PreviewController::DoOpen()
{
	previewModel->SetDoOpen(true);
}

void PreviewController::Report(std::string message)
{
	if(Client::Ref().ReportSave(saveId, message) == RequestOkay)
	{
		Exit();
		new InformationMessage(TEXT_GUI_SAVE_PRE_REPORT_INFO_TITLE, TEXT_GUI_SAVE_PRE_REPORT_INFO_MSG, false);
	}
	else
		new ErrorMessage(TEXT_GUI_SAVE_PRE_REPORT_ERR_TITLE, TEXT_GUI_SAVE_PRE_REPORT_ERR_MSG + Client::Ref().GetWLastError());
}

void PreviewController::FavouriteSave()
{
	if(previewModel->GetSaveInfo() && Client::Ref().GetAuthUser().ID)
	{
		try
		{
			if(previewModel->GetSaveInfo()->Favourite)
				previewModel->SetFavourite(false);
			else
				previewModel->SetFavourite(true);
		}
		catch (PreviewModelException & e)
		{
			new ErrorMessage(TEXT_GUI_SAVE_PRE_FAV_ERR_TITLE, format::StringToWString(e.what()));
		}
	}
}

void PreviewController::OpenInBrowser()
{
	std::stringstream uriStream;
	uriStream << "http://" << SERVER << "/Browse/View.html?ID=" << saveId;
	Platform::OpenURI(uriStream.str());
}

bool PreviewController::NextCommentPage()
{
	if(previewModel->GetCommentsPageNum() < previewModel->GetCommentsPageCount() && previewModel->GetCommentsLoaded() && !previewModel->GetDoOpen())
	{
		previewModel->UpdateComments(previewModel->GetCommentsPageNum()+1);
		return true;
	}
	return false;
}

bool PreviewController::PrevCommentPage()
{
	if(previewModel->GetCommentsPageNum() > 1 && previewModel->GetCommentsLoaded() && !previewModel->GetDoOpen())
	{
		previewModel->UpdateComments(previewModel->GetCommentsPageNum()-1);
		return true;
	}
	return false;
}

void PreviewController::Exit()
{
	previewView->CloseActiveWindow();
	HasExited = true;
	if(callback)
		callback->ControllerExit();
}

PreviewController::~PreviewController()
{
	previewView->CloseActiveWindow();
	Client::Ref().RemoveListener(this);
	delete previewModel;
	delete previewView;
	delete callback;
}
