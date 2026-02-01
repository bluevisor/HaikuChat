#include "InputView.h"

#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <Window.h>
#include <cmath>

#include "Constants.h"

// InputTextView implementation

InputTextView::InputTextView(BRect frame, BMessenger target)
	:
	BTextView(frame, "InputText", frame.InsetByCopy(4, 4),
		B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS),
	fTarget(target)
{
	SetWordWrap(true);
}


InputTextView::~InputTextView()
{
}


void
InputTextView::KeyDown(const char* bytes, int32 numBytes)
{
	if (numBytes == 1) {
		// Check for Cmd+Enter or Ctrl+Enter to send
		uint32 modifiers = Window()->CurrentMessage()->FindInt32("modifiers");
		if (bytes[0] == B_ENTER) {
			if (modifiers & (B_COMMAND_KEY | B_CONTROL_KEY)) {
				// Send message
				BMessage msg(kMsgSendMessage);
				fTarget.SendMessage(&msg);
				return;
			}
		}
	}

	BTextView::KeyDown(bytes, numBytes);

	// Notify about text change for height adjustment
	BMessage msg(kMsgInputChanged);
	fTarget.SendMessage(&msg);
}


void
InputTextView::FrameResized(float newWidth, float newHeight)
{
	BTextView::FrameResized(newWidth, newHeight);

	// Update text rect when resized
	BRect textRect = Bounds();
	textRect.InsetBy(4, 4);
	textRect.OffsetTo(0, 0);
	SetTextRect(textRect);
}


// InputView implementation

InputView::InputView()
	:
	BView("InputView", B_WILL_DRAW | B_FRAME_EVENTS),
	fTextView(NULL),
	fScrollView(NULL),
	fSendButton(NULL),
	fCurrentHeight(kInputMinHeight)
{
	SetViewColor(kBackgroundColor);
}


InputView::~InputView()
{
}


void
InputView::AttachedToWindow()
{
	BView::AttachedToWindow();

	// Create text input with larger font
	BRect textFrame(0, 0, 200, kInputMinHeight - 10);
	fTextView = new InputTextView(textFrame, BMessenger(this));
	fTextView->SetViewColor(kInputBackgroundColor);
	fTextView->SetLowColor(kInputBackgroundColor);

	// Use slightly larger font for input
	BFont font(be_plain_font);
	font.SetSize(font.Size() + 1);
	fTextView->SetFontAndColor(&font, B_FONT_ALL, &kUserTextColor);

	fScrollView = new BScrollView("InputScroll", fTextView,
		B_FOLLOW_ALL_SIDES, B_WILL_DRAW, false, false, B_NO_BORDER);
	fScrollView->SetViewColor(kInputBackgroundColor);
	fScrollView->SetLowColor(kInputBackgroundColor);

	// Create send button with icon-like label
	fSendButton = new BButton("->", new BMessage(kMsgSendMessage));
	fSendButton->SetTarget(Window());
	fSendButton->MakeDefault(true);
	fSendButton->SetExplicitSize(BSize(40, 36));

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 8)
		.SetInsets(12, 8, 12, 12)
		.Add(fScrollView, 1.0f)
		.Add(fSendButton)
		.End();
}


void
InputView::Draw(BRect updateRect)
{
	BView::Draw(updateRect);

	// Draw rounded border around the input area
	BRect bounds = Bounds();
	bounds.InsetBy(8, 4);

	SetHighColor(kBorderColor);
	SetPenSize(1.0f);
	StrokeRoundRect(bounds, 8.0f, 8.0f);
}


void
InputView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgInputChanged:
			_UpdateHeight();
			break;

		case kMsgSendMessage:
			Window()->PostMessage(message);
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
InputView::GetPreferredSize(float* width, float* height)
{
	*width = 200;
	*height = fCurrentHeight > kInputMinHeight ? fCurrentHeight : kInputMinHeight;
}


BSize
InputView::MinSize()
{
	return BSize(200, kInputMinHeight);
}


BSize
InputView::MaxSize()
{
	return BSize(B_SIZE_UNLIMITED, kInputMaxHeight);
}


BSize
InputView::PreferredSize()
{
	return BSize(B_SIZE_UNSET, fCurrentHeight > kInputMinHeight ? fCurrentHeight : kInputMinHeight);
}


const char*
InputView::Text() const
{
	if (fTextView != NULL)
		return fTextView->Text();
	return "";
}


void
InputView::SetText(const char* text)
{
	if (fTextView != NULL) {
		fTextView->SetText(text);
		// Reapply font and color after SetText (it resets formatting)
		BFont font(be_plain_font);
		font.SetSize(font.Size() + 1);
		if (fTextView->TextLength() > 0) {
			fTextView->SetFontAndColor(0, fTextView->TextLength(),
				&font, B_FONT_ALL, &kUserTextColor);
		}
		// Reset height to minimum when text is cleared
		if (text == NULL || text[0] == '\0') {
			fCurrentHeight = kInputMinHeight;
		}
	}
}


void
InputView::MakeFocus(bool focus)
{
	if (fTextView != NULL)
		fTextView->MakeFocus(focus);
}


void
InputView::SetEnabled(bool enabled)
{
	if (fTextView != NULL)
		fTextView->MakeEditable(enabled);
	if (fSendButton != NULL)
		fSendButton->SetEnabled(enabled);
}


void
InputView::_UpdateHeight()
{
	if (fTextView == NULL)
		return;

	float textHeight = fTextView->TextHeight(0, fTextView->CountLines());
	float newHeight = textHeight + 24;  // Add padding

	if (newHeight < kInputMinHeight)
		newHeight = kInputMinHeight;
	if (newHeight > kInputMaxHeight)
		newHeight = kInputMaxHeight;

	if (fabs(newHeight - fCurrentHeight) > 2.0f) {
		fCurrentHeight = newHeight;
		SetExplicitMinSize(BSize(B_SIZE_UNSET, newHeight));
		SetExplicitPreferredSize(BSize(B_SIZE_UNSET, newHeight));
		if (Window() != NULL)
			Window()->PostMessage(kMsgInputChanged);
	}
}


void
InputView::RefreshColors()
{
	SetViewColor(kBackgroundColor);

	if (fTextView != NULL) {
		fTextView->SetViewColor(kInputBackgroundColor);
		fTextView->SetLowColor(kInputBackgroundColor);

		BFont font(be_plain_font);
		font.SetSize(font.Size() + 1);
		fTextView->SetFontAndColor(0, fTextView->TextLength(),
			&font, B_FONT_ALL, &kUserTextColor);
	}

	if (fScrollView != NULL) {
		fScrollView->SetViewColor(kInputBackgroundColor);
		fScrollView->SetLowColor(kInputBackgroundColor);
	}

	Invalidate();
}
