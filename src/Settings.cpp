#include "Settings.h"

#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>

#include "Log.h"

Settings::Settings()
	:
	fApiType(kApiTypeOpenAI),
	fDarkTheme(true),
	fWindowFrame(100, 100, 900, 700),
	fSidebarCollapsed(false),
	fSessions(20, true),
	fCurrentSession(NULL)
{
	// Initialize default endpoints
	fApiEndpoints[kApiTypeOpenAI] = "https://api.openai.com/v1";
	fApiEndpoints[kApiTypeClaude] = "https://api.anthropic.com/v1";
	fApiEndpoints[kApiTypeGemini] = "https://generativelanguage.googleapis.com/v1beta";

	// Initialize default models
	fModels[kApiTypeOpenAI] = "gpt-4";
	fModels[kApiTypeClaude] = "claude-sonnet-4-20250514";
	fModels[kApiTypeGemini] = "gemini-2.0-flash";

	// Initialize cached model lists
	for (int i = 0; i < 3; i++)
		fCachedModels[i].MakeEmpty();
}


Settings::~Settings()
{
}


// Current API type accessors
const char*
Settings::GetApiEndpoint() const
{
	return fApiEndpoints[fApiType].String();
}


const char*
Settings::GetApiKey() const
{
	return fApiKeys[fApiType].String();
}


const char*
Settings::GetModel() const
{
	return fModels[fApiType].String();
}


void
Settings::SetApiEndpoint(const char* endpoint)
{
	fApiEndpoints[fApiType] = endpoint;
}


void
Settings::SetApiKey(const char* key)
{
	fApiKeys[fApiType] = key;
}


void
Settings::SetModel(const char* model)
{
	fModels[fApiType] = model;
}


// Specific API type accessors
const char*
Settings::GetApiKeyFor(ApiType type) const
{
	if (type >= 0 && type < 3)
		return fApiKeys[type].String();
	return "";
}


const char*
Settings::GetApiEndpointFor(ApiType type) const
{
	if (type >= 0 && type < 3)
		return fApiEndpoints[type].String();
	return "";
}


const char*
Settings::GetModelFor(ApiType type) const
{
	if (type >= 0 && type < 3)
		return fModels[type].String();
	return "";
}


void
Settings::SetApiKeyFor(ApiType type, const char* key)
{
	if (type >= 0 && type < 3)
		fApiKeys[type] = key;
}


void
Settings::SetApiEndpointFor(ApiType type, const char* endpoint)
{
	if (type >= 0 && type < 3)
		fApiEndpoints[type] = endpoint;
}


void
Settings::SetModelFor(ApiType type, const char* model)
{
	if (type >= 0 && type < 3)
		fModels[type] = model;
}


