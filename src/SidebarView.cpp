#include "SidebarView.h"

#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <Window.h>
#include <ctime>

#include "Constants.h"
#include "Log.h"

// ChatListItem implementation

ChatListItem::ChatListItem(ChatSession* session)
	:
	BStringItem(session->Title()),
	fSession(session)
{
	UpdateFromSession();
}


ChatListItem::~ChatListItem()
{
}


void
ChatListItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	rgb_color bgColor;
	if (IsSelected())
		bgColor = kSidebarHoverColor;
	else
		bgColor = kSidebarColor;

	owner->SetHighColor(bgColor);
	owner->FillRect(frame);

	owner->SetLowColor(bgColor);

	BFont font;
	owner->GetFont(&font);
	font_height fh;
	font.GetHeight(&fh);

	float textY = frame.top + (frame.Height() - (fh.ascent + fh.descent)) / 2 + fh.ascent;

	// Calculate available width for title (leave space for time)
	float timeWidth = 0;
	if (fTimeString.Length() > 0)
		timeWidth = font.StringWidth(fTimeString.String()) + 16;

	float maxTitleWidth = frame.Width() - 24 - timeWidth;

	// Draw title with ellipsis if too long
	owner->SetHighColor(kSidebarTextColor);
	BString title(Text());
	float titleWidth = font.StringWidth(title.String());

	if (titleWidth > maxTitleWidth && maxTitleWidth > 20) {
		// Truncate with ellipsis
		while (titleWidth > maxTitleWidth - font.StringWidth("...") && title.Length() > 0) {
			title.Truncate(title.Length() - 1);
			titleWidth = font.StringWidth(title.String());
		}
		title.Append("...");
	}

	owner->MovePenTo(frame.left + 12, textY);
	owner->DrawString(title.String());

	// Draw time on right side (smaller, dimmer)
	if (fTimeString.Length() > 0) {
		rgb_color dimColor = kSidebarTextColor;
		dimColor.red = (uint8)(dimColor.red * 0.5);
		dimColor.green = (uint8)(dimColor.green * 0.5);
		dimColor.blue = (uint8)(dimColor.blue * 0.5);
		owner->SetHighColor(dimColor);
		owner->MovePenTo(frame.right - font.StringWidth(fTimeString.String()) - 12, textY);
		owner->DrawString(fTimeString.String());
	}
}


void
ChatListItem::Update(BView* owner, const BFont* font)
{
	BStringItem::Update(owner, font);
	SetHeight(kChatItemHeight);
}


void
ChatListItem::UpdateFromSession()
{
	SetText(fSession->Title());

	// Format time string
	time_t updated = fSession->UpdatedAt();
	time_t now = time(NULL);
	double diff = difftime(now, updated);

	if (diff < 60) {
		fTimeString = "now";
	} else if (diff < 3600) {
		int mins = static_cast<int>(diff / 60);
		fTimeString.SetToFormat("%dm", mins);
	} else if (diff < 86400) {
		int hours = static_cast<int>(diff / 3600);
		fTimeString.SetToFormat("%dh", hours);
	} else {
		int days = static_cast<int>(diff / 86400);
		fTimeString.SetToFormat("%dd", days);
	}
}


// SidebarView implementation

SidebarView::SidebarView()
	:
	BView("SidebarView", B_WILL_DRAW | B_FRAME_EVENTS),
	fNewChatButton(NULL),
	fChatList(NULL),
	fScrollView(NULL),
	fUserButton(NULL),
	fSessions(20, false)  // Don't own sessions, MainWindow owns them
{
	SetViewColor(kSidebarColor);
}


SidebarView::~SidebarView()
{
}


void
SidebarView::AttachedToWindow()
{
	BView::AttachedToWindow();

	LOG("SidebarView::AttachedToWindow - Creating UI components");

	// New Chat button - styled like ChatGPT
	fNewChatButton = new BButton("+ New chat", new BMessage(kMsgNewChat));
	fNewChatButton->SetTarget(Window());
	fNewChatButton->SetExplicitMinSize(BSize(B_SIZE_UNSET, 36));

	// Chat list
	fChatList = new BListView("ChatList", B_SINGLE_SELECTION_LIST);
	fChatList->SetSelectionMessage(new BMessage(kMsgSelectChat));
	fChatList->SetTarget(Window());
	fChatList->SetViewColor(kSidebarColor);
	fChatList->SetLowColor(kSidebarColor);
	fChatList->SetExplicitMinSize(BSize(100, 50));

	fScrollView = new BScrollView("ChatScroll", fChatList,
		B_FOLLOW_ALL_SIDES, B_WILL_DRAW, false, true, B_NO_BORDER);
	fScrollView->SetViewColor(kSidebarColor);
	fScrollView->SetExplicitMinSize(BSize(100, 50));

	// User button area
	fUserButton = new BView("UserButton", B_WILL_DRAW);
	fUserButton->SetViewColor(kSidebarColor);
	fUserButton->SetExplicitMinSize(BSize(B_SIZE_UNSET, 56));
	fUserButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 56));

	// Create layout
	BGroupLayout* layout = new BGroupLayout(B_VERTICAL, 0);
	SetLayout(layout);

	BGroupLayout* buttonGroup = new BGroupLayout(B_HORIZONTAL);
	buttonGroup->SetInsets(12, 12, 12, 8);
	buttonGroup->AddView(fNewChatButton);

	layout->AddItem(buttonGroup, 0.0f);
	layout->AddView(fScrollView, 1.0f);
	layout->AddView(fUserButton, 0.0f);

	// Populate chat list with sessions that were added before attachment
	// Sessions in fSessions are already ordered newest-first from Settings::LoadSessions
	// So we add them at the end to preserve that order in the list view
	LOG("SidebarView - Populating chat list with %d sessions", fSessions.CountItems());
	for (int32 i = 0; i < fSessions.CountItems(); i++) {
		ChatSession* session = fSessions.ItemAt(i);
		ChatListItem* item = new ChatListItem(session);
		fChatList->AddItem(item);  // Add at end to preserve order
		LOG("  Added to chat list: '%s'", session->Title());
	}

	// Force list to redraw
	if (fChatList->CountItems() > 0) {
		fChatList->Invalidate();
	}
}


