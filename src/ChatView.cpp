#include "ChatView.h"

#include <ScrollBar.h>
#include <cmath>

#include "Constants.h"
#include "Log.h"

ChatView::ChatView()
	:
	BView("ChatView", B_WILL_DRAW | B_FRAME_EVENTS),
	fBubbles(20, true),
	fContentHeight(0),
	fViewWidth(0),
	fLayoutInProgress(false)
{
	SetViewColor(B_TRANSPARENT_COLOR);  // We'll draw our own background
}


ChatView::~ChatView()
{
}


void
ChatView::AttachedToWindow()
{
	BView::AttachedToWindow();

	// Get initial width from parent
	if (Parent() != NULL) {
		fViewWidth = Parent()->Bounds().Width() - B_V_SCROLL_BAR_WIDTH;
	}

	_LayoutMessages();
}


void
ChatView::FrameResized(float newWidth, float newHeight)
{
	BView::FrameResized(newWidth, newHeight);

	// Only relayout if width actually changed significantly
	if (fabs(newWidth - fViewWidth) > 2.0f) {
		fViewWidth = newWidth;
		_LayoutMessages();
	}
}


void
ChatView::Draw(BRect updateRect)
{
	// Fill entire view with background color
	SetHighColor(kBackgroundColor);
	FillRect(Bounds());
}


void
ChatView::AddMessage(ChatMessage* message)
{
	LOG("ChatView::AddMessage - Role: %d, Content length: %d",
		message->Role(), (int)strlen(message->Content()));

	MessageBubble* bubble = new MessageBubble(message);
	fBubbles.AddItem(bubble);
	AddChild(bubble);

	_LayoutMessages();
	ScrollToBottom();
}


void
ChatView::UpdateLastMessage()
{
	MessageBubble* last = LastBubble();
	if (last != NULL) {
		last->UpdateContent();
		_LayoutMessages();
		ScrollToBottom();
	}
}


void
ChatView::ClearMessages()
{
	for (int32 i = fBubbles.CountItems() - 1; i >= 0; i--) {
		MessageBubble* bubble = fBubbles.ItemAt(i);
		RemoveChild(bubble);
	}
	fBubbles.MakeEmpty();
	fContentHeight = 0;

	BScrollBar* scrollBar = ScrollBar(B_VERTICAL);
	if (scrollBar != NULL)
		scrollBar->SetRange(0, 0);

	Invalidate();
}


void
ChatView::ScrollToBottom()
{
	BScrollBar* scrollBar = ScrollBar(B_VERTICAL);
	if (scrollBar != NULL) {
		float min, max;
		scrollBar->GetRange(&min, &max);
		scrollBar->SetValue(max);
	}
}


MessageBubble*
ChatView::LastBubble() const
{
	if (fBubbles.CountItems() == 0)
		return NULL;
	return fBubbles.ItemAt(fBubbles.CountItems() - 1);
}


void
ChatView::_LayoutMessages()
{
	// Prevent recursive layout calls
	if (fLayoutInProgress)
		return;
	fLayoutInProgress = true;

	// Get viewport dimensions from parent scroll view
	float viewportHeight = 400;
	float viewportWidth = 600;

	if (Parent() != NULL) {
		viewportHeight = Parent()->Bounds().Height();
		viewportWidth = Parent()->Bounds().Width() - B_V_SCROLL_BAR_WIDTH;
	}

	// Always use the current viewport width
	if (viewportWidth > 100) {
		fViewWidth = viewportWidth;
	} else if (fViewWidth < 100) {
		// Fallback to reasonable default
		fViewWidth = 500;
	}

	float maxBubbleWidth = fViewWidth * kBubbleMaxWidthRatio;
	float y = kBubbleMargin;

	for (int32 i = 0; i < fBubbles.CountItems(); i++) {
		MessageBubble* bubble = fBubbles.ItemAt(i);
		bubble->SetMaxWidth(maxBubbleWidth);

		float prefWidth, prefHeight;
		bubble->GetPreferredSize(&prefWidth, &prefHeight);

		// Bubble height is text height plus padding
		float bubbleHeight = prefHeight + 2 * kBubblePadding;

		// Ensure minimum height
		if (bubbleHeight < 40)
			bubbleHeight = 40;

		bubble->MoveTo(kBubbleMargin, y);
		bubble->ResizeTo(fViewWidth - 2 * kBubbleMargin, bubbleHeight);

		y += bubbleHeight + kBubbleMargin;
	}

	fContentHeight = y;

	// Calculate new height - only resize if needed
	float currentHeight = Bounds().Height();
	float newHeight = max_c(viewportHeight, fContentHeight);

	if (fabs(newHeight - currentHeight) > 1.0f) {
		ResizeTo(fViewWidth, newHeight);
	}

	// Update scroll bar
	BScrollBar* scrollBar = ScrollBar(B_VERTICAL);
	if (scrollBar != NULL) {
		if (fContentHeight > viewportHeight) {
			scrollBar->SetRange(0, fContentHeight - viewportHeight);
			scrollBar->SetProportion(viewportHeight / fContentHeight);
			scrollBar->SetSteps(20, viewportHeight - 20);
		} else {
			scrollBar->SetRange(0, 0);
			scrollBar->SetProportion(1.0);
		}
	}

	Invalidate();
	fLayoutInProgress = false;
}


// ChatScrollView implementation

ChatScrollView::ChatScrollView(ChatView* target)
	:
	BScrollView("ChatScrollView", target, B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW | B_FRAME_EVENTS, false, true, B_NO_BORDER),
	fChatView(target)
{
	SetViewColor(kBackgroundColor);
}


void
ChatScrollView::FrameResized(float newWidth, float newHeight)
{
	BScrollView::FrameResized(newWidth, newHeight);

	// Resize the chat view to match our width - only if significantly different
	if (fChatView != NULL) {
		float chatWidth = newWidth - B_V_SCROLL_BAR_WIDTH;
		float currentWidth = fChatView->Bounds().Width();
		if (fabs(chatWidth - currentWidth) > 2.0f) {
			fChatView->ResizeTo(chatWidth, fChatView->Bounds().Height());
		}
	}
}


ChatScrollView::~ChatScrollView()
{
}
