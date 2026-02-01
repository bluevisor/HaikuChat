#ifndef INPUT_VIEW_H
#define INPUT_VIEW_H

#include <Button.h>
#include <Messenger.h>
#include <Size.h>
#include <TextView.h>
#include <View.h>

class InputTextView : public BTextView {
public:
						InputTextView(BRect frame, BMessenger target);
	virtual				~InputTextView();

	virtual void		KeyDown(const char* bytes, int32 numBytes);
	virtual void		FrameResized(float newWidth, float newHeight);

private:
	BMessenger			fTarget;
};


class InputView : public BView {
public:
						InputView();
	virtual				~InputView();

	virtual void		AttachedToWindow();
	virtual void		Draw(BRect updateRect);
	virtual void		MessageReceived(BMessage* message);
	virtual void		GetPreferredSize(float* width, float* height);
	virtual BSize		MinSize();
	virtual BSize		MaxSize();
	virtual BSize		PreferredSize();

	const char*			Text() const;
	void				SetText(const char* text);
	void				MakeFocus(bool focus = true);
	void				SetEnabled(bool enabled);
	void				RefreshColors();

private:
	void				_UpdateHeight();

	InputTextView*		fTextView;
	BScrollView*		fScrollView;
	BButton*			fSendButton;
	float				fCurrentHeight;
};

#endif // INPUT_VIEW_H
