#include "ChatSession.h"

#include <Directory.h>
#include <File.h>
#include <cstdio>
#include <cstdlib>

#include "Log.h"

ChatSession::ChatSession()
	:
	fTitle("New Chat"),
	fCreatedAt(time(NULL)),
	fUpdatedAt(time(NULL)),
	fMessages(20, true)
{
	_GenerateId();
}


ChatSession::ChatSession(const char* id)
	:
	fId(id),
	fTitle("New Chat"),
	fCreatedAt(time(NULL)),
	fUpdatedAt(time(NULL)),
	fMessages(20, true)
{
}


ChatSession::ChatSession(const BMessage* archive)
	:
	fTitle("New Chat"),
	fCreatedAt(time(NULL)),
	fUpdatedAt(time(NULL)),
	fMessages(20, true)
{
	if (archive == NULL)
		return;

	const char* str;
	if (archive->FindString("id", &str) == B_OK)
		fId = str;
	else
		_GenerateId();

	if (archive->FindString("title", &str) == B_OK)
		fTitle = str;

	int64 timestamp;
	if (archive->FindInt64("created_at", &timestamp) == B_OK)
		fCreatedAt = static_cast<time_t>(timestamp);
	if (archive->FindInt64("updated_at", &timestamp) == B_OK)
		fUpdatedAt = static_cast<time_t>(timestamp);

	// Load messages
	BMessage msgArchive;
	for (int32 i = 0; archive->FindMessage("message", i, &msgArchive) == B_OK; i++) {
		ChatMessage* msg = ChatMessage::Instantiate(&msgArchive);
		if (msg != NULL)
			fMessages.AddItem(msg);
	}
}


ChatSession::~ChatSession()
{
}


status_t
ChatSession::Archive(BMessage* archive) const
{
	if (archive == NULL)
		return B_BAD_VALUE;

	status_t status = archive->AddString("id", fId.String());
	if (status == B_OK)
		status = archive->AddString("title", fTitle.String());
	if (status == B_OK)
		status = archive->AddInt64("created_at", static_cast<int64>(fCreatedAt));
	if (status == B_OK)
		status = archive->AddInt64("updated_at", static_cast<int64>(fUpdatedAt));

	// Save messages
	for (int32 i = 0; i < fMessages.CountItems() && status == B_OK; i++) {
		BMessage msgArchive;
		status = fMessages.ItemAt(i)->Archive(&msgArchive);
		if (status == B_OK)
			status = archive->AddMessage("message", &msgArchive);
	}

	return status;
}


status_t
ChatSession::Load(const char* path)
{
	BFile file(path, B_READ_ONLY);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;

	BMessage archive;
	status = archive.Unflatten(&file);
	if (status != B_OK)
		return status;

	const char* str;
	if (archive.FindString("id", &str) == B_OK)
		fId = str;
	if (archive.FindString("title", &str) == B_OK)
		fTitle = str;

	int64 timestamp;
	if (archive.FindInt64("created_at", &timestamp) == B_OK)
		fCreatedAt = static_cast<time_t>(timestamp);
	if (archive.FindInt64("updated_at", &timestamp) == B_OK)
		fUpdatedAt = static_cast<time_t>(timestamp);

	// Load messages
	fMessages.MakeEmpty();
	BMessage msgArchive;
	for (int32 i = 0; archive.FindMessage("message", i, &msgArchive) == B_OK; i++) {
		ChatMessage* msg = ChatMessage::Instantiate(&msgArchive);
		if (msg != NULL)
			fMessages.AddItem(msg);
	}

	return B_OK;
}


status_t
ChatSession::Save(const char* path) const
{
	LOG("ChatSession::Save - Saving '%s' to %s", fTitle.String(), path);

	BFile file(path, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t status = file.InitCheck();
	if (status != B_OK) {
		LOG_ERROR("  Failed to open file for writing: %s", strerror(status));
		return status;
	}

	BMessage archive;
	status = Archive(&archive);
	if (status != B_OK) {
		LOG_ERROR("  Failed to archive session: %s", strerror(status));
		return status;
	}

	status = archive.Flatten(&file);
	if (status == B_OK) {
		LOG("  Session saved successfully (%d messages)", fMessages.CountItems());
	} else {
		LOG_ERROR("  Failed to flatten archive: %s", strerror(status));
	}

	return status;
}


void
ChatSession::SetTitle(const char* title)
{
	fTitle = title;
	UpdateTimestamp();
}


void
ChatSession::UpdateTimestamp()
{
	fUpdatedAt = time(NULL);
}


void
ChatSession::AddMessage(ChatMessage* message)
{
	fMessages.AddItem(message);
	UpdateTimestamp();

	// Auto-generate title from first user message
	if (fTitle == "New Chat" && message->Role() == kRoleUser) {
		GenerateTitle();
	}
}


void
ChatSession::ClearMessages()
{
	fMessages.MakeEmpty();
	fTitle = "New Chat";
	UpdateTimestamp();
}


void
ChatSession::GenerateTitle()
{
	// Find first user message and use it as title
	for (int32 i = 0; i < fMessages.CountItems(); i++) {
		ChatMessage* msg = fMessages.ItemAt(i);
		if (msg->Role() == kRoleUser) {
			BString title = msg->Content();
			// Truncate to first line or 50 chars
			int32 newline = title.FindFirst('\n');
			if (newline > 0)
				title.Truncate(newline);
			if (title.Length() > 50) {
				title.Truncate(47);
				title.Append("...");
			}
			fTitle = title;
			return;
		}
	}
}


void
ChatSession::_GenerateId()
{
	// Generate unique ID based on timestamp and random number
	char id[32];
	snprintf(id, sizeof(id), "chat_%ld_%d",
		static_cast<long>(time(NULL)), rand() % 10000);
	fId = id;
}
