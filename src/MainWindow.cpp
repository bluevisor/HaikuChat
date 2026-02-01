#include "MainWindow.h"

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <SeparatorView.h>

#include "Constants.h"
#include "Log.h"
#include "SettingsWindow.h"

MainWindow::MainWindow(Settings* settings)
	:
	BWindow(settings->GetWindowFrame(), "HaikuChat",
		B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS),
	fSettings(settings),
	fTopBar(NULL),
	fToggleButton(NULL),
	fTitleView(NULL),
	fSettingsButton(NULL),
	fSplitView(NULL),
	fSidebarView(NULL),
	fMainView(NULL),
	fChatScrollView(NULL),
	fChatView(NULL),
	fInputView(NULL),
	fLLMClient(NULL),
	fCurrentAssistantMessage(NULL),
	fIsWaitingForResponse(false)
{
	_BuildUI();

	// Create LLM client
	fLLMClient = new LLMClient(BMessenger(this));

	// Load sessions into sidebar
	_LoadSessions();
}


MainWindow::~MainWindow()
{
	if (fLLMClient != NULL) {
		fLLMClient->Lock();
		fLLMClient->Quit();
	}
}


void
MainWindow::_BuildUI()
{
	// === Top Bar ===
	fTopBar = new BView("TopBar", B_WILL_DRAW);
	fTopBar->SetViewColor(kSidebarColor);
	fTopBar->SetExplicitMinSize(BSize(B_SIZE_UNSET, kTopBarHeight));
	fTopBar->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, kTopBarHeight));

	fToggleButton = new BButton("≡", new BMessage(kMsgToggleSidebar));
	fToggleButton->SetExplicitSize(BSize(36, 28));

	fTitleView = new BStringView("Title", "HaikuChat");
	fTitleView->SetHighColor(kSidebarTextColor);
	fTitleView->SetFont(be_bold_font);

	fSettingsButton = new BButton("⚙", new BMessage(kMsgShowSettings));
	fSettingsButton->SetExplicitSize(BSize(36, 28));

	BLayoutBuilder::Group<>(fTopBar, B_HORIZONTAL, 8)
		.SetInsets(8, 6, 8, 6)
		.Add(fToggleButton)
		.Add(fTitleView)
		.AddGlue()
		.Add(fSettingsButton)
		.End();

	// === Sidebar ===
	fSidebarView = new SidebarView();
	fSidebarView->SetExplicitMinSize(BSize(kSidebarWidth, B_SIZE_UNSET));
	fSidebarView->SetExplicitMaxSize(BSize(kSidebarWidth, B_SIZE_UNLIMITED));

	// === Main Chat Area ===
	fMainView = new BView("MainView", B_WILL_DRAW | B_FRAME_EVENTS);
	fMainView->SetViewColor(kBackgroundColor);

	fChatView = new ChatView();
	fChatScrollView = new ChatScrollView(fChatView);
	fChatScrollView->SetExplicitMinSize(BSize(200, 100));

	fInputView = new InputView();
	fInputView->SetExplicitMinSize(BSize(200, kInputMinHeight));
	fInputView->SetExplicitPreferredSize(BSize(B_SIZE_UNSET, kInputMinHeight));
	fInputView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, kInputMaxHeight));

	BGroupLayout* mainLayout = new BGroupLayout(B_VERTICAL, 0);
	fMainView->SetLayout(mainLayout);
	mainLayout->AddView(fChatScrollView, 1.0f);
	mainLayout->AddView(fInputView, 0.0f);

	// === Split View (Sidebar + Main) ===
	fSplitView = new BSplitView(B_HORIZONTAL, 0);
	fSplitView->SetViewColor(kBackgroundColor);

	BLayoutBuilder::Split<>(fSplitView)
		.Add(fSidebarView, 0.0f)
		.Add(fMainView, 1.0f)
		.SetCollapsible(0, true)
		.SetCollapsible(1, false)
		.End();

	// Apply collapsed state
	if (fSettings->IsSidebarCollapsed()) {
		fSplitView->SetItemCollapsed(0, true);
	}

	// === Main Layout ===
	BGroupLayout* layout = new BGroupLayout(B_VERTICAL, 0);
	SetLayout(layout);

	layout->AddView(fTopBar);
	layout->AddView(fSplitView, 1.0f);

	// Set minimum window size
	SetSizeLimits(400, 10000, 300, 10000);
}


void
MainWindow::_LoadSessions()
{
	BObjectList<ChatSession>& sessions = fSettings->GetSessions();

	LOG("MainWindow::_LoadSessions - Loading %d sessions into sidebar",
		sessions.CountItems());

	for (int32 i = 0; i < sessions.CountItems(); i++) {
		ChatSession* session = sessions.ItemAt(i);
		LOG("  Adding session to sidebar: '%s' (ID: %s)",
			session->Title(), session->Id());
		fSidebarView->AddSession(session, false);  // Load at end, preserve order
	}

	// Select current session or create new one
	ChatSession* current = fSettings->GetCurrentSession();
	if (current != NULL) {
		LOG("Selecting current session: '%s'", current->Title());
		fSidebarView->SelectSession(current);
		_UpdateChatView();
	} else if (sessions.CountItems() == 0) {
		LOG("No sessions found, creating new chat");
		_NewChat();
	} else {
		LOG("No current session set, selecting first session");
		fSettings->SetCurrentSession(sessions.ItemAt(0));
		fSidebarView->SelectSession(sessions.ItemAt(0));
		_UpdateChatView();
	}
}


