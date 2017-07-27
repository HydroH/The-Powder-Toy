#include "ServerSaveActivity.h"
#include "graphics/Graphics.h"
#include "gui/interface/Label.h"
#include "gui/interface/Textbox.h"
#include "gui/interface/Button.h"
#include "gui/interface/Checkbox.h"
#include "client/requestbroker/RequestBroker.h"
#include "gui/dialogues/ErrorMessage.h"
#include "gui/dialogues/SaveIDMessage.h"
#include "gui/dialogues/ConfirmPrompt.h"
#include "gui/dialogues/InformationMessage.h"
#include "client/Client.h"
#include "tasks/Task.h"
#include "gui/Style.h"
#include "client/GameSave.h"
#include "images.h"
#include "Format.h"
#include "Lang.h"

class ServerSaveActivity::CancelAction: public ui::ButtonAction
{
	ServerSaveActivity * a;
public:
	CancelAction(ServerSaveActivity * a) : a(a) {}
	virtual void ActionCallback(ui::Button * sender)
	{
		a->Exit();
	}
};

class ServerSaveActivity::SaveAction: public ui::ButtonAction
{
	ServerSaveActivity * a;
public:
	SaveAction(ServerSaveActivity * a) : a(a) {}
	virtual void ActionCallback(ui::Button * sender)
	{
		a->Save();
	}
};

class ServerSaveActivity::PublishingAction: public ui::ButtonAction
{
	ServerSaveActivity * a;
public:
	PublishingAction(ServerSaveActivity * a) : a(a) {}
	virtual void ActionCallback(ui::Button * sender)
	{
		a->ShowPublishingInfo();
	}
};

class ServerSaveActivity::RulesAction: public ui::ButtonAction
{
	ServerSaveActivity * a;
public:
	RulesAction(ServerSaveActivity * a) : a(a) {}
	virtual void ActionCallback(ui::Button * sender)
	{
		a->ShowRules();
	}
};

class ServerSaveActivity::NameChangedAction: public ui::TextboxAction
{
public:
	ServerSaveActivity * a;
	NameChangedAction(ServerSaveActivity * a) : a(a) {}
	virtual void TextChangedCallback(ui::Textbox * sender) {
		a->CheckName(sender->GetText());
	}
};

class SaveUploadTask: public Task
{
	SaveInfo save;

	virtual void before()
	{

	}

	virtual void after()
	{

	}

	virtual bool doWork()
	{
		notifyProgress(-1);
		return Client::Ref().UploadSave(save) == RequestOkay;
	}

public:
	SaveInfo GetSave()
	{
		return save;
	}

	SaveUploadTask(SaveInfo save):
		save(save)
	{

	}
};

