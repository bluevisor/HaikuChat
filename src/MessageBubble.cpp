#include "MessageBubble.h"

#include <LayoutUtils.h>
#include <String.h>

#include "Constants.h"

MessageBubble::MessageBubble(ChatMessage* message)
	:
	BView("MessageBubble", B_WILL_DRAW | B_FRAME_EVENTS),
	fMessage(message),
	fTextView(NULL),
	fMaxWidth(400.0f)
{
	fIsUser = (message->Role() == kRoleUser);
	fBubbleColor = fIsUser ? kUserBubbleColor : kAssistantBubbleColor;
	fTextColor = fIsUser ? kUserTextColor : kAssistantTextColor;

	// Code styling colors
	fCodeColor = fTextColor;
	if (IsDarkTheme()) {
		fCodeBgColor = (rgb_color){60, 60, 60, 255};
	} else {
		fCodeBgColor = (rgb_color){240, 240, 240, 255};
	}

	// Initialize fonts
	fPlainFont = *be_plain_font;
	fBoldFont = *be_bold_font;
	fItalicFont = *be_plain_font;
	fItalicFont.SetFace(B_ITALIC_FACE);
	fCodeFont = *be_fixed_font;
	fCodeFont.SetSize(fPlainFont.Size() - 1);

	SetViewColor(B_TRANSPARENT_COLOR);

	// Create text view for content
	BRect textRect(0, 0, fMaxWidth - 2 * kBubblePadding, 100);
	fTextView = new BTextView(textRect, "content", textRect,
		B_FOLLOW_NONE, B_WILL_DRAW);
	fTextView->SetViewColor(fBubbleColor);
	fTextView->MakeEditable(false);
	fTextView->MakeSelectable(true);
	fTextView->SetWordWrap(true);
	fTextView->SetStylable(true);

	// Set text and apply formatting
	fTextView->SetText(message->Content());
	_ApplyMarkdown();

	AddChild(fTextView);
}


MessageBubble::~MessageBubble()
{
}


void
MessageBubble::Draw(BRect updateRect)
{
	BRect bounds = Bounds();

	// First, clear entire bounds with chat background color
	SetHighColor(kBackgroundColor);
	FillRect(bounds);

	float textWidth, textHeight;
	GetPreferredSize(&textWidth, &textHeight);

	// Calculate bubble rect
	BRect bubbleRect;
	if (fIsUser) {
		bubbleRect = BRect(bounds.right - textWidth - 2 * kBubblePadding,
			0, bounds.right, textHeight + 2 * kBubblePadding);
	} else {
		bubbleRect = BRect(0, 0, textWidth + 2 * kBubblePadding,
			textHeight + 2 * kBubblePadding);
	}

	// Draw rounded rectangle background for bubble
	SetHighColor(fBubbleColor);
	FillRoundRect(bubbleRect, kBubbleRadius, kBubbleRadius);
}


void
MessageBubble::GetPreferredSize(float* width, float* height)
{
	if (fTextView == NULL) {
		*width = 100;
		*height = 30;
		return;
	}

	float textHeight = fTextView->TextHeight(0, fTextView->CountLines());
	float textWidth = fMaxWidth - 2 * kBubblePadding;

	// Find actual text width if it's shorter
	float lineWidth = 0;
	for (int32 i = 0; i < fTextView->CountLines(); i++) {
		float w = fTextView->LineWidth(i);
		if (w > lineWidth)
			lineWidth = w;
	}
	if (lineWidth < textWidth && lineWidth > 0)
		textWidth = lineWidth;

	*width = textWidth;
	*height = textHeight;
}


void
MessageBubble::FrameResized(float newWidth, float newHeight)
{
	BView::FrameResized(newWidth, newHeight);
	_LayoutTextView();
	Invalidate();
}


void
MessageBubble::SetMaxWidth(float maxWidth)
{
	if (maxWidth < 100)
		maxWidth = 400;  // Use default if width is invalid
	fMaxWidth = maxWidth;
	_LayoutTextView();
}


void
MessageBubble::UpdateContent()
{
	if (fTextView != NULL) {
		fTextView->SetText(fMessage->Content());
		_ApplyMarkdown();
		_LayoutTextView();
		Invalidate();
	}
}


