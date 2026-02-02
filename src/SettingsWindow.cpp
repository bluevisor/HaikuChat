#include "SettingsWindow.h"

#include <Alert.h>
#include <Application.h>
#include <LayoutBuilder.h>
#include <MenuItem.h>
#include <SeparatorView.h>

#include "Constants.h"

SettingsWindow::SettingsWindow(BRect frame, Settings* settings)
	:
	BWindow(BRect(frame.left + 50, frame.top + 50, frame.left + 700, frame.top + 500),
		"Settings", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_CLOSE_ON_ESCAPE),
	fSettings(settings),
	fLLMClient(NULL),
	fCurrentApiType(settings->GetApiType())
{
	// Create LLM client for fetching models
	fLLMClient = new LLMClient(BMessenger(this));

	// API Type menu
	fApiTypeMenu = new BPopUpMenu("API Type");
	BMessage* openaiMsg = new BMessage(kMsgApiTypeChanged);
	openaiMsg->AddInt32("type", kApiTypeOpenAI);
	fApiTypeMenu->AddItem(new BMenuItem("OpenAI-compatible", openaiMsg));

	BMessage* claudeMsg = new BMessage(kMsgApiTypeChanged);
	claudeMsg->AddInt32("type", kApiTypeClaude);
	fApiTypeMenu->AddItem(new BMenuItem("Claude (Anthropic)", claudeMsg));

	BMessage* geminiMsg = new BMessage(kMsgApiTypeChanged);
	geminiMsg->AddInt32("type", kApiTypeGemini);
	fApiTypeMenu->AddItem(new BMenuItem("Gemini (Google)", geminiMsg));

	fApiTypeField = new BMenuField("API Type:", fApiTypeMenu);

	// Text fields
	fEndpointField = new BTextControl("Endpoint:", "", NULL);
	fEndpointField->SetModificationMessage(new BMessage('endp'));

	fApiKeyField = new BTextControl("API Key:", "", NULL);
	fApiKeyField->TextView()->HideTyping(true);

	// Model dropdown
	fModelMenu = new BPopUpMenu("Select Model");
	fModelField = new BMenuField("Model:", fModelMenu);

	// Fetch models button
	fFetchModelsButton = new BButton("Fetch Models", new BMessage(kMsgFetchModels));

	// Theme checkbox
	fDarkThemeCheckbox = new BCheckBox("Dark theme", new BMessage(kMsgThemeChanged));
	fDarkThemeCheckbox->SetValue(fSettings->IsDarkTheme() ? B_CONTROL_ON : B_CONTROL_OFF);

	// Status view
	fStatusView = new BStringView("status", "");
	fStatusView->SetExplicitMinSize(BSize(200, B_SIZE_UNSET));

	// Buttons
	fResetButton = new BButton("Reset to Defaults", new BMessage(kMsgSettingsReset));
	fSaveButton = new BButton("Save", new BMessage(kMsgSettingsSave));
	fCancelButton = new BButton("Cancel", new BMessage(kMsgSettingsCancel));

	// Layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, 10)
		.SetInsets(15)
		.AddGrid(10, 8)
			.Add(fApiTypeField->CreateLabelLayoutItem(), 0, 0)
			.Add(fApiTypeField->CreateMenuBarLayoutItem(), 1, 0, 2)
			.Add(fEndpointField->CreateLabelLayoutItem(), 0, 1)
			.Add(fEndpointField->CreateTextViewLayoutItem(), 1, 1, 2)
			.Add(fApiKeyField->CreateLabelLayoutItem(), 0, 2)
			.Add(fApiKeyField->CreateTextViewLayoutItem(), 1, 2, 2)
			.Add(fModelField->CreateLabelLayoutItem(), 0, 3)
			.Add(fModelField->CreateMenuBarLayoutItem(), 1, 3)
			.Add(fFetchModelsButton, 2, 3)
		.End()
		.Add(fStatusView)
		.AddStrut(10)
		.Add(new BSeparatorView(B_HORIZONTAL))
		.AddStrut(5)
		.Add(fDarkThemeCheckbox)
		.AddGlue()
		.Add(new BSeparatorView(B_HORIZONTAL))
		.AddGroup(B_HORIZONTAL)
			.Add(fResetButton)
			.AddGlue()
			.Add(fCancelButton)
			.Add(fSaveButton)
		.End()
	.End();

	// Load current settings
	fApiTypeMenu->ItemAt(fSettings->GetApiType())->SetMarked(true);
	fEndpointField->SetText(fSettings->GetApiEndpointFor(fCurrentApiType));
	fApiKeyField->SetText(fSettings->GetApiKeyFor(fCurrentApiType));

	// Load cached models for current API type
	_LoadCachedModels();

	fSaveButton->MakeDefault(true);
}


