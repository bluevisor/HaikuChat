#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <Button.h>
#include <MenuBar.h>
#include <SplitView.h>
#include <StringView.h>
#include <Window.h>

#include "ChatSession.h"
#include "ChatView.h"
#include "InputView.h"
#include "LLMClient.h"
#include "Settings.h"
#include "SidebarView.h"

class MainWindow : public BWindow {
public:
						MainWindow(Settings* settings);
	virtual				~MainWindow();

	virtual void		MessageReceived(BMessage* message);
	virtual bool		QuitRequested();

private:
	void				_BuildUI();
	void				_LoadSessions();
	void				_SendMessage();
	void				_NewChat();
	void				_SelectChat(ChatSession* session);
	void				_DeleteChat(ChatSession* session);
	void				_ToggleSidebar();
	void				_ShowSettings();
	void				_ShowAbout();
	void				_UpdateChatView();
	void				_RefreshTheme();
	BString				_BuildMessagesJson();

	Settings*			fSettings;

	// UI components
	BView*				fTopBar;
	BButton*			fToggleButton;
	BStringView*		fTitleView;
	BButton*			fSettingsButton;

	BSplitView*			fSplitView;
	SidebarView*		fSidebarView;

	BView*				fMainView;
	ChatScrollView*		fChatScrollView;
	ChatView*			fChatView;
	InputView*			fInputView;

	// LLM
	LLMClient*			fLLMClient;
	ChatMessage*		fCurrentAssistantMessage;
	bool				fIsWaitingForResponse;
};

#endif // MAIN_WINDOW_H
