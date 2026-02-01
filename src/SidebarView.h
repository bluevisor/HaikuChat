#ifndef SIDEBAR_VIEW_H
#define SIDEBAR_VIEW_H

#include <Button.h>
#include <ListView.h>
#include <ObjectList.h>
#include <ScrollView.h>
#include <StringItem.h>
#include <View.h>

#include "ChatSession.h"

class ChatListItem : public BStringItem {
public:
						ChatListItem(ChatSession* session);
	virtual				~ChatListItem();

	virtual void		DrawItem(BView* owner, BRect frame, bool complete);
	virtual void		Update(BView* owner, const BFont* font);

	ChatSession*		Session() const { return fSession; }
	void				UpdateFromSession();

private:
	ChatSession*		fSession;
	BString				fTimeString;
};


class SidebarView : public BView {
public:
						SidebarView();
	virtual				~SidebarView();

	virtual void		AttachedToWindow();
	virtual void		MessageReceived(BMessage* message);
	virtual void		Draw(BRect updateRect);

	void				AddSession(ChatSession* session, bool atTop = true);
	void				RemoveSession(ChatSession* session);
	void				SelectSession(ChatSession* session);
	void				UpdateSession(ChatSession* session);
	void				ClearSessions();
	void				RefreshColors();

	ChatSession*		SelectedSession() const;
	int32				CountSessions() const;

private:
	void				_UpdateLayout();

	BButton*			fNewChatButton;
	BListView*			fChatList;
	BScrollView*		fScrollView;
	BView*				fUserButton;
	BObjectList<ChatSession> fSessions;
};

#endif // SIDEBAR_VIEW_H
