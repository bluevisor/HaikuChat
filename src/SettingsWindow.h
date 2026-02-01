#ifndef SETTINGS_WINDOW_H
#define SETTINGS_WINDOW_H

#include <Button.h>
#include <CheckBox.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <StringView.h>
#include <TextControl.h>
#include <Window.h>

#include "LLMClient.h"
#include "Settings.h"

class SettingsWindow : public BWindow {
public:
						SettingsWindow(BRect frame, Settings* settings);
	virtual				~SettingsWindow();

	virtual void		MessageReceived(BMessage* message);

private:
	void				_SaveSettings();
	void				_UpdateFieldsForApiType(ApiType type);
	void				_FetchModels();
	void				_PopulateModels(BMessage* message);
	void				_LoadCachedModels();

	Settings*			fSettings;
	LLMClient*			fLLMClient;
	ApiType				fCurrentApiType;

	BPopUpMenu*			fApiTypeMenu;
	BMenuField*			fApiTypeField;
	BTextControl*		fEndpointField;
	BTextControl*		fApiKeyField;
	BPopUpMenu*			fModelMenu;
	BMenuField*			fModelField;
	BButton*			fFetchModelsButton;
	BCheckBox*			fDarkThemeCheckbox;
	BStringView*		fStatusView;
	BButton*			fSaveButton;
	BButton*			fCancelButton;
};

#endif // SETTINGS_WINDOW_H
