# HaikuChat UI Redesign Plan

## Current Issues
1. **Text not visible after send** - Text color not being applied correctly after SetText()
2. **UI needs ChatGPT-style layout** - Current layout is basic, needs sidebar with chat history

## New UI Layout
```
+----------------------------------------------------------+
| [≡] HaikuChat                              [Settings]    |  <- Top bar with toggle
+--------+------------------------------------------------+
|        |                                                 |
| Chat   |                                                 |
| History|           Chat Messages Area                    |
| List   |                                                 |
|        |           (Scrollable)                          |
| [New]  |                                                 |
|        |                                                 |
|--------|                                                 |
|        |                                                 |
| [User] |  +------------------------------------------+   |
| Icon   |  | Type a message...              [Send]    |   |
+--------+------------------------------------------------+
```

## File Changes

### 1. Fix MessageBubble text visibility (MessageBubble.cpp)
- Reapply text color after SetText() in UpdateContent()
- Ensure color is set on initial text as well

### 2. New SidebarView class (SidebarView.cpp/.h)
- Collapsible panel on left side
- Contains:
  - "New Chat" button at top
  - Scrollable list of chat history items
  - User icon/button at bottom
- Width: ~250px when expanded, 0 when collapsed
- Each history item shows first line of conversation + timestamp

### 3. New ChatHistoryItem class (ChatHistoryItem.cpp/.h)
- Individual clickable item in sidebar
- Shows chat title (first message preview)
- Shows timestamp
- Delete button on hover/selection

### 4. Update MainWindow (MainWindow.cpp/.h)
- Add BSplitView for sidebar + main content
- Add toggle button in menu bar area
- Handle sidebar collapse/expand animation
- Manage multiple chat sessions
- Load/save chat history list

### 5. Update Settings (Settings.cpp/.h)
- Store multiple chat sessions (not just one)
- Each session: id, title, messages[], created_at, updated_at
- Store sidebar collapsed state
- Store current active chat id

### 6. Update Constants.h
- Add sidebar-related message codes
- Add sidebar colors and dimensions

### 7. Update Makefile
- Add new source files

## Implementation Order
1. Fix text visibility bug first (quick fix)
2. Update Settings for multiple chats
3. Create ChatHistoryItem widget
4. Create SidebarView
5. Refactor MainWindow with split layout
6. Add toggle functionality
7. Add user icon

## Chat Session Storage Format
```
~/config/settings/HaikuChat/
├── settings          # App settings (api, window frame, sidebar state)
└── chats/
    ├── chat_001.msg  # Individual chat sessions
    ├── chat_002.msg
    └── ...
```