SettingsWindow::~SettingsWindow()
{
	if (fLLMClient != NULL) {
		fLLMClient->Lock();
		fLLMClient->Quit();
	}
}


void
SettingsWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgApiTypeChanged:
		{
			int32 type;
			if (message->FindInt32("type", &type) == B_OK) {
				_UpdateFieldsForApiType(static_cast<ApiType>(type));
			}
			break;
		}

		case kMsgThemeChanged:
			// Theme change will be applied on save
			break;

		case kMsgFetchModels:
			_FetchModels();
			break;

		case kMsgModelsReceived:
			_PopulateModels(message);
			break;

		case kMsgModelSelected:
			// Model selection handled by menu
			break;

		case kMsgLLMError:
		{
			const char* error;
			if (message->FindString("error", &error) == B_OK) {
				BString status("Error: ");
				status.Append(error);
				fStatusView->SetText(status.String());
			}
			fFetchModelsButton->SetEnabled(true);
			break;
		}

		case kMsgSettingsSave:
			_SaveSettings();
			Quit();
			break;

		case kMsgSettingsReset:
			_ResetSettings();
			break;

		case kMsgSettingsCancel:
			Quit();
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
SettingsWindow::_SaveSettings()
{
	// Save current field values to the current API type's slots
	fSettings->SetApiEndpointFor(fCurrentApiType, fEndpointField->Text());
	fSettings->SetApiKeyFor(fCurrentApiType, fApiKeyField->Text());

	// Get selected model for current type
	BMenuItem* modelItem = fModelMenu->FindMarked();
	if (modelItem != NULL) {
		fSettings->SetModelFor(fCurrentApiType, modelItem->Label());
	}

	// Determine API type from menu and set as active
	BMenuItem* selected = fApiTypeMenu->FindMarked();
	if (selected != NULL) {
		int32 index = fApiTypeMenu->IndexOf(selected);
		fSettings->SetApiType(static_cast<ApiType>(index));
	}

	// Save theme setting and check if it changed
	bool darkTheme = (fDarkThemeCheckbox->Value() == B_CONTROL_ON);
	bool themeChanged = (darkTheme != fSettings->IsDarkTheme());
	fSettings->SetDarkTheme(darkTheme);
	SetDarkTheme(darkTheme);

	fSettings->Save();

	// Notify all windows of theme change
	if (themeChanged) {
		// Broadcast theme change to all windows
		for (int32 i = 0; i < be_app->CountWindows(); i++) {
			BWindow* window = be_app->WindowAt(i);
			if (window != this) {
				window->PostMessage(kMsgThemeChanged);
			}
		}
	}
}


void
SettingsWindow::_ResetSettings()
{
	// Clear API keys and endpoints for current type
	fSettings->SetApiEndpointFor(fCurrentApiType, "");
	fSettings->SetApiKeyFor(fCurrentApiType, "");
	fSettings->SetModelFor(fCurrentApiType, "");

	// Update UI fields to reflect reset
	fEndpointField->SetText("");
	fApiKeyField->SetText("");

	// Clear and reload cached models (which will now be empty)
	_LoadCachedModels();

	fStatusView->SetText("Settings reset to defaults");
}


void
SettingsWindow::_UpdateFieldsForApiType(ApiType newType)
{
	if (newType == fCurrentApiType)
		return;

	// Save current field values to the old API type's slots
	fSettings->SetApiEndpointFor(fCurrentApiType, fEndpointField->Text());
	fSettings->SetApiKeyFor(fCurrentApiType, fApiKeyField->Text());

	// Save selected model for current type
	BMenuItem* modelItem = fModelMenu->FindMarked();
	if (modelItem != NULL) {
		fSettings->SetModelFor(fCurrentApiType, modelItem->Label());
	}

	// Switch to new API type
	fCurrentApiType = newType;

	// Load the new API type's settings
	fEndpointField->SetText(fSettings->GetApiEndpointFor(newType));
	fApiKeyField->SetText(fSettings->GetApiKeyFor(newType));

	// Load cached models for new API type
	_LoadCachedModels();
}


