//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UIMENUS_H_
#define _UIMENUS_H_

#include "UI.h"                  // Parent class
#include "UIMenuItems.h"

#include "tnlNetConnection.h"    // for TerminationReason

#include <boost/shared_ptr.hpp>
#include "Test.h"


namespace Zap
{

using namespace std;


static const string UPLOAD_LEVELS = "UPLOAD LEVELS";
static const string ALL_LEVELS = "All Levels";


// This class is the template for most all of our menus...
class MenuUserInterface : public UserInterface
{
   typedef UserInterface Parent;

private:
   S32 checkMenuIndexBounds(S32 index) const;   // Returns corrected index
   Timer mScrollTimer;

   Timer mFadingNoticeTimer;
   S32 mFadingNoticeVerticalPosition;
   string mFadingNoticeMessage;

   // For detecting keys being held down
   bool mRepeatMode;
   bool mKeyDown;

   virtual void renderExtras() const;     // For drawing something extra on a menu
   void advanceItem();                    // What happens when we move on to the next menu item?

   virtual void initialize();

   void renderMenuInstructions(GameSettings *settings) const;
   static void renderArrow(S32 pos, bool pointingUp);

protected:
   Vector<boost::shared_ptr<MenuItem> > mMenuItems;

   S32 findFirstVisibleItem() const;                  // Calculates index of top-most visible item
   S32 getBaseYStart() const;                         // Base calculation for getYStart()
   virtual S32 getYStart() const;                     // Get vert pos of first menu item
   virtual S32 getTextSize(MenuItemSize size) const;  // Let menus set their own text size
   virtual S32 getGap(MenuItemSize size) const;       // Gap is the space between items

   string mMenuTitle;
   string mMenuSubTitle;

   Color mMenuSubTitleColor;
   bool mMenuFooterContainsInstructions;

   S32 mMaxMenuSize;
   S32 mFirstVisibleItem;        // Some menus have items than will fit on the screen; this is the index of the first visible item
   S32 mSelectedIndex;           // Index of the currently highlighted menu item (public so tests can access this member)
   bool mItemSelectedWithMouse;

   bool mRenderInstructions;
   bool mRenderSpecialInstructions;
   bool mIgnoreNextMouseEvent;

   // Handle keyboard input while a menu is displayed
   virtual bool processMenuSpecificKeys(InputCode inputCode);
   virtual bool processKeys(InputCode inputCode);
   virtual S32 getSelectedMenuItem();

   S32 getTotalMenuItemHeight() const; // Add up height of all menu items

   void sortMenuItems();
   MenuItem *getLastMenuItem() const;
   S32 getMaxFirstItemIndex() const;   // Calculates maximum index that the first item can have -- on non scrolling menus, this will be 0

   BfObject *mAssociatedObject;        // Some menus can have an associated object... this is it

public:
   // Constructors
   MenuUserInterface(ClientGame *game, UIManager *uiManager);
   MenuUserInterface(ClientGame *game, UIManager *uiManager, const string &title);
   virtual ~MenuUserInterface();

   bool isScrollingMenu() const;

   void clearMenuItems();
   S32 addMenuItem(MenuItem *menuItem);
   void addWrappedMenuItem(boost::shared_ptr<MenuItem> menuItem);
   MenuItem *getMenuItem(S32 index) const;
   S32 getMenuItemCount() const;

   virtual void idle(U32 timeDelta); 

   void getMenuResponses(Vector<string> &responses);     // Fill responses with values from menu

   void render() const;                                        // Draw the basic menu
   bool onKeyDown(InputCode inputCode);
   void onKeyUp(InputCode inputCode);
   void onTextInput(char ascii);
   void onMouseMoved();
   void processMouse();

   virtual void onActivate();
   virtual void onReactivate();
   virtual void onEscape();

   BfObject *getAssociatedObject();
   void setAssociatedObject(BfObject *obj);

   void setSubtitle(const string &subtitle);

   void setFadingNotice(U32 time, S32 top, const string &message);  // Set a fading notice on a menu
   void clearFadingNotice();                                        // Clear a fading notice