void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgSendMessage:
			_SendMessage();
			break;

		case kMsgNewChat:
			_NewChat();
			break;

		case kMsgSelectChat:
		{
			ChatSession* session = fSidebarView->SelectedSession();
			if (session != NULL)
				_SelectChat(session);
			break;
		}

		case kMsgDeleteChat:
		{
			ChatSession* session = fSidebarView->SelectedSession();
			if (session != NULL)
				_DeleteChat(session);
			break;
		}

		case kMsgToggleSidebar:
			_ToggleSidebar();
			break;

		case kMsgShowSettings:
			_ShowSettings();
			break;

		case kMsgShowAbout:
			_ShowAbout();
			break;

		case kMsgLLMChunk:
		{
			const char* text;
			if (message->FindString("text", &text) == B_OK) {
				if (fCurrentAssistantMessage != NULL) {
					fCurrentAssistantMessage->AppendContent(text);
					fChatView->UpdateLastMessage();
				}
			}
			break;
		}

		case kMsgLLMDone:
		{
			fIsWaitingForResponse = false;
			fInputView->SetEnabled(true);
			fInputView->MakeFocus(true);

			// Save session
			ChatSession* session = fSettings->GetCurrentSession();
			if (session != NULL) {
				fSettings->SaveSession(session);
				fSidebarView->UpdateSession(session);
			}
			break;
		}

		case kMsgLLMError:
		{
			const char* error;
			if (message->FindString("error", &error) == B_OK) {
				LOG_ERROR("LLM error received: %s", error);

				// Build a more helpful error message
				BString errorText(error);
				BString helpText;

				if (errorText.FindFirst("API key") >= 0 ||
					errorText.FindFirst("api_key") >= 0 ||
					errorText.FindFirst("Unauthorized") >= 0 ||
					errorText.FindFirst("401") >= 0) {
					helpText = "\n\nPlease check your API key in Settings.";
				} else if (errorText.FindFirst("model") >= 0 ||
						   errorText.FindFirst("does not exist") >= 0) {
					helpText = "\n\nThe selected model may not be available. Try fetching models in Settings.";
				} else if (errorText.FindFirst("rate") >= 0 ||
						   errorText.FindFirst("limit") >= 0 ||
						   errorText.FindFirst("quota") >= 0) {
					helpText = "\n\nYou may have exceeded your API rate limit or quota.";
				} else if (errorText.FindFirst("network") >= 0 ||
						   errorText.FindFirst("connection") >= 0 ||
						   errorText.FindFirst("failed") >= 0) {
					helpText = "\n\nPlease check your internet connection.";
				}

				BString fullError = errorText;
				fullError.Append(helpText);

				BAlert* alert = new BAlert("API Error", fullError.String(), "OK",
					NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
				alert->Go();
			}
			fIsWaitingForResponse = false;
			fInputView->SetEnabled(true);
			fInputView->MakeFocus(true);
			break;
		}

		case kMsgInputChanged:
			// Input height changed - just invalidate the main view layout
			if (fMainView != NULL && fMainView->GetLayout() != NULL) {
				fMainView->GetLayout()->InvalidateLayout();
			}
			break;

		case kMsgThemeChanged:
			_RefreshTheme();
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
MainWindow::QuitRequested()
{
	fSettings->SetWindowFrame(Frame());
	fSettings->SetSidebarCollapsed(fSplitView->IsItemCollapsed(0));
	fSettings->Save();

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
MainWindow::_SendMessage()
{
	if (fIsWaitingForResponse) {
		LOG("_SendMessage - Already waiting for response, ignoring");
		return;
	}

	const char* text = fInputView->Text();
	if (text == NULL || text[0] == '\0') {
		LOG("_SendMessage - Empty message, ignoring");
		return;
	}

	LOG("_SendMessage - Sending message: %.50s%s", text, strlen(text) > 50 ? "..." : "");

	ChatSession* session = fSettings->GetCurrentSession();
	if (session == NULL) {
		LOG("No current session, creating new one");
		session = fSettings->CreateNewSession();
		fSidebarView->AddSession(session);
		fSidebarView->SelectSession(session);
	}

	// Add user message
	ChatMessage* userMsg = new ChatMessage(kRoleUser, text);
	session->AddMessage(userMsg);
	fChatView->AddMessage(userMsg);

	// Clear input
	fInputView->SetText("");

	// Create placeholder for assistant response
	fCurrentAssistantMessage = new ChatMessage(kRoleAssistant, "");
	session->AddMessage(fCurrentAssistantMessage);
	fChatView->AddMessage(fCurrentAssistantMessage);

	// Disable input while waiting
	fIsWaitingForResponse = true;
	fInputView->SetEnabled(false);

	// Update sidebar with new title if needed
	fSidebarView->UpdateSession(session);

	// Build messages JSON and send request
	BString messagesJson = _BuildMessagesJson();
	fLLMClient->SendChatRequest(
		messagesJson.String(),
		fSettings->GetApiType(),
		fSettings->GetApiEndpoint(),
		fSettings->GetApiKey(),
		fSettings->GetModel()
	);
}


void
MainWindow::_NewChat()
{
	ChatSession* session = fSettings->CreateNewSession();
	fSidebarView->AddSession(session);
	fSidebarView->SelectSession(session);
	_UpdateChatView();
	fInputView->MakeFocus(true);
}


void
MainWindow::_SelectChat(ChatSession* session)
{
	if (session == fSettings->GetCurrentSession())
		return;

	fSettings->SetCurrentSession(session);
	_UpdateChatView();
}


void
MainWindow::_DeleteChat(ChatSession* session)
{
	fSidebarView->RemoveSession(session);
	fSettings->DeleteSession(session);

	// Update view
	ChatSession* current = fSettings->GetCurrentSession();
	if (current != NULL) {
		fSidebarView->SelectSession(current);
		_UpdateChatView();
	} else {
		fChatView->ClearMessages();
	}
}


void
MainWindow::_ToggleSidebar()
{
	bool collapsed = fSplitView->IsItemCollapsed(0);
	fSplitView->SetItemCollapsed(0, !collapsed);
	fSettings->SetSidebarCollapsed(!collapsed);
}


void
MainWindow::_ShowSettings()
{
	SettingsWindow* window = new SettingsWindow(Frame(), fSettings);
	window->Show();
}


void
MainWindow::_ShowAbout()
{
	BAlert* alert = new BAlert("About HaikuChat",
		"HaikuChat\n\n"
		"A native AI chat client for Haiku.\n"
		"Supports OpenAI, Claude, and Gemini APIs.\n\n"
		"Version 1.0",
		"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT);
	alert->Go();
}


void
MainWindow::_UpdateChatView()
{
	fChatView->ClearMessages();
	fCurrentAssistantMessage = NULL;

	ChatSession* session = fSettings->GetCurrentSession();
	if (session == NULL)
		return;

	const BObjectList<ChatMessage>& messages = session->Messages();
	for (int32 i = 0; i < messages.CountItems(); i++) {
		ChatMessage* msg = messages.ItemAt(i);
		fChatView->AddMessage(msg);

		if (msg->Role() == kRoleAssistant)
			fCurrentAssistantMessage = msg;
	}

	// Update title
	fTitleView->SetText(session->Title());
}


BString
MainWindow::_BuildMessagesJson()
{
	BString json = "[";

	ChatSession* session = fSettings->GetCurrentSession();
	if (session == NULL)
		return "[]";

	const BObjectList<ChatMessage>& messages = session->Messages();
	for (int32 i = 0; i < messages.CountItems(); i++) {
		ChatMessage* msg = messages.ItemAt(i);

		// Skip empty assistant messages (placeholders)
		if (msg->Role() == kRoleAssistant && msg->Content()[0] == '\0')
			continue;

		if (json.Length() > 1)
			json.Append(",");

		const char* role;
		switch (msg->Role()) {
			case kRoleUser:
				role = "user";
				break;
			case kRoleAssistant:
				role = "assistant";
				break;
			case kRoleSystem:
				role = "system";
				break;
			default:
				role = "user";
		}

		// Escape content for JSON
		BString content = msg->Content();
		content.ReplaceAll("\\", "\\\\");
		content.ReplaceAll("\"", "\\\"");
		content.ReplaceAll("\n", "\\n");
		content.ReplaceAll("\r", "\\r");
		content.ReplaceAll("\t", "\\t");

		BString msgJson;
		msgJson.SetToFormat("{\"role\":\"%s\",\"content\":\"%s\"}",
			role, content.String());
		json.Append(msgJson);
	}

	json.Append("]");
	return json;
}


void
MainWindow::_RefreshTheme()
{
	// Update view colors for theme change
	fTopBar->SetViewColor(kSidebarColor);
	fTitleView->SetHighColor(kSidebarTextColor);
	fMainView->SetViewColor(kBackgroundColor);
	fSplitView->SetViewColor(kBackgroundColor);
	fChatScrollView->SetViewColor(kBackgroundColor);
	fChatView->SetViewColor(B_TRANSPARENT_COLOR);

	// Refresh sidebar
	fSidebarView->RefreshColors();

	// Refresh input view
	fInputView->RefreshColors();

	// Refresh chat - need to recreate bubbles with new colors
	_UpdateChatView();

	// Force full window redraw
	fTopBar->Invalidate();
	fMainView->Invalidate();
}
