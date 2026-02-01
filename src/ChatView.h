#ifndef CHAT_VIEW_H
#define CHAT_VIEW_H

#include <ObjectList.h>
#include <ScrollView.h>
#include <View.h>

#include "ChatMessage.h"
#include "MessageBubble.h"

class ChatView : public BView {
public:
						ChatView();
	virtual				~ChatView();

	virtual void		AttachedToWindow();
	virtual void		FrameResized(float newWidth, float newHeight);
	virtual void		Draw(BRect updateRect);

	void				AddMessage(ChatMessage* message);
	void				UpdateLastMessage();
	void				ClearMessages();
	void				ScrollToBottom();

	MessageBubble*		LastBubble() const;

private:
	void				_LayoutMessages();

	BObjectList<MessageBubble> fBubbles;
	float				fContentHeight;
	float				fViewWidth;
	bool				fLayoutInProgress;
};


class ChatScrollView : public BScrollView {
public:
						ChatScrollView(ChatView* target);
	virtual				~ChatScrollView();

	virtual void		FrameResized(float newWidth, float newHeight);

	ChatView*			GetChatView() const { return fChatView; }

private:
	ChatView*			fChatView;
};

#endif // CHAT_VIEW_H
