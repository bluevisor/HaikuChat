#ifndef CHAT_MESSAGE_H
#define CHAT_MESSAGE_H

#include <Message.h>
#include <String.h>
#include <ctime>

enum MessageRole {
	kRoleUser = 0,
	kRoleAssistant = 1,
	kRoleSystem = 2
};

class ChatMessage {
public:
						ChatMessage();
						ChatMessage(MessageRole role, const char* content);
						ChatMessage(const BMessage* archive);
						~ChatMessage();

	status_t			Archive(BMessage* archive) const;
	static ChatMessage*	Instantiate(const BMessage* archive);

	MessageRole			Role() const { return fRole; }
	const char*			Content() const { return fContent.String(); }
	time_t				Timestamp() const { return fTimestamp; }

	void				SetContent(const char* content);
	void				AppendContent(const char* text);

private:
	MessageRole			fRole;
	BString				fContent;
	time_t				fTimestamp;
};

#endif // CHAT_MESSAGE_H
