#include "client/Client.h"
#include "TagsView.h"

#include "gui/dialogues/ErrorMessage.h"
#include "TagsController.h"
#include "TagsModel.h"
#include "TagsModelException.h"

#include "gui/interface/Button.h"
#include "gui/interface/Textbox.h"
#include "gui/interface/Label.h"
#include "gui/interface/Keys.h"

#include "Format.h"
#include "Lang.h"

TagsView::TagsView():
	ui::Window(ui::Point(-1, -1), ui::Point(195, 250))
{

	class CloseAction : public ui::ButtonAction
	{
		TagsView * v;
	public:
		CloseAction(TagsView * _v) { v = _v; }
		void ActionCallback(ui::Button * sender)
		{
			v->c->Exit();
		}
	};
	closeButton = new ui::Button(ui::Point(0, Size.Y-16), ui::Point(195, 16), TEXT_GUI_TAG_BTN_CLOSE);
	closeButton->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	closeButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	closeButton->SetActionCallback(new CloseAction(this));
	AddComponent(closeButton);
	SetCancelButton(closeButton);


	tagInput = new ui::Textbox(ui::Point(8, Size.Y-40), ui::Point(Size.X-64, 16), L"", TEXT_GUI_TAG_TBOX_HOLDER);
	tagInput->Appearance.icon = IconTag;
	tagInput->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	tagInput->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	AddComponent(tagInput);

	class AddTagAction : public ui::ButtonAction
	{
		TagsView * v;
	public:
		AddTagAction(TagsView * _v) { v = _v; }
		void ActionCallback(ui::Button * sender)
		{
			v->addTag();
		}
	};
	addButton = new ui::Button(ui::Point(tagInput->Position.X+tagInput->Size.X+4, tagInput->Position.Y), ui::Point(44, 16), TEXT_GUI_TAG_BTN_ADD);
	addButton->Appearance.icon = IconAdd;
	addButton->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	addButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	addButton->SetActionCallback(new AddTagAction(this));
	AddComponent(addButton);

	if (!Client::Ref().GetAuthUser().ID)
		addButton->Enabled = false;

	title = new ui::Label(ui::Point(5, 5), ui::Point(185, 28), TEXT_GUI_TAG_LABEL);
	title->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	title->Appearance.VerticalAlign = ui::Appearance::AlignTop;
	title->SetMultiline(true);
	AddComponent(title);
}

void TagsView::OnDraw()
{
	Graphics * g = ui::Engine::Ref().g;
	g->clearrect(Position.X-2, Position.Y-2, Size.X+3, Size.Y+3);
	g->drawrect(Position.X, Position.Y, Size.X, Size.Y, 255, 255, 255, 255);
}

void TagsView::NotifyTagsChanged(TagsModel * sender)
{
	for (size_t i = 0; i < tags.size(); i++)
	{
		RemoveComponent(tags[i]);
		delete tags[i];
	}
	tags.clear();


	class DeleteTagAction : public ui::ButtonAction
	{
		TagsView * v;
		std::string tag;
	public:
		DeleteTagAction(TagsView * _v, std::string tag) { v = _v; this->tag = tag; }
		void ActionCallback(ui::Button * sender)
		{
			try
			{
				v->c->RemoveTag(tag);
			}
			catch(TagsModelException & ex)
			{
				new ErrorMessage(TEXT_GUI_TAG_REMOVE_ERR_TITLE, format::StringToWString(ex.what()));
			}
		}
	};

	if(sender->GetSave())
	{
		std::list<std::string> Tags = sender->GetSave()->GetTags();
		int i = 0;
		for(std::list<std::string>::const_iterator iter = Tags.begin(), end = Tags.end(); iter != end; iter++)
		{
			ui::Label * tempLabel = new ui::Label(ui::Point(35, 35+(16*i)), ui::Point(120, 16), *iter);
			tempLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;			tempLabel->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
			tags.push_back(tempLabel);
			AddComponent(tempLabel);

			if(sender->GetSave()->GetUserName() == Client::Ref().GetAuthUser().Username || Client::Ref().GetAuthUser().UserElevation == User::ElevationAdmin || Client::Ref().GetAuthUser().UserElevation == User::ElevationModerator)
			{
				ui::Button * tempButton = new ui::Button(ui::Point(15, 37 + (16 * i)), ui::Point(11, 12), L"");
				tempButton->Appearance.icon = IconDelete;
				tempButton->Appearance.Border = ui::Border(0);
				tempButton->Appearance.Margin.Top += 2;
				tempButton->Appearance.HorizontalAlign = ui::Appearance::AlignCentre;	
				tempButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
				tempButton->SetActionCallback(new DeleteTagAction(this, *iter));
				tags.push_back(tempButton);
				AddComponent(tempButton);
			}
			i++;
		}
	}
}

void TagsView::OnKeyPress(int key, Uint16 character, bool shift, bool ctrl, bool alt)
{
	switch(key)
	{
	case KEY_ENTER:
	case KEY_RETURN:
		if(IsFocused(tagInput))
		{
			addTag();
		}
		break;
	}
}

void TagsView::addTag()
{
	if (tagInput->GetText().length() < 4)
	{
		new ErrorMessage(TEXT_GUI_TAG_LENGTH_ERR_TITLE, TEXT_GUI_TAG_LENGTH_ERR_MSG);
		return;
	}
	try
	{
		c->AddTag(tagInput->GetText());
	}
	catch(TagsModelException & ex)
	{
		new ErrorMessage(TEXT_GUI_TAG_ADD_ERR_TITLE, format::StringToWString(ex.what()));
	}
	tagInput->SetText("");
}

TagsView::~TagsView() {
}