void
SidebarView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BView::MessageReceived(message);
			break;
	}
}


void
SidebarView::Draw(BRect updateRect)
{
	BView::Draw(updateRect);

	// Draw user icon area
	if (fUserButton != NULL) {
		BRect userRect = fUserButton->Frame();

		// Draw separator line at top
		SetHighColor(kBorderColor);
		StrokeLine(BPoint(0, userRect.top), BPoint(Bounds().right, userRect.top));

		// Draw user icon (circle with "U")
		float iconX = 16;
		float iconY = userRect.top + (userRect.Height() - kUserIconSize) / 2;
		BRect iconRect(iconX, iconY, iconX + kUserIconSize, iconY + kUserIconSize);

		SetHighColor(kAccentColor);
		FillEllipse(iconRect);

		SetHighColor(kUserTextColor);
		BFont font(be_bold_font);
		font.SetSize(14);
		SetFont(&font);

		font_height fh;
		font.GetHeight(&fh);
		float textX = iconRect.left + (iconRect.Width() - font.StringWidth("U")) / 2;
		float textY = iconRect.top + (iconRect.Height() + fh.ascent - fh.descent) / 2;
		MovePenTo(textX, textY);
		DrawString("U");

		// Draw "User" label
		SetFont(be_plain_font);
		MovePenTo(iconRect.right + 10, textY);
		DrawString("User");
	}
}


void
SidebarView::AddSession(ChatSession* session, bool atTop)
{
	LOG("SidebarView::AddSession - Adding session: '%s' (atTop=%d)", session->Title(), atTop);

	if (atTop) {
		fSessions.AddItem(session, 0);  // Add to front of internal list
	} else {
		fSessions.AddItem(session);  // Add to end of internal list
	}

	// If not yet attached to window, the list item will be created in AttachedToWindow
	if (fChatList != NULL) {
		ChatListItem* item = new ChatListItem(session);
		if (atTop) {
			fChatList->AddItem(item, 0);  // Add at top of list view
			LOG("  Added to chat list at position 0");
		} else {
			fChatList->AddItem(item);  // Add at end of list view
			LOG("  Added to chat list at end");
		}
	} else {
		LOG("  Chat list not ready, will be populated in AttachedToWindow");
	}
}


void
SidebarView::RemoveSession(ChatSession* session)
{
	if (fChatList != NULL) {
		for (int32 i = 0; i < fChatList->CountItems(); i++) {
			ChatListItem* item = dynamic_cast<ChatListItem*>(fChatList->ItemAt(i));
			if (item != NULL && item->Session() == session) {
				fChatList->RemoveItem(item);
				delete item;
				break;
			}
		}
	}
	fSessions.RemoveItem(session);
}


void
SidebarView::SelectSession(ChatSession* session)
{
	if (fChatList == NULL) {
		LOG("SidebarView::SelectSession - Chat list not ready yet");
		return;
	}

	for (int32 i = 0; i < fChatList->CountItems(); i++) {
		ChatListItem* item = dynamic_cast<ChatListItem*>(fChatList->ItemAt(i));
		if (item != NULL && item->Session() == session) {
			fChatList->Select(i);
			LOG("SidebarView::SelectSession - Selected '%s' at index %d",
				session->Title(), i);
			break;
		}
	}
}


void
SidebarView::UpdateSession(ChatSession* session)
{
	if (fChatList == NULL)
		return;

	for (int32 i = 0; i < fChatList->CountItems(); i++) {
		ChatListItem* item = dynamic_cast<ChatListItem*>(fChatList->ItemAt(i));
		if (item != NULL && item->Session() == session) {
			item->UpdateFromSession();
			fChatList->InvalidateItem(i);
			break;
		}
	}
}


void
SidebarView::ClearSessions()
{
	if (fChatList != NULL)
		fChatList->MakeEmpty();
	fSessions.MakeEmpty();
}


ChatSession*
SidebarView::SelectedSession() const
{
	if (fChatList == NULL)
		return NULL;

	int32 index = fChatList->CurrentSelection();
	if (index < 0)
		return NULL;

	ChatListItem* item = dynamic_cast<ChatListItem*>(fChatList->ItemAt(index));
	if (item == NULL)
		return NULL;

	return item->Session();
}


int32
SidebarView::CountSessions() const
{
	return fSessions.CountItems();
}


void
SidebarView::_UpdateLayout()
{
	// Layout is handled by BLayoutBuilder
}


void
SidebarView::RefreshColors()
{
	SetViewColor(kSidebarColor);

	if (fChatList != NULL) {
		fChatList->SetViewColor(kSidebarColor);
		fChatList->SetLowColor(kSidebarColor);
		fChatList->Invalidate();
	}

	if (fScrollView != NULL) {
		fScrollView->SetViewColor(kSidebarColor);
	}

	if (fUserButton != NULL) {
		fUserButton->SetViewColor(kSidebarColor);
	}

	Invalidate();
}