void
MessageBubble::_LayoutTextView()
{
	if (fTextView == NULL)
		return;

	float textWidth = fMaxWidth - 2 * kBubblePadding;
	if (textWidth < 50)
		textWidth = 350;  // Fallback to reasonable width

	BRect textRect(0, 0, textWidth, 2000);
	fTextView->SetTextRect(textRect);

	float textHeight = fTextView->TextHeight(0, fTextView->CountLines());
	if (textHeight < 10)
		textHeight = 20;  // Minimum height for empty messages

	// Find actual width needed
	float lineWidth = 0;
	for (int32 i = 0; i < fTextView->CountLines(); i++) {
		float w = fTextView->LineWidth(i);
		if (w > lineWidth)
			lineWidth = w;
	}
	if (lineWidth < textWidth && lineWidth > 0)
		textWidth = lineWidth;

	BRect bounds = Bounds();

	// Use sensible defaults if bounds not yet valid
	float boundsWidth = bounds.Width();
	if (boundsWidth < 100)
		boundsWidth = fMaxWidth;

	BRect bubbleRect;
	if (fIsUser) {
		bubbleRect = BRect(boundsWidth - textWidth - 2 * kBubblePadding,
			0, boundsWidth, textHeight + 2 * kBubblePadding);
	} else {
		bubbleRect = BRect(0, 0, textWidth + 2 * kBubblePadding,
			textHeight + 2 * kBubblePadding);
	}

	fTextView->MoveTo(bubbleRect.left + kBubblePadding,
		bubbleRect.top + kBubblePadding);
	fTextView->ResizeTo(textWidth, textHeight);
	fTextView->SetViewColor(fBubbleColor);
}


void
MessageBubble::_ApplyStyle(int32 start, int32 end, const BFont* font,
	const rgb_color* color)
{
	if (start < 0 || end <= start || end > fTextView->TextLength())
		return;

	fTextView->SetFontAndColor(start, end, font, B_FONT_ALL, color);
}


