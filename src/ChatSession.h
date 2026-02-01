#ifndef CHAT_SESSION_H
#define CHAT_SESSION_H

#include <Message.h>
#include <ObjectList.h>
#include <String.h>

#include "ChatMessage.h"

class ChatSession {
public:
						ChatSession();
						ChatSession(const char* id);
						ChatSession(const BMessage* archive);
						~ChatSession();

	status_t			Archive(BMessage* archive) const;
	status_t			Load(const char* path);
	status_t			Save(const char* path) const;

	const char*			Id() const { return fId.String(); }
	const char*			Title() const { return fTitle.String(); }
	time_t				CreatedAt() const { return fCreatedAt; }
	time_t				UpdatedAt() const { return fUpdatedAt; }

	void				SetTitle(const char* title);
	void				UpdateTimestamp();

	// Message management
	const BObjectList<ChatMessage>& Messages() const { return fMessages; }
	void				AddMessage(ChatMessage* message);
	void				ClearMessages();
	int32				CountMessages() const { return fMessages.CountItems(); }

	// Generate title from first user message
	void				GenerateTitle();

private:
	void				_GenerateId();

	BString				fId;
	BString				fTitle;
	time_t				fCreatedAt;
	time_t				fUpdatedAt;
	BObjectList<ChatMessage> fMessages;
};

#endif // CHAT_SESSION_H