void
SettingsWindow::_FetchModels()
{
	const char* endpoint = fEndpointField->Text();
	const char* apiKey = fApiKeyField->Text();

	if (endpoint == NULL || endpoint[0] == '\0') {
		fStatusView->SetText("Please enter an endpoint URL");
		return;
	}

	if (apiKey == NULL || apiKey[0] == '\0') {
		fStatusView->SetText("Please enter an API key");
		return;
	}

	fStatusView->SetText("Fetching models...");
	fFetchModelsButton->SetEnabled(false);

	fLLMClient->FetchModels(fCurrentApiType, endpoint, apiKey);
}


void
SettingsWindow::_PopulateModels(BMessage* message)
{
	// Clear existing items
	while (fModelMenu->CountItems() > 0)
		delete fModelMenu->RemoveItem((int32)0);

	// Get current model from settings to preserve selection
	BString currentModel(fSettings->GetModelFor(fCurrentApiType));

	// Use current API type for caching
	ApiType apiType = fCurrentApiType;

	// Build list for caching
	BObjectList<BString> modelList(20, true);

	// Add models from message
	const char* model;
	int32 count = 0;
	bool foundCurrent = false;

	for (int32 i = 0; message->FindString("model", i, &model) == B_OK; i++) {
		BMessage* modelMsg = new BMessage(kMsgModelSelected);
		modelMsg->AddString("model", model);
		BMenuItem* item = new BMenuItem(model, modelMsg);

		if (currentModel == model) {
			item->SetMarked(true);
			foundCurrent = true;
		}

		fModelMenu->AddItem(item);
		modelList.AddItem(new BString(model));
		count++;
	}

	// Cache the models
	fSettings->SetCachedModels(apiType, modelList);

	// Select first item if no current selection
	if (!foundCurrent && fModelMenu->CountItems() > 0) {
		fModelMenu->ItemAt(0)->SetMarked(true);
	}

	BString status;
	status.SetToFormat("Found %d models (cached)", count);
	fStatusView->SetText(status.String());
	fFetchModelsButton->SetEnabled(true);
}


void
SettingsWindow::_LoadCachedModels()
{
	// Clear existing items
	while (fModelMenu->CountItems() > 0)
		delete fModelMenu->RemoveItem((int32)0);

	// Use current API type
	ApiType apiType = fCurrentApiType;

	// Get current model from settings for this API type
	BString currentModel(fSettings->GetModelFor(apiType));
	bool foundCurrent = false;

	// Check if we have cached models
	if (fSettings->HasCachedModels(apiType)) {
		const BObjectList<BString>& models = fSettings->GetCachedModels(apiType);
		for (int32 i = 0; i < models.CountItems(); i++) {
			BMessage* modelMsg = new BMessage(kMsgModelSelected);
			modelMsg->AddString("model", models.ItemAt(i)->String());
			BMenuItem* item = new BMenuItem(models.ItemAt(i)->String(), modelMsg);

			if (currentModel == *models.ItemAt(i)) {
				item->SetMarked(true);
				foundCurrent = true;
			}

			fModelMenu->AddItem(item);
		}

		BString status;
		status.SetToFormat("%d models (from cache)", models.CountItems());
		fStatusView->SetText(status.String());
	} else {
		// No cache - add current model if set
		if (currentModel.Length() > 0) {
			BMessage* modelMsg = new BMessage(kMsgModelSelected);
			modelMsg->AddString("model", currentModel.String());
			BMenuItem* item = new BMenuItem(currentModel.String(), modelMsg);
			item->SetMarked(true);
			fModelMenu->AddItem(item);
			foundCurrent = true;
		}
		fStatusView->SetText("Click 'Fetch Models' to load available models");
	}

	// Select first item if no current selection
	if (!foundCurrent && fModelMenu->CountItems() > 0) {
		fModelMenu->ItemAt(0)->SetMarked(true);
	}
}
