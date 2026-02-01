#ifndef MESSAGE_BUBBLE_H
#define MESSAGE_BUBBLE_H

#include <Font.h>
#include <TextView.h>
#include <View.h>

#include "ChatMessage.h"

class MessageBubble : public BView {
public:
						MessageBubble(ChatMessage* message);
	virtual				~MessageBubble();

	virtual void		Draw(BRect updateRect);
	virtual void		GetPreferredSize(float* width, float* height);
	virtual void		FrameResized(float newWidth, float newHeight);

	void				SetMaxWidth(float maxWidth);
	void				UpdateContent();
	ChatMessage*		Message() const { return fMessage; }

private:
	void				_LayoutTextView();
	void				_ApplyMarkdown();
	void				_ApplyStyle(int32 start, int32 end, const BFont* font,
							const rgb_color* color);

	ChatMessage*		fMessage;
	BTextView*			fTextView;
	float				fMaxWidth;
	rgb_color			fBubbleColor;
	rgb_color			fTextColor;
	rgb_color			fCodeColor;
	rgb_color			fCodeBgColor;
	bool				fIsUser;

	BFont				fPlainFont;
	BFont				fBoldFont;
	BFont				fItalicFont;
	BFont				fCodeFont;
};

#endif // MESSAGE_BUBBLE_H