ServerSaveActivity::ServerSaveActivity(SaveInfo save, ServerSaveActivity::SaveUploadedCallback * callback) :
	WindowActivity(ui::Point(-1, -1), ui::Point(440, 200)),
	thumbnail(NULL),
	save(save),
	callback(callback),
	saveUploadTask(NULL)
{
	titleLabel = new ui::Label(ui::Point(4, 5), ui::Point((Size.X/2)-8, 16), "");
	titleLabel->SetTextColour(style::Colour::InformationTitle);
	titleLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	titleLabel->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	AddComponent(titleLabel);
	CheckName(save.GetName()); //set titleLabel text

	ui::Label * previewLabel = new ui::Label(ui::Point((Size.X/2)+4, 5), ui::Point((Size.X/2)-8, 16), TEXT_GUI_SAVE_WIN_PREVIEW_LABEL);
	previewLabel->SetTextColour(style::Colour::InformationTitle);
	previewLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	previewLabel->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	AddComponent(previewLabel);

	nameField = new ui::Textbox(ui::Point(8, 25), ui::Point((Size.X/2)-16, 16), format::StringToWString(save.GetName()), TEXT_GUI_SAVE_WIN_NAME_TBOX_HOLDER);
	nameField->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	nameField->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	nameField->SetActionCallback(new NameChangedAction(this));
	AddComponent(nameField);
	FocusComponent(nameField);

	descriptionField = new ui::Textbox(ui::Point(8, 65), ui::Point((Size.X/2)-16, Size.Y-(65+16+4)), format::StringToWString(save.GetDescription()), TEXT_GUI_SAVE_WIN_DESC_TBOX_HOLDER);
	descriptionField->SetMultiline(true);
	descriptionField->SetLimit(254);
	descriptionField->Appearance.VerticalAlign = ui::Appearance::AlignTop;
	descriptionField->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	AddComponent(descriptionField);

	publishedCheckbox = new ui::Checkbox(ui::Point(8, 45), ui::Point((Size.X/2)-80, 16), TEXT_GUI_SAVE_WIN_PUBLISH_CHECK, L"");
	if(Client::Ref().GetAuthUser().Username != save.GetUserName())
	{
		//Save is not owned by the user, disable by default
		publishedCheckbox->SetChecked(false);	
	}
	else
	{
		//Save belongs to the current user, use published state already set
		publishedCheckbox->SetChecked(save.GetPublished());
	}
	AddComponent(publishedCheckbox);

	pausedCheckbox = new ui::Checkbox(ui::Point(144, 45), ui::Point(55, 16), TEXT_GUI_SAVE_WIN_PAUSE_CHECK, L"");
	pausedCheckbox->SetChecked(save.GetGameSave()->paused);
	AddComponent(pausedCheckbox);

	ui::Button * cancelButton = new ui::Button(ui::Point(0, Size.Y-16), ui::Point((Size.X/2)-75, 16), TEXT_GUI_SAVE_WIN_BTN_CANCEL);
	cancelButton->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	cancelButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	cancelButton->Appearance.BorderInactive = ui::Colour(200, 200, 200);
	cancelButton->SetActionCallback(new CancelAction(this));
	AddComponent(cancelButton);
	SetCancelButton(cancelButton);

	ui::Button * okayButton = new ui::Button(ui::Point((Size.X/2)-76, Size.Y-16), ui::Point(76, 16), TEXT_GUI_SAVE_WIN_BTN_SAVE);
	okayButton->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	okayButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	okayButton->Appearance.TextInactive = style::Colour::InformationTitle;
	okayButton->SetActionCallback(new SaveAction(this));
	AddComponent(okayButton);
	SetOkayButton(okayButton);

	ui::Button * PublishingInfoButton = new ui::Button(ui::Point((Size.X*3/4)-75, Size.Y-42), ui::Point(150, 16), TEXT_GUI_SAVE_WIN_BTN_INFO);
	PublishingInfoButton->Appearance.HorizontalAlign = ui::Appearance::AlignCentre;
	PublishingInfoButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	PublishingInfoButton->Appearance.TextInactive = style::Colour::InformationTitle;
	PublishingInfoButton->SetActionCallback(new PublishingAction(this));
	AddComponent(PublishingInfoButton);

	ui::Button * RulesButton = new ui::Button(ui::Point((Size.X*3/4)-75, Size.Y-22), ui::Point(150, 16), TEXT_GUI_SAVE_WIN_BTN_RULE);
	RulesButton->Appearance.HorizontalAlign = ui::Appearance::AlignCentre;
	RulesButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	RulesButton->Appearance.TextInactive = style::Colour::InformationTitle;
	RulesButton->SetActionCallback(new RulesAction(this));
	AddComponent(RulesButton);

	if(save.GetGameSave())
		RequestBroker::Ref().RenderThumbnail(save.GetGameSave(), false, true, (Size.X/2)-16, -1, this);
}

ServerSaveActivity::ServerSaveActivity(SaveInfo save, bool saveNow, ServerSaveActivity::SaveUploadedCallback * callback) :
	WindowActivity(ui::Point(-1, -1), ui::Point(200, 50)),
	thumbnail(NULL),
	save(save),
	callback(callback),
	saveUploadTask(NULL)
{
	ui::Label * titleLabel = new ui::Label(ui::Point(0, 0), Size, TEXT_GUI_SAVE_WIN_SAVING_INFO);
	titleLabel->SetTextColour(style::Colour::InformationTitle);
	titleLabel->Appearance.HorizontalAlign = ui::Appearance::AlignCentre;
	titleLabel->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	AddComponent(titleLabel);

	AddAuthorInfo();

	saveUploadTask = new SaveUploadTask(this->save);
	saveUploadTask->AddTaskListener(this);
	saveUploadTask->Start();
}

void ServerSaveActivity::NotifyDone(Task * task)
{
	if(!task->GetSuccess())
	{
		Exit();
		new ErrorMessage(TEXT_ERR_TITLE, Client::Ref().GetWLastError());
	}
	else
	{
		if(callback)
		{
			callback->SaveUploaded(save);
		}
		Exit();
	}
}