   ///// Testing
   FRIEND_TEST(LevelMenuSelectUserInterfaceTests, GetIndexOfNext);
};


class MenuUserInterfaceWithIntroductoryAnimation : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

private:
   static const S32 FadeInTime = 400;       // Post animation fade in time (ms) 

   static bool mFirstTime;    // Is this the first time an intro menu is being shown?
   Timer mFadeInTimer;        // Track the brief fade in interval the first time menu is shown
   bool mShowingAnimation;    // Is intro animation currently being played?

public:
   explicit MenuUserInterfaceWithIntroductoryAnimation(ClientGame *game, UIManager *uiManager);
   virtual ~MenuUserInterfaceWithIntroductoryAnimation();

   void onActivate();
   void idle(U32 timeDelta);
   void render() const;
   bool onKeyDown(InputCode inputCode);
   void processSelection(U32 index);
};


////////////////////////////////////////
////////////////////////////////////////

// <--- DO NOT SUBCLASS MainMenuUserInterface!! (unless you override onActivate) ---> //
class MainMenuUserInterface : public MenuUserInterfaceWithIntroductoryAnimation
{
   typedef MenuUserInterfaceWithIntroductoryAnimation Parent;

private:
   string mMotd;
   S32 mMotdPos;

   Timer mColorTimer;
   Timer mColorTimer2;
   
   static const S32 ColorTime = 1000;
   static const S32 ColorTime2 = 1700;
   
   bool mTransDir;
   bool mTransDir2;
   bool mNeedToUpgrade;       // True if client is out of date and needs to upgrade, false if we're on the latest version
   bool mShowedUpgradeAlert;  // So we don't show the upgrade message more than once

   S32 getYStart() const;

   void renderExtras() const;

public:
   explicit MainMenuUserInterface(ClientGame *game, UIManager *uiManager);    // Constructor
   virtual ~MainMenuUserInterface();

   void onEscape();
   void render() const;
   void idle(U32 timeDelta); 
   void setMOTD(const string &motd);            // Message of the day, from Master
   void onActivate();
   void setNeedToUpgrade(bool needToUpgrade);   // Is client in need of an upgrade?

   void showUpgradeAlert();                     // Display message to the user that it is time to upgrade
   bool getNeedToUpgrade();
};


////////////////////////////////////////
////////////////////////////////////////

class OptionsMenuUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

public:
   explicit OptionsMenuUserInterface(ClientGame *game, UIManager *uiManager);    // Constructor
   virtual ~OptionsMenuUserInterface();

   void onEscape();
   void setupMenus();
   void onActivate();
   void toggleDisplayMode();
};


////////////////////////////////////////
////////////////////////////////////////

class InputOptionsMenuUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

public:
   explicit InputOptionsMenuUserInterface(ClientGame *game, UIManager *uiManager);  // Constructor
   virtual ~InputOptionsMenuUserInterface();

   void render() const;

   void onEscape();
   void setupMenus();
   void onActivate();
};


////////////////////////////////////////
////////////////////////////////////////

class SoundOptionsMenuUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

public:
   explicit SoundOptionsMenuUserInterface(ClientGame *game, UIManager *uiManager);  // Constructor
   virtual ~SoundOptionsMenuUserInterface();

   void onEscape();
   void setupMenus();
   void onActivate();
};


////////////////////////////////////////
////////////////////////////////////////

class InGameHelpOptionsUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

public:
   explicit InGameHelpOptionsUserInterface(ClientGame *game, UIManager *uiManager);    // Constructor
   virtual ~InGameHelpOptionsUserInterface();

   void onEscape();
   void setupMenus();
   void onActivate();
};


////////////////////////////////////////
////////////////////////////////////////

class EditorOptionsUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

public:
   explicit EditorOptionsUserInterface(ClientGame *game, UIManager *uiManager);    // Constructor
   virtual ~EditorOptionsUserInterface();

   void onEscape();
   void setupMenus();
   void onActivate();
};


////////////////////////////////////////
////////////////////////////////////////

class RobotOptionsMenuUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

public:
   explicit RobotOptionsMenuUserInterface(ClientGame *game, UIManager *uiManager);     // Constructor
   virtual ~RobotOptionsMenuUserInterface();

   void onEscape();
   void setupMenus();
   void onActivate();
   void saveSettings();
};

