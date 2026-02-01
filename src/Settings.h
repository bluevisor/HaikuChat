#ifndef SETTINGS_H
#define SETTINGS_H

#include <File.h>
#include <Message.h>
#include <ObjectList.h>
#include <Path.h>
#include <Rect.h>
#include <String.h>

#include "ChatSession.h"
#include "Constants.h"

class Settings {
public:
						Settings();
						~Settings();

	status_t			Load();
	status_t			Save();

	// API settings - per provider
	ApiType				GetApiType() const { return fApiType; }
	void				SetApiType(ApiType type) { fApiType = type; }

	// These return/set values for the CURRENT API type
	const char*			GetApiEndpoint() const;
	const char*			GetApiKey() const;
	const char*			GetModel() const;

	void				SetApiEndpoint(const char* endpoint);
	void				SetApiKey(const char* key);
	void				SetModel(const char* model);

	// Get/set for specific API type
	const char*			GetApiKeyFor(ApiType type) const;
	const char*			GetApiEndpointFor(ApiType type) const;
	const char*			GetModelFor(ApiType type) const;

	void				SetApiKeyFor(ApiType type, const char* key);
	void				SetApiEndpointFor(ApiType type, const char* endpoint);
	void				SetModelFor(ApiType type, const char* model);

	// Cached models per API type
	const BObjectList<BString>& GetCachedModels(ApiType type) const;
	void				SetCachedModels(ApiType type, const BObjectList<BString>& models);
	bool				HasCachedModels(ApiType type) const;

	// Theme settings
	bool				IsDarkTheme() const { return fDarkTheme; }
	void				SetDarkTheme(bool dark) { fDarkTheme = dark; }

	// Window settings
	BRect				GetWindowFrame() const { return fWindowFrame; }
	void				SetWindowFrame(BRect frame) { fWindowFrame = frame; }

	// Sidebar settings
	bool				IsSidebarCollapsed() const { return fSidebarCollapsed; }
	void				SetSidebarCollapsed(bool collapsed) { fSidebarCollapsed = collapsed; }

	// Chat sessions
	BObjectList<ChatSession>& GetSessions() { return fSessions; }
	ChatSession*		GetCurrentSession() const { return fCurrentSession; }
	void				SetCurrentSession(ChatSession* session) { fCurrentSession = session; }

	ChatSession*		CreateNewSession();
	void				DeleteSession(ChatSession* session);
	status_t			LoadSessions();
	status_t			SaveSessions();
	status_t			SaveSession(ChatSession* session);

private:
	status_t			_EnsureDirectories();
	BString				_GetSessionPath(ChatSession* session);

	ApiType				fApiType;
	bool				fDarkTheme;
	BRect				fWindowFrame;
	bool				fSidebarCollapsed;

	// Per-provider settings (indexed by ApiType)
	BString				fApiEndpoints[3];
	BString				fApiKeys[3];
	BString				fModels[3];

	BObjectList<ChatSession> fSessions;
	ChatSession*		fCurrentSession;

	// Cached models per API type (3 types: OpenAI, Claude, Gemini)
	BObjectList<BString> fCachedModels[3];
};

#endif // SETTINGS_H
