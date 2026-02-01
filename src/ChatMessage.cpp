#include "ChatMessage.h"

ChatMessage::ChatMessage()
	:
	fRole(kRoleUser),
	fContent(""),
	fTimestamp(time(NULL))
{
}


ChatMessage::ChatMessage(MessageRole role, const char* content)
	:
	fRole(role),
	fContent(content),
	fTimestamp(time(NULL))
{
}


ChatMessage::ChatMessage(const BMessage* archive)
	:
	fRole(kRoleUser),
	fContent(""),
	fTimestamp(time(NULL))
{
	if (archive == NULL)
		return;

	int32 role;
	if (archive->FindInt32("role", &role) == B_OK)
		fRole = static_cast<MessageRole>(role);

	const char* content;
	if (archive->FindString("content", &content) == B_OK)
		fContent = content;

	int64 timestamp;
	if (archive->FindInt64("timestamp", &timestamp) == B_OK)
		fTimestamp = static_cast<time_t>(timestamp);
}


ChatMessage::~ChatMessage()
{
}


status_t
ChatMessage::Archive(BMessage* archive) const
{
	if (archive == NULL)
		return B_BAD_VALUE;

	status_t status = archive->AddInt32("role", static_cast<int32>(fRole));
	if (status == B_OK)
		status = archive->AddString("content", fContent.String());
	if (status == B_OK)
		status = archive->AddInt64("timestamp", static_cast<int64>(fTimestamp));

	return status;
}


ChatMessage*
ChatMessage::Instantiate(const BMessage* archive)
{
	return new ChatMessage(archive);
}


void
ChatMessage::SetContent(const char* content)
{
	fContent = content;
}


void
ChatMessage::AppendContent(const char* text)
{
	fContent.Append(text);
}