status_t
Settings::Load()
{
	LOG("Settings::Load - Loading settings");
	_EnsureDirectories();

	BString path(SETTINGS_DIR);
	path.Append("/");
	path.Append(SETTINGS_FILE);

	LOG("Settings file path: %s", path.String());

	BFile file(path.String(), B_READ_ONLY);
	status_t status = file.InitCheck();
	if (status != B_OK) {
		LOG("Settings file not found or could not open: %s", strerror(status));
		return status;
	}

	BMessage archive;
	status = archive.Unflatten(&file);
	if (status != B_OK)
		return status;

	int32 apiType;
	if (archive.FindInt32("api_type", &apiType) == B_OK)
		fApiType = static_cast<ApiType>(apiType);

	bool darkTheme;
	if (archive.FindBool("dark_theme", &darkTheme) == B_OK)
		fDarkTheme = darkTheme;

	// Load per-provider settings
	const char* str;
	for (int32 type = 0; type < 3; type++) {
		BString keyName, endpointName, modelName;
		keyName.SetToFormat("api_key_%d", type);
		endpointName.SetToFormat("api_endpoint_%d", type);
		modelName.SetToFormat("model_%d", type);

		if (archive.FindString(keyName.String(), &str) == B_OK)
			fApiKeys[type] = str;
		if (archive.FindString(endpointName.String(), &str) == B_OK)
			fApiEndpoints[type] = str;
		if (archive.FindString(modelName.String(), &str) == B_OK)
			fModels[type] = str;
	}

	// Legacy support - migrate old single-key settings
	if (archive.FindString("api_key", &str) == B_OK && fApiKeys[fApiType].Length() == 0)
		fApiKeys[fApiType] = str;
	if (archive.FindString("api_endpoint", &str) == B_OK)
		fApiEndpoints[fApiType] = str;
	if (archive.FindString("model", &str) == B_OK)
		fModels[fApiType] = str;

	if (archive.FindRect("window_frame", &fWindowFrame) != B_OK)
		fWindowFrame = BRect(100, 100, 900, 700);

	bool collapsed;
	if (archive.FindBool("sidebar_collapsed", &collapsed) == B_OK)
		fSidebarCollapsed = collapsed;

	// Load cached models for each API type
	for (int32 type = 0; type < 3; type++) {
		fCachedModels[type].MakeEmpty();
		const char* model;
		BString fieldName;
		fieldName.SetToFormat("cached_models_%d", type);
		for (int32 i = 0; archive.FindString(fieldName.String(), i, &model) == B_OK; i++) {
			fCachedModels[type].AddItem(new BString(model));
		}
	}

	// Load sessions
	LoadSessions();

	// Set current session
	const char* currentId;
	if (archive.FindString("current_session", &currentId) == B_OK) {
		for (int32 i = 0; i < fSessions.CountItems(); i++) {
			if (BString(fSessions.ItemAt(i)->Id()) == currentId) {
				fCurrentSession = fSessions.ItemAt(i);
				break;
			}
		}
	}

	return B_OK;
}