void ServerSaveActivity::Save()
{
	class PublishConfirmation: public ConfirmDialogueCallback {
	public:
		ServerSaveActivity * a;
		PublishConfirmation(ServerSaveActivity * a) : a(a) {}
		virtual void ConfirmCallback(ConfirmPrompt::DialogueResult result) {
			if (result == ConfirmPrompt::ResultOkay)
			{
				a->Exit();
				a->saveUpload();
			}
		}
		virtual ~PublishConfirmation() { }
	};

	if(nameField->GetText().length())
	{
		if(Client::Ref().GetAuthUser().Username != save.GetUserName() && publishedCheckbox->GetChecked())
		{
			new ConfirmPrompt(TEXT_GUI_SAVE_WIN_PUB_CONF_TITLE, TEXT_GUI_SAVE_WIN_PUB_CONF_MSG1 + format::StringToWString(save.GetUserName()) + TEXT_GUI_SAVE_WIN_PUB_CONF_MSG2, new PublishConfirmation(this));
		}
		else
		{
			Exit();
			saveUpload();
		}
	}
	else
	{
		new ErrorMessage(TEXT_GUI_SAVE_WIN_NAME_ERR_TITLE, TEXT_GUI_SAVE_WIN_NAME_ERR_MSG);
	}
}

void ServerSaveActivity::AddAuthorInfo()
{
	Json::Value serverSaveInfo;
	serverSaveInfo["type"] = "save";
	serverSaveInfo["id"] = save.GetID();
	serverSaveInfo["username"] = Client::Ref().GetAuthUser().Username;
	serverSaveInfo["title"] = save.GetName();
	serverSaveInfo["description"] = save.GetDescription();
	serverSaveInfo["published"] = (int)save.GetPublished();
	serverSaveInfo["date"] = (Json::Value::UInt64)time(NULL);
	Client::Ref().SaveAuthorInfo(&serverSaveInfo);
	save.GetGameSave()->authors = serverSaveInfo;
}

void ServerSaveActivity::saveUpload()
{
	save.SetName(nameField->GetText());
	save.SetDescription(descriptionField->GetText());
	save.SetPublished(publishedCheckbox->GetChecked());
	save.SetUserName(Client::Ref().GetAuthUser().Username);
	save.SetID(0);
	save.GetGameSave()->paused = pausedCheckbox->GetChecked();
	AddAuthorInfo();

	if(Client::Ref().UploadSave(save) != RequestOkay)
	{
		new ErrorMessage(TEXT_GUI_SAVE_WIN_UPLOAD_ERR_TITLE, TEXT_GUI_SAVE_WIN_UPLOAD_ERR_MSG+Client::Ref().GetWLastError());
	}
	else if(callback)
	{
		new SaveIDMessage(save.GetID());
		callback->SaveUploaded(save);
	}
}

void ServerSaveActivity::Exit()
{
	WindowActivity::Exit();
}

void ServerSaveActivity::ShowPublishingInfo() 
{
	const wchar_t *info = TEXT_BLOCK_PUBLISH_INFO;

	new InformationMessage(TEXT_GUI_SAVE_WIN_PUBLISH_INFO_TITLE, info, true);
}

void ServerSaveActivity::ShowRules()
{
	const wchar_t *rules = TEXT_BLOCK_SAVE_RULES;
	new InformationMessage(TEXT_GUI_SAVE_WIN_UPLOAD_RULE_TITLE, rules, true);
}

void ServerSaveActivity::CheckName(std::string newname)
{
	if (newname.length() && newname == save.GetName() && save.GetUserName() == Client::Ref().GetAuthUser().Username)
		titleLabel->SetText(TEXT_GUI_SAVE_WIN_MODIFY_SAVE_TITLE);
	else
		titleLabel->SetText(TEXT_GUI_SAVE_WIN_NEW_SAVE_TITLE);
}

void ServerSaveActivity::OnTick(float dt)
{
	if(saveUploadTask)
		saveUploadTask->Poll();
}

void ServerSaveActivity::OnDraw()
{
	Graphics * g = GetGraphics();
	g->draw_rgba_image(save_to_server_image, -10, 0, 0.7f);
	g->clearrect(Position.X-2, Position.Y-2, Size.X+3, Size.Y+3);
	g->drawrect(Position.X, Position.Y, Size.X, Size.Y, 255, 255, 255, 255);

	if(Size.X>220)
		g->draw_line(Position.X+(Size.X/2)-1, Position.Y, Position.X+(Size.X/2)-1, Position.Y+Size.Y-1, 255, 255, 255, 255);

	if(thumbnail)
	{
		g->draw_image(thumbnail, Position.X+(Size.X/2)+((Size.X/2)-thumbnail->Width)/2, Position.Y+25, 255);
		g->drawrect(Position.X+(Size.X/2)+((Size.X/2)-thumbnail->Width)/2, Position.Y+25, thumbnail->Width, thumbnail->Height, 180, 180, 180, 255);
	}
}

void ServerSaveActivity::OnResponseReady(void * imagePtr, int identifier)
{
	delete thumbnail;
	thumbnail = (VideoBuffer *)imagePtr;
}

ServerSaveActivity::~ServerSaveActivity()
{
	RequestBroker::Ref().DetachRequestListener(this);
	delete saveUploadTask;
	delete callback;
	delete thumbnail;
}