////////////////////////////////////////
////////////////////////////////////////

class ServerInfoMenuUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

public:
   explicit ServerInfoMenuUserInterface(ClientGame *game, UIManager *uiManager);     // Constructor
   virtual ~ServerInfoMenuUserInterface();

   void onEscape();
   void setupMenus();
   void onActivate();
   void saveSettings() const;
};


////////////////////////////////////////
////////////////////////////////////////

class ServerPasswordsMenuUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

public:
   explicit ServerPasswordsMenuUserInterface(ClientGame *game, UIManager *uiManager);     // Constructor
   virtual ~ServerPasswordsMenuUserInterface();

   void onEscape();
   void setupMenus();
   void onActivate();
   void saveSettings();
};


////////////////////////////////////////
////////////////////////////////////////

class HostMenuUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

private:
   S32 mEditingIndex;                        // Index of item we're editing, -1 if none

public:
   explicit HostMenuUserInterface(ClientGame *game, UIManager *uiManager);    // Constructor
   virtual ~HostMenuUserInterface();

   void onEscape();
   void setupMenus();
   void onActivate();
   void render() const;
   void saveSettings();       // Save parameters in INI file
};


////////////////////////////////////////
////////////////////////////////////////

// Player enters their name and password
class NameEntryUserInterface : public MenuUserInterfaceWithIntroductoryAnimation
{
   typedef MenuUserInterfaceWithIntroductoryAnimation Parent;

private:
   NetConnection::TerminationReason mReason;
   void renderExtras() const;

public:
   explicit NameEntryUserInterface(ClientGame *game, UIManager *uiManager);    // Constructor
   virtual ~NameEntryUserInterface();

   void onEscape();
   void setupMenu();
   void onActivate();
   void setReactivationReason(NetConnection::TerminationReason r);
};


////////////////////////////////////////
////////////////////////////////////////

class GameType;

class GameMenuUserInterface : public MenuUserInterface
{
   enum {
      OPT_OPTIONS,
      OPT_HELP,
      OPT_ADMIN,
      OPT_KICK,
      OPT_LEVEL_CHANGE_PW,
      OPT_CHOOSELEVEL,
      OPT_ADD2MINS,
      OPT_RESTART,
      OPT_QUIT
   };

private:
   typedef MenuUserInterface Parent;
   SafePtr<GameType> mGameType;
   InputMode lastInputMode;
   void buildMenu();

public:
   explicit GameMenuUserInterface(ClientGame *game, UIManager *uiManager);    // Constructor
   virtual ~GameMenuUserInterface();

   void idle(U32 timeDelta);
   void onActivate();
   void onReactivate();
   void onEscape();
};


////////////////////////////////////////
////////////////////////////////////////

class LevelMenuUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

public:
   explicit LevelMenuUserInterface(ClientGame *game, UIManager *uiManager);      // Constructor
   virtual ~LevelMenuUserInterface();

   void onActivate();
};


////////////////////////////////////////
////////////////////////////////////////

class RobotsMenuUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

public:
   explicit RobotsMenuUserInterface(ClientGame *game, UIManager *uiManager);     // Constructor
   virtual ~RobotsMenuUserInterface();

   void onActivate();
};


////////////////////////////////////////
////////////////////////////////////////

class AdminMenuUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

private:
   SafePtr<GameType> mGameType;

public:
   AdminMenuUserInterface();      // Constructor
   virtual ~AdminMenuUserInterface();

   void onActivate();
   void processSelection(U32 index);
   void processShiftSelection(U32 index);
   void onEscape();
};


////////////////////////////////////////
////////////////////////////////////////

class TeamMenuUserInterface : public MenuUserInterface
{
   typedef MenuUserInterface Parent;

public:
   explicit TeamMenuUserInterface(ClientGame *game, UIManager *uiManager);    // Constructor
   virtual ~TeamMenuUserInterface();

   string mNameToChange;

   void idle(U32 timeDelta);

   void processSelection(U32 index);
};


};


#endif      // #ifndef _UIMENUS_H_