status_t
Settings::Save()
{
	_EnsureDirectories();

	BString path(SETTINGS_DIR);
	path.Append("/");
	path.Append(SETTINGS_FILE);

	BFile file(path.String(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;

	BMessage archive;
	archive.AddInt32("api_type", static_cast<int32>(fApiType));
	archive.AddBool("dark_theme", fDarkTheme);
	archive.AddRect("window_frame", fWindowFrame);
	archive.AddBool("sidebar_collapsed", fSidebarCollapsed);

	// Save per-provider settings
	for (int32 type = 0; type < 3; type++) {
		BString keyName, endpointName, modelName;
		keyName.SetToFormat("api_key_%d", type);
		endpointName.SetToFormat("api_endpoint_%d", type);
		modelName.SetToFormat("model_%d", type);

		archive.AddString(keyName.String(), fApiKeys[type].String());
		archive.AddString(endpointName.String(), fApiEndpoints[type].String());
		archive.AddString(modelName.String(), fModels[type].String());
	}

	if (fCurrentSession != NULL)
		archive.AddString("current_session", fCurrentSession->Id());

	// Save cached models for each API type
	for (int32 type = 0; type < 3; type++) {
		BString fieldName;
		fieldName.SetToFormat("cached_models_%d", type);
		for (int32 i = 0; i < fCachedModels[type].CountItems(); i++) {
			archive.AddString(fieldName.String(), fCachedModels[type].ItemAt(i)->String());
		}
	}

	status = archive.Flatten(&file);

	// Save all sessions
	SaveSessions();

	return status;
}


ChatSession*
Settings::CreateNewSession()
{
	ChatSession* session = new ChatSession();
	fSessions.AddItem(session, 0);  // Add at front
	fCurrentSession = session;
	LOG("Created new session: ID=%s, Title='%s'", session->Id(), session->Title());
	SaveSession(session);
	return session;
}


void
Settings::DeleteSession(ChatSession* session)
{
	if (session == NULL)
		return;

	// Delete file
	BString path = _GetSessionPath(session);
	BEntry entry(path.String());
	if (entry.Exists())
		entry.Remove();

	// Remove from list
	fSessions.RemoveItem(session);

	// Update current session
	if (fCurrentSession == session) {
		if (fSessions.CountItems() > 0)
			fCurrentSession = fSessions.ItemAt(0);
		else
			fCurrentSession = NULL;
	}

	delete session;
}


status_t
Settings::LoadSessions()
{
	LOG("Settings::LoadSessions - Loading chat sessions from %s", CHATS_DIR);
	fSessions.MakeEmpty();

	BDirectory dir(CHATS_DIR);
	if (dir.InitCheck() != B_OK) {
		LOG("Chats directory does not exist or cannot be opened (first run?)");
		return B_OK;  // No chats yet
	}

	BEntry entry;
	int32 loadedCount = 0;
	int32 failedCount = 0;

	while (dir.GetNextEntry(&entry) == B_OK) {
		BPath path;
		entry.GetPath(&path);

		LOG("Loading session from: %s", path.Path());

		ChatSession* session = new ChatSession("");
		status_t status = session->Load(path.Path());
		if (status == B_OK) {
			fSessions.AddItem(session);
			loadedCount++;
			LOG("  Session loaded: '%s' (ID: %s, Messages: %d)",
				session->Title(), session->Id(), session->Messages().CountItems());
		} else {
			LOG_ERROR("  Failed to load session: %s", strerror(status));
			delete session;
			failedCount++;
		}
	}

	LOG("Sessions loaded: %d successful, %d failed", loadedCount, failedCount);

	// Sort by updated time (newest first)
	fSessions.SortItems([](const ChatSession* a, const ChatSession* b) -> int {
		time_t diff = b->UpdatedAt() - a->UpdatedAt();
		if (diff > 0) return 1;
		if (diff < 0) return -1;
		return 0;
	});

	return B_OK;
}


status_t
Settings::SaveSessions()
{
	_EnsureDirectories();

	for (int32 i = 0; i < fSessions.CountItems(); i++) {
		SaveSession(fSessions.ItemAt(i));
	}

	return B_OK;
}


status_t
Settings::SaveSession(ChatSession* session)
{
	if (session == NULL)
		return B_BAD_VALUE;

	_EnsureDirectories();

	BString path = _GetSessionPath(session);
	return session->Save(path.String());
}


status_t
Settings::_EnsureDirectories()
{
	// Check if SETTINGS_DIR exists as a file (not directory) and remove it
	BEntry settingsEntry(SETTINGS_DIR);
	if (settingsEntry.Exists() && !settingsEntry.IsDirectory()) {
		LOG("Settings directory path exists as a file, removing it");
		settingsEntry.Remove();
	}

	status_t status = create_directory(SETTINGS_DIR, 0755);
	if (status != B_OK && status != B_FILE_EXISTS) {
		LOG_ERROR("Failed to create settings directory: %s", strerror(status));
		return status;
	}

	status = create_directory(CHATS_DIR, 0755);
	if (status != B_OK && status != B_FILE_EXISTS) {
		LOG_ERROR("Failed to create chats directory: %s", strerror(status));
		return status;
	}

	return B_OK;
}


BString
Settings::_GetSessionPath(ChatSession* session)
{
	BString path(CHATS_DIR);
	path.Append("/");
	path.Append(session->Id());
	path.Append(".chat");
	return path;
}


const BObjectList<BString>&
Settings::GetCachedModels(ApiType type) const
{
	if (type >= 0 && type < 3)
		return fCachedModels[type];
	return fCachedModels[0];  // Fallback to OpenAI list
}


void
Settings::SetCachedModels(ApiType type, const BObjectList<BString>& models)
{
	if (type < 0 || type >= 3)
		return;

	fCachedModels[type].MakeEmpty();
	for (int32 i = 0; i < models.CountItems(); i++) {
		fCachedModels[type].AddItem(new BString(*models.ItemAt(i)));
	}
}


bool
Settings::HasCachedModels(ApiType type) const
{
	if (type >= 0 && type < 3)
		return fCachedModels[type].CountItems() > 0;
	return false;
}