void
MessageBubble::_ApplyMarkdown()
{
	if (fTextView == NULL)
		return;

	const char* text = fTextView->Text();
	int32 length = fTextView->TextLength();

	if (length == 0)
		return;

	// First apply default style to all text
	fTextView->SetFontAndColor(0, length, &fPlainFont, B_FONT_ALL, &fTextColor);

	BString content(text);

	// Dimmed color for markers
	rgb_color dimColor = fTextColor;
	dimColor.red = (uint8)(dimColor.red * 0.4);
	dimColor.green = (uint8)(dimColor.green * 0.4);
	dimColor.blue = (uint8)(dimColor.blue * 0.4);

	// Process code blocks first (```) - handle incomplete blocks during streaming
	int32 pos = 0;
	while (pos < length) {
		int32 codeBlockStart = content.FindFirst("```", pos);
		if (codeBlockStart < 0)
			break;

		int32 codeBlockEnd = content.FindFirst("```", codeBlockStart + 3);

		// Skip the language specifier line if present
		int32 contentStart = codeBlockStart + 3;
		int32 newlinePos = content.FindFirst('\n', contentStart);

		if (codeBlockEnd < 0) {
			// Incomplete code block (still streaming) - style to end of text
			if (newlinePos >= 0 && newlinePos < length) {
				contentStart = newlinePos + 1;
			}
			// Style the opening marker
			_ApplyStyle(codeBlockStart, contentStart, &fCodeFont, &dimColor);
			// Style the code content to end
			if (contentStart < length) {
				_ApplyStyle(contentStart, length, &fCodeFont, &fCodeColor);
			}
			break;  // No more code blocks possible
		}

		// Complete code block
		if (newlinePos >= 0 && newlinePos < codeBlockEnd) {
			contentStart = newlinePos + 1;
		}

		// Apply code font to the content between ```
		_ApplyStyle(contentStart, codeBlockEnd, &fCodeFont, &fCodeColor);

		// Apply dimmed style to the ``` markers and language line
		_ApplyStyle(codeBlockStart, contentStart, &fCodeFont, &dimColor);
		_ApplyStyle(codeBlockEnd, codeBlockEnd + 3, &fCodeFont, &dimColor);

		pos = codeBlockEnd + 3;
	}

	// Process inline code (`) - skip areas inside code blocks
	pos = 0;
	while (pos < length) {
		int32 codeStart = content.FindFirst('`', pos);
		if (codeStart < 0)
			break;

		// Skip if part of code block marker
		if ((codeStart > 0 && content[codeStart - 1] == '`') ||
			(codeStart + 1 < length && content[codeStart + 1] == '`')) {
			pos = codeStart + 1;
			continue;
		}

		int32 codeEnd = content.FindFirst('`', codeStart + 1);
		if (codeEnd < 0)
			break;

		// Skip if code block marker
		if (codeEnd + 1 < length && content[codeEnd + 1] == '`') {
			pos = codeEnd + 1;
			continue;
		}

		_ApplyStyle(codeStart, codeEnd + 1, &fCodeFont, &fCodeColor);
		pos = codeEnd + 1;
	}

	// Process bold (**text**)
	pos = 0;
	while (pos < length - 3) {
		int32 boldStart = content.FindFirst("**", pos);
		if (boldStart < 0)
			break;

		int32 boldEnd = content.FindFirst("**", boldStart + 2);
		if (boldEnd < 0)
			break;

		_ApplyStyle(boldStart + 2, boldEnd, &fBoldFont, &fTextColor);
		_ApplyStyle(boldStart, boldStart + 2, &fPlainFont, &dimColor);
		_ApplyStyle(boldEnd, boldEnd + 2, &fPlainFont, &dimColor);

		pos = boldEnd + 2;
	}

	// Process italic (*text* or _text_) - be careful not to match ** or __
	pos = 0;
	while (pos < length - 1) {
		int32 italicStart = -1;
		char marker = 0;

		// Find single * or _ that's not part of ** or __
		for (int32 i = pos; i < length; i++) {
			if (content[i] == '*' || content[i] == '_') {
				// Check it's not a double marker
				bool isPrevSame = (i > 0 && content[i - 1] == content[i]);
				bool isNextSame = (i + 1 < length && content[i + 1] == content[i]);
				if (!isPrevSame && !isNextSame) {
					italicStart = i;
					marker = content[i];
					break;
				}
			}
		}

		if (italicStart < 0)
			break;

		// Find closing marker
		int32 italicEnd = -1;
		for (int32 i = italicStart + 1; i < length; i++) {
			if (content[i] == marker) {
				bool isPrevSame = (content[i - 1] == marker);
				bool isNextSame = (i + 1 < length && content[i + 1] == marker);
				if (!isPrevSame && !isNextSame) {
					italicEnd = i;
					break;
				}
			}
		}

		if (italicEnd < 0)
			break;

		_ApplyStyle(italicStart + 1, italicEnd, &fItalicFont, &fTextColor);
		_ApplyStyle(italicStart, italicStart + 1, &fPlainFont, &dimColor);
		_ApplyStyle(italicEnd, italicEnd + 1, &fPlainFont, &dimColor);

		pos = italicEnd + 1;
	}

	// Process headers (# at start of line)
	pos = 0;
	while (pos < length) {
		// Find start of line
		int32 lineStart = pos;
		if (pos > 0) {
			lineStart = content.FindFirst('\n', pos);
			if (lineStart < 0)
				break;
			lineStart++;
		}

		if (lineStart >= length)
			break;

		// Count # at start of line
		int32 hashCount = 0;
		int32 i = lineStart;
		while (i < length && content[i] == '#' && hashCount < 6) {
			hashCount++;
			i++;
		}

		if (hashCount > 0 && i < length && content[i] == ' ') {
			// Find end of line
			int32 lineEnd = content.FindFirst('\n', i);
			if (lineEnd < 0)
				lineEnd = length;

			// Apply bold to header text
			BFont headerFont = fBoldFont;
			float sizeDelta = (6 - hashCount) * 1.5f;
			headerFont.SetSize(fPlainFont.Size() + sizeDelta);
			_ApplyStyle(i + 1, lineEnd, &headerFont, &fTextColor);
			_ApplyStyle(lineStart, i + 1, &fPlainFont, &dimColor);

			pos = lineEnd;
		} else {
			pos = lineStart + 1;
		}
	}

	// Process bullet points (- or * at start of line)
	pos = 0;
	while (pos < length) {
		int32 lineStart = pos;
		if (pos > 0) {
			lineStart = content.FindFirst('\n', pos);
			if (lineStart < 0)
				break;
			lineStart++;
		}

		if (lineStart >= length)
			break;

		// Check for bullet
		if ((content[lineStart] == '-' || content[lineStart] == '*') &&
			lineStart + 1 < length && content[lineStart + 1] == ' ') {
			// Apply accent color to bullet
			_ApplyStyle(lineStart, lineStart + 1, &fBoldFont, &kAccentColor);
		}

		pos = lineStart + 1;
	}
}
