//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "ChatMessageDisplayer.h"

#include "ClientGame.h"
#include "DisplayManager.h"
#include "FontManager.h"
#include "RenderUtils.h"
#include "ScissorsManager.h"
#include "UI.h"

namespace Zap
{

static const S32 FADE_TIME = 100;


void ColorTimerString::set(const string &s, bool useFadeTimer, const Color &c, S32 time, U32 id)    // id defaults to 0
{
   str = s;
   color = c;
   groupId = id;
   timer.reset(time);
   usingFadeTimer = useFadeTimer;
   fadeTimer.clear();
}


// Returns true if item is disappearing
bool ColorTimerString::idle(U32 timeDelta)
{
   if(timer.update(timeDelta))
   {
      // Main timer just expired!  Start the fade timer if we're using it.
      if(!usingFadeTimer)
         return true;

      fadeTimer.reset(FADE_TIME);
      return false;
   }
   
   // Main timer did not expire... either it's still going, or it finished earlier.  If the main 
   // timer has not yet expired, the fadeTimer will have a period of 0, which means the following
   // statment will return false.  Otherwise, if we're really ticking the fadeTimer, we'll return 
   // true when that timer expires.
   return fadeTimer.update(timeDelta);
}


////////////////////////////////////////
////////////////////////////////////////

// Defined constants needed here in .cpp because of usage with an std:: library
// e.g. std::min.  See:    http://stackoverflow.com/q/16957458
const U32 ChatMessageDisplayer::MAX_MESSAGES = 24;


// Constructor
ChatMessageDisplayer::ChatMessageDisplayer(ClientGame *game, S32 msgCount, bool topDown, S32 wrapWidth, S32 fontSize, S32 fontWidth)
{
   mChatScrollTimer.setPeriod(SCROLL_TIME);     // Transition time when new msg arrives (ms) 

   mMessages.resize(MAX_MESSAGES + 1);          // Have an extra message for scrolling effect.  Will only display msgCount messages.

   reset();

   mMessagesToShowInShortMode = msgCount;

   mGame      = game;
   mTopDown   = topDown;
   mWrapWidth = wrapWidth;
   mFontSize  = fontSize;
   mFontGap   = fontWidth;

   mDisplayMode = ShortTimeout;
   
   mNextGroupId = 0;
}


// Destructor
ChatMessageDisplayer::~ChatMessageDisplayer()
{
   // Do nothing
}


// Effectivley clears all messages
void ChatMessageDisplayer::reset()
{
   mFirst = mLast = 0;
   mFull = false;
   for(S32 i = 0; i < mMessages.size(); i++)
   {
      mMessages[i].timer.clear();
      mMessages[i].fadeTimer.clear();
      mMessages[i].str = "";
   }
}


void ChatMessageDisplayer::idle(U32 timeDelta, bool composingMessage)
{
   mChatScrollTimer.update(timeDelta);

   // Advance our message timers 
   for(S32 i = 0; i < mMessages.size(); i++)
      if(mMessages[i].idle(timeDelta))    // Returns true if message has expired and will no longer be displayed
      {
         mLast++;
         if(mTopDown && getMessageCount() <= getNumberOfMessagesToShow(composingMessage))
            mChatScrollTimer.reset();
      }
}


// User pressed Ctrl+M to cycle through the different message displays
void ChatMessageDisplayer::toggleDisplayMode()
{
   mDisplayMode = MessageDisplayMode((S32)mDisplayMode + 1);
   if(mDisplayMode == MessageDisplayModes)
      mDisplayMode =  MessageDisplayMode(0);
}


// Make room for a new message at the head of our list
void ChatMessageDisplayer::advanceFirst()
{
   mFirst++;

   if(mLast % mMessages.size() == mFirst % mMessages.size())
   {
      mLast++;
      mFull = true;
   }
}


U32 ChatMessageDisplayer::getMessageCount() const
{
   return mFirst - mLast;   
}


// Replace %vars% in chat messages 
// Currently only evaluates names of keybindings (as used in the INI file), and %playerName%
// Vars are case insensitive
static string getSubstVarVal(ClientGame *game, const string &var)
{
   // %keybinding%
   InputCode inputCode = game->getSettings()->getInputCodeManager()->getKeyBoundToBindingCodeName(var);
   if(inputCode != KEY_UNKNOWN)
      return string("[") + InputCodeManager::inputCodeToString(inputCode) + "]";
   
   // %playerName%
   if(caseInsensitiveStringCompare(var, "playerName"))
      return game->getClientInfo()->getName().getString();

   // Not a variable... preserve formatting
   return "%" + var + "%";
}


// Add it to the list, will be displayed in render()
void ChatMessageDisplayer::onChatMessageReceived(const Color &msgColor, const string &msg)
{
   FontManager::pushFontContext(ChatMessageContext);
   Vector<string> lines = wrapString(substitueVars(msg), mWrapWidth, mFontSize, "      ");   // Six spaces, if you're wondering...
   FontManager::popFontContext();

   // All lines from this message will share a groupId.  We'll use that to expire the group as a whole.
   for(S32 i = 0; i < lines.size(); i++)
   {
      advanceFirst();      // Make room for a new message at the top of the list (essentially mFirst++)
      mMessages[mFirst % mMessages.size()].set(lines[i], !mTopDown, msgColor, MESSAGE_EXPIRE_TIME, mNextGroupId); 
   }

   mNextGroupId++;

   if(!mTopDown)
      mChatScrollTimer.reset();
}


// Check if we have any %variables% that need substituting
string ChatMessageDisplayer::substitueVars(const string &str) const
{
   string s = str;      // Make working copy

   bool inside = false;

   std::size_t startPos, endPos;

   inside = false; 

   for(std::size_t i = 0; i < s.length(); i++)
   {
      if(s[i] == '%')
      {
         if(!inside)    // Found beginning of variable
         {
            startPos = i + 1;
            inside = true;
         }
         else           // Found end of variable
         {
            endPos = i - startPos;
            inside = false;

            string var = s.substr(startPos, endPos);
            string val = getSubstVarVal(mGame, var);

            s.replace(startPos - 1, endPos + 2, val);

            i += val.length() - var.length() - 2;     // Make sure we don't evaluate the contents of val; i.e. no recursion
         }
      }
   }

   return s;
}


// How many messages do we show, given our current display mode?
U32 ChatMessageDisplayer::getNumberOfMessagesToShow(bool composingMessage) const
{
   if(composingMessage)
      return MAX_MESSAGES;

   if(mDisplayMode == ShortTimeout || mDisplayMode == ShortFixed)
      return mMessagesToShowInShortMode;

   TNLAssert(mDisplayMode == LongFixed, "Unknown display mode!");

   return MAX_MESSAGES;     // Enough to fill the screen
}


// Some display modes will show messages even after their timer has expired.  Return whether the current display mode does that.
bool ChatMessageDisplayer::showExpiredMessages(bool composingMessage) const
{
   return composingMessage || (mDisplayMode != ShortTimeout);      // All other display modes show expired messages
}


bool ChatMessageDisplayer::isScrolling() const
{
   return mChatScrollTimer.getCurrent() > 0;
}


// Returns index of last message that should be displayed.
// This method has partial test coverage; add more if you need to change or debug.
U32 ChatMessageDisplayer::getCountOfMessagesToDisplay(F32 helperFadeIn, bool composingMessage) const
{
   if(mFirst == 0)
      return 0;

   if(helperFadeIn > 0)
      return min(MAX_MESSAGES, mFirst);

   bool scrolling = isScrolling();

   // Render an extra message while we're scrolling (in some cases).  Scissors will control the total vertical height.
   S32 messagesBeingScrolledOff = 0;

   if(scrolling)
   {
      if(mTopDown)
         messagesBeingScrolledOff = 1;
      else if(mFull)    // Only render extra item on bottom-up if list is fully occupied
         messagesBeingScrolledOff = 1;
   }

   U32 messagesToDisplay = 0;
   S32 scrollingMessageCount = 0;
   
   // Normally, we'll expect this loop to terminate with a break statement
   for(U32 i = mFirst; i > 0; i--)
   {
      S32 index = i % mMessages.size();    // Convert i into an index in our message list

      bool messageHasExpired = mMessages[index].timer.getCurrent() == 0 &&    // Current message has expired; and
                               mMessages[index].fadeTimer.getCurrent() == 0;  // Current message is not fading out

      if(messageHasExpired)
      {
         // We show expired messages in several circumstances: 1) We're composing a message; 2) The compose window
         // is fading in or out; 3) We're in a display mode where expired messages are always visible; or
         // 4) The expired message is scrolling off the display.
         // If none of these conditions are met, we won't show the expired message, and we can stop.
         if(!(composingMessage || helperFadeIn > 0 || mDisplayMode != ShortTimeout))
         {
            if(!scrolling)
               break;

            // Next we need to check if we're scrolling and we've already found our quota
            scrollingMessageCount++;
            if(scrollingMessageCount > messagesBeingScrolledOff)    
               break;
         }
      }

      // Check if we've found our limit of number of messages to display
      messagesToDisplay++;

      if(messagesToDisplay >= getNumberOfMessagesToShow(composingMessage))
         break;

   }

   return messagesToDisplay;
}


// Render any incoming player chat msgs.  Pass 0 for helperFadeIn if this is not a ChatMessageDisplayer that
// fades in and out the way in-game chat does.  (e.g. server messages do not fade)
void ChatMessageDisplayer::render(S32 anchorPos, F32 helperFadeIn, bool composingMessage, 
                                  bool anouncementActive, F32 baseAlpha) const
{
   // Are we in the act of transitioning between one message and another?
   bool scrolling = isScrolling();

   // Check if there any messages to display... if not, we're done
   U32 last = mFirst - getCountOfMessagesToDisplay(helperFadeIn, composingMessage);
   if(mFirst == last)
      return;

   S32 lineHeight = mFontSize + mFontGap;

   // Reuse this to avoid setup and breakdown costs
   static ScissorsManager scissorsManager;

   // For performance, only scissors if we're scrolling.  If we're not, we control the display by only showing
   // the specified number of lines; there are normally no partial lines that need vertical clipping as 
   // there are when we're scrolling.  Note also that we only clip vertically, and can ignore the horizontal.
   if(scrolling)    
   {
      // Remember that our message list contains an extra entry that exists only for scrolling purposes.
      // We want the height of the clip window to omit this line, so we subtract 1 below.  
      S32 displayAreaHeight = MAX_MESSAGES * lineHeight;
      S32 displayAreaYPos = anchorPos + (mTopDown ? displayAreaHeight : lineHeight);

      scissorsManager.enable(true, mGame->getSettings()->getSetting<DisplayMode>(IniKey::WindowMode),                // enable, mode
                             0, F32(displayAreaYPos - displayAreaHeight),                                            // x, y
                             F32(DisplayManager::getScreenInfo()->getGameCanvasWidth()), F32(displayAreaHeight));    // w, h
   }

   // Initialize the starting rendering position.  This represents the bottom of the message rendering area, and
   // we'll work our way up as we go.  In all cases, newest messages will appear on the bottom, older ones on top.
   // Note that anchorPos reflects something different (i.e. the top or the bottom of the area) in each case.
   S32 y = anchorPos + S32(mChatScrollTimer.getFraction() * lineHeight);
   if(mTopDown && scrolling)
      y -= lineHeight;

   // Advance anchor from top to the bottom of the render area.  When we are rendering at the bottom, anchorPos
   // already represents the bottom, so no additional adjustment is necessary.
   if(mTopDown)      // - 1 below because y is already correct for the first message.
      y += (mFirst - last - 1) * lineHeight;

   // y now represents the correct coordinate for rendering the most recently received message

   // Adjust our last line if we have an announcement
   if(anouncementActive)
   {
      // Render one fewer line if we're past the size threshold for this displayer
      if(mFirst >= U32(mMessages.size()) - 1)
         last++;

      y -= lineHeight;
   }

   FontManager::pushFontContext(ChatMessageContext);

   y += mFontSize;

   ////////////////////////////////
   // Draw message lines -- here we loop over all active messages; we may loop over more than we'll actually show.
   // At the end of this loop, we'll exit early once we've displayed the max number of messages we want to show.
   for(U32 i = mFirst; i > last; i--)
   {
      S32 index = i % mMessages.size();    // Convert i into an index in our message list

      F32 alpha = baseAlpha;

      // Fade if message is in the fade phase
      if(!showExpiredMessages(composingMessage) && mMessages[index].timer.getCurrent() == 0 && mMessages[index].fadeTimer.getCurrent() > 0)
         alpha *= mMessages[index].fadeTimer.getFraction();

      // If we've just started composing a chat message, older messages may need to fade onto the screen.  Apply more alpha if needed,
      // but only to the appropriate messages.  Messages that were already displayed do not fade in.
      if (helperFadeIn > 0 && helperFadeIn < 1 &&
            ((mMessages[index].timer.getCurrent() == 0 && mDisplayMode == ShortTimeout) ||
            (mFirst - i) >= getNumberOfMessagesToShow(false)))
         alpha *= helperFadeIn;

      FontManager::setFontColor(mMessages[index].color, alpha);
      RenderUtils::drawString_fixed(UserInterface::horizMargin, y, mFontSize, mMessages[index].str.c_str());

      y -= lineHeight;
   }

   FontManager::popFontContext();

   // Restore scissors settings -- only used during scrolling, but this call is cheap if scissors was not on
   scissorsManager.disable();
}

};
