//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008-2009 Chris Eykamp
// Original work copyright (C) 2004 GarageGames.com, Inc.
// Other code copyright as noted
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#include "SymbolShape.h"

#include "FontManager.h"
#include "InputCode.h"
#include "Joystick.h"
#include "JoystickRender.h"

#include "gameObjectRender.h"
#include "Colors.h"

#include "OpenglUtils.h"
#include "RenderUtils.h"

using namespace TNL;


namespace Zap { namespace UI {


void SymbolStringSetCollection::clear()
{
   mSymbolSet.clear();
   mAlignment.clear();
   mXPos.clear();
}


void SymbolStringSetCollection::addSymbolStringSet(const SymbolStringSet &set, Alignment alignment, S32 xpos)
{
   mSymbolSet.push_back(set);
   mAlignment.push_back(alignment);
   mXPos.push_back(xpos);
}


S32 SymbolStringSetCollection::render(S32 yPos) const
{
   S32 lines = -1;

   // Figure out how many lines in our tallest SymbolStringSet
   for(S32 i = 0; i < mSymbolSet.size(); i++)
      lines = max(lines, mSymbolSet[i].getItemCount());

   // Render the SymbolStringSets line-by-line, keeping all lines aligned with one another.
   // Make a tally of the total height along the way (using the height of the tallest item rendered).
   S32 totalHeight = 0;

   for(S32 i = 0; i < lines; i++)
   {
      S32 height = 0;

      for(S32 j = 0; j < mSymbolSet.size(); j++)
      {
         S32 h = mSymbolSet[j].renderLine(i, mXPos[j], yPos + totalHeight, mAlignment[j]);
         height = max(h, height);   // Find tallest
      }

      totalHeight += height;
   }

   return totalHeight;
}


////////////////////////////////////////
////////////////////////////////////////


SymbolStringSet::SymbolStringSet(S32 gap)
{
   mGap = gap;
}


void SymbolStringSet::clear()
{
   mSymbolStrings.clear();
}


void SymbolStringSet::add(const SymbolString &symbolString)
{
   mSymbolStrings.push_back(symbolString);
}


S32 SymbolStringSet::getHeight() const
{
   S32 height = 0;
   for(S32 i = 0; i < mSymbolStrings.size(); i++)
      height += mSymbolStrings[i].getHeight() + (mSymbolStrings[i].getHasGap() ? mGap : 0);

   return height;
}


S32 SymbolStringSet::getItemCount() const
{
   return mSymbolStrings.size();
}


void SymbolStringSet::render(S32 x, S32 y, Alignment alignment) const
{
   for(S32 i = 0; i < mSymbolStrings.size(); i++)
   {
      mSymbolStrings[i].render(x, y, alignment);
      y += mSymbolStrings[i].getHeight() + mGap;
   }
}


S32 SymbolStringSet::renderLine(S32 line, S32 x, S32 y, Alignment alignment) const
{
   // Make sure we're in bounds
   if(line >= mSymbolStrings.size())
      return 0;

   mSymbolStrings[line].render(x, y, alignment);
   return mSymbolStrings[line].getHeight() + (mSymbolStrings[line].getHasGap() ? mGap : 0);
}


////////////////////////////////////////
////////////////////////////////////////

// Width is the sum of the widths of all elements in the symbol list
static S32 computeWidth(const Vector<SymbolShapePtr> &symbols, S32 fontSize, FontContext fontContext)
{
   S32 width = 0;

   for(S32 i = 0; i < symbols.size(); i++)
      width += symbols[i]->getWidth();

   return width;
}


// Width of a layered item is the widest of the widths of all elements in the symbol list
static S32 computeLayeredWidth(const Vector<SymbolShapePtr> &symbols, S32 fontSize, FontContext fontContext)
{
   S32 width = 0;

   for(S32 i = 0; i < symbols.size(); i++)
   {
      S32 w = symbols[i]->getWidth();

      width = max(w, width);
   }

   return width;
}

// Height is the height of the tallest element in the symbol list
static S32 computeHeight(const Vector<SymbolShapePtr> &symbols, S32 fontSize, FontContext fontContext)
{
   S32 height = 0;

   for(S32 i = 0; i < symbols.size(); i++)
   {
      S32 h = symbols[i]->getHeight();
      height = max(h, height);
   }

   return height;
}


// Constructor with symbols
SymbolString::SymbolString(const Vector<SymbolShapePtr> &symbols, S32 fontSize, FontContext fontContext) : mSymbols(symbols)
{
   mFontSize    = fontSize;
   mFontContext = fontContext;
   mReady = true;

   mWidth = computeWidth(symbols, fontSize, fontContext);
   mHeight = computeHeight(symbols, fontSize, fontContext);
}


// Constructor -- symbols will be provided later
SymbolString::SymbolString(S32 fontSize, FontContext fontContext)
{
   mFontSize    = fontSize;
   mFontContext = fontContext;
   mReady = false;

   mWidth = 0;
}


// Destructor
SymbolString::~SymbolString()
{
  // Do nothing
}


void SymbolString::setSymbols(const Vector<SymbolShapePtr> &symbols)
{
   mSymbols = symbols;

   mWidth = computeWidth(symbols, mFontSize, mFontContext);
   mReady = true;
}


S32 SymbolString::getWidth() const
{ 
   TNLAssert(mReady, "Not ready!");

   return mWidth;
}


S32 SymbolString::getHeight() const
{ 
   TNLAssert(mReady, "Not ready!");

   S32 height = -1;
   for(S32 i = 0; i < mSymbols.size(); i++)
      height = max(height, mSymbols[i]->getHeight());

   return height;
}


// Here to make class non-virtual
void SymbolString::render(const Point &pos) const
{
   render(pos, AlignmentCenter);
}


void SymbolString::render(const Point &center, Alignment alignment) const
{
   render((S32)center.x, (S32)center.y, alignment);
}


void SymbolString::render(S32 x, S32 y, Alignment alignment) const
{
   TNLAssert(mReady, "Not ready!");

   // Alignment of overall symbol string
   if(alignment == AlignmentCenter)
      x -= mWidth / 2;     // x is now at the left edge of the render area

   for(S32 i = 0; i < mSymbols.size(); i++)
   {
      S32 w = mSymbols[i]->getWidth();
      mSymbols[i]->render(Point(x + w / 2, y));
      x += w;
   }
}


bool SymbolString::getHasGap() const
{
   for(S32 i = 0; i < mSymbols.size(); i++)
      if(mSymbols[i]->getHasGap())
         return true;

   return false;
}


static const S32 buttonHalfHeight = 9;   // This is the default half-height of a button
static const S32 rectButtonWidth = 24;
static const S32 rectButtonHeight = 18;
static const S32 smallRectButtonWidth = 19;
static const S32 smallRectButtonHeight = 15;
static const S32 horizEllipseButtonDiameterX = 28;
static const S32 horizEllipseButtonDiameterY = 16;
static const S32 rightTriangleWidth = 28;
static const S32 rightTriangleHeight = 18;
static const S32 RectRadius = 3;
static const S32 RoundedRectRadius = 5;


static SymbolShapePtr getSymbol(InputCode inputCode, const Color *color);

static SymbolShapePtr getSymbol(Joystick::ButtonShape shape, const Color *color)
{
   switch(shape)
   {
      case Joystick::ButtonShapeRound:
         return SymbolShapePtr(new SymbolCircle(buttonHalfHeight, color));

      case Joystick::ButtonShapeRect:
         return SymbolShapePtr(new SymbolRoundedRect(rectButtonWidth, 
                                                     rectButtonHeight, 
                                                     RectRadius,
                                                     color));

      case Joystick::ButtonShapeSmallRect:
         return SymbolShapePtr(new SymbolSmallRoundedRect(smallRectButtonWidth, 
                                                          smallRectButtonHeight, 
                                                          RectRadius,
                                                          color));

      case Joystick::ButtonShapeRoundedRect:
         return SymbolShapePtr(new SymbolRoundedRect(rectButtonWidth, 
                                                     rectButtonHeight, 
                                                     RoundedRectRadius,
                                                     color));

      case Joystick::ButtonShapeSmallRoundedRect:
         return SymbolShapePtr(new SymbolSmallRoundedRect(smallRectButtonWidth, 
                                                          smallRectButtonHeight, 
                                                          RoundedRectRadius,
                                                          color));
                                                     
      case Joystick::ButtonShapeHorizEllipse:
         return SymbolShapePtr(new SymbolHorizEllipse(horizEllipseButtonDiameterX, 
                                                      horizEllipseButtonDiameterY,
                                                      color));

      case Joystick::ButtonShapeRightTriangle:
         return SymbolShapePtr(new SymbolRightTriangle(rightTriangleWidth, color));

      default:
         //TNLAssert(false, "Unknown button shape!");
         return getSymbol(KEY_UNKNOWN, &Colors::red);
   }
}


static SymbolShapePtr getSymbol(Joystick::ButtonShape shape, const string &label, const Color *color)
{
   static const S32 LabelSize = 13;
   Vector<SymbolShapePtr> symbols;
   
   // Get the button outline
   SymbolShapePtr shapePtr = getSymbol(shape, color);

   symbols.push_back(shapePtr);

   // Handle some special cases -- there are some button labels that refer to special glyphs
   Joystick::ButtonSymbol buttonSymbol = Joystick::stringToButtonSymbol(label);

   if(buttonSymbol == Joystick::ButtonSymbolNone)
      symbols.push_back(SymbolShapePtr(new SymbolText(label, LabelSize + shapePtr->getLabelSizeAdjustor(label, LabelSize), 
                                                      KeyContext, shapePtr->getLabelOffset(label, LabelSize))));
   else
      symbols.push_back(SymbolShapePtr(new SymbolButtonSymbol(buttonSymbol)));

   return SymbolShapePtr(new LayeredSymbolString(symbols, LabelSize, KeyContext));
}


static S32 KeyFontSize = 13;     // Size of characters used for rendering key bindings

// Color is ignored for controller buttons
static SymbolShapePtr getSymbol(InputCode inputCode, const Color *color)
{
   if(InputCodeManager::isKeyboardKey(inputCode))
   {
      const char *str = InputCodeManager::inputCodeToString(inputCode);
      return SymbolShapePtr(new SymbolKey(str, color));
   }
   else if(inputCode == LEFT_JOYSTICK)
      return SymbolString::getSymbolText("Left Joystick", KeyFontSize, KeyContext, color);
   else if(inputCode == RIGHT_JOYSTICK)
      return SymbolString::getSymbolText("Right Joystick", KeyFontSize, KeyContext, color);
   else if(inputCode == MOUSE_LEFT)
      return SymbolString::getSymbolText("Left Mouse Button", KeyFontSize, KeyContext, color);
   else if(inputCode == MOUSE_MIDDLE)
      return SymbolString::getSymbolText("Middle Mouse Button", KeyFontSize, KeyContext, color);
   else if(inputCode == MOUSE_RIGHT)
      return SymbolString::getSymbolText("Right Mouse Button", KeyFontSize, KeyContext, color);
   else if(inputCode == MOUSE_WHEEL_UP)
      return SymbolString::getSymbolText("Mouse Wheel Up", KeyFontSize, KeyContext, color);
   else if(inputCode == MOUSE_WHEEL_DOWN)
      return SymbolString::getSymbolText("Mouse Wheel Down", KeyFontSize, KeyContext, color);
   else if(inputCode == MOUSE)
      return SymbolString::getSymbolText("Mouse", KeyFontSize, KeyContext, color);
   else if(InputCodeManager::isCtrlKey(inputCode))
   {
      Vector<SymbolShapePtr> symbols;
      
      symbols.push_back(SymbolShapePtr(new SymbolKey(InputCodeManager::getModifierString(inputCode), color)));
      symbols.push_back(SymbolShapePtr(new SymbolText(" + ", 13, KeyContext, color)));
      symbols.push_back(SymbolShapePtr(new SymbolKey(InputCodeManager::getBaseKeyString(inputCode), color)));

      return SymbolShapePtr(new SymbolString(symbols, 10, KeyContext));
   }
   else if(InputCodeManager::isControllerButton(inputCode))
   {
      // This gives us the logical button that inputCode represents... something like JoystickButton3
      JoystickButton button = InputCodeManager::inputCodeToJoystickButton(inputCode);

      // Now we need to figure out which symbol to use for this button, depending on controller make/model
      Joystick::ButtonInfo buttonInfo = Joystick::JoystickPresetList[Joystick::SelectedPresetIndex].buttonMappings[button];

      if(!Joystick::isButtonDefined(Joystick::SelectedPresetIndex, button))
         return getSymbol(KEY_UNKNOWN, color);

      // This gets us the button shape index, which will tell us what to draw... something like ButtonShapeRound
      Joystick::ButtonShape buttonShape = buttonInfo.buttonShape;

      SymbolShapePtr symbol = getSymbol(buttonShape, buttonInfo.label, &buttonInfo.color);

      return symbol;

   }
   else if(inputCode == KEY_UNKNOWN)
   {
      return SymbolShapePtr(new SymbolUnknown(color));
   }
   else
   {
      return getSymbol(KEY_UNKNOWN, color);
   }
}


// Static method
SymbolShapePtr SymbolString::getControlSymbol(InputCode inputCode, const Color *color)
{
   return getSymbol(inputCode, color);
}


// Static method
SymbolShapePtr SymbolString::getSymbolGear(S32 fontSize)
{
   return SymbolShapePtr(new SymbolGear(fontSize));
}


// Static method
SymbolShapePtr SymbolString::getSymbolText(const string &text, S32 fontSize, FontContext context, const Color *color)
{
   return SymbolShapePtr(new SymbolText(text, fontSize, context, color));
}


// Static method
SymbolShapePtr SymbolString::getBlankSymbol(S32 width, S32 height)
{
   return SymbolShapePtr(new SymbolBlank(width, height));
}


// Static method
SymbolShapePtr SymbolString::getHorizLine(S32 length, S32 height, const Color *color)
{
   return SymbolShapePtr(new SymbolHorizLine(length, height, color));
}


SymbolShapePtr SymbolString::getHorizLine(S32 length, S32 vertOffset, S32 height, const Color *color)
{
   return SymbolShapePtr(new SymbolHorizLine(length, vertOffset, height, color));
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor
LayeredSymbolString::LayeredSymbolString(const Vector<boost::shared_ptr<SymbolShape> > &symbols, S32 fontSize, FontContext fontContext) :
                  Parent(symbols, fontSize, fontContext)
{
   mWidth = computeLayeredWidth(symbols, fontSize, fontContext);
}


// Destructor
LayeredSymbolString::~LayeredSymbolString()
{
   // Do nothing
}


// Each layer is rendered atop the previous, creating a layered effect
void LayeredSymbolString::render(S32 x, S32 y, Alignment alignment) const
{
   TNLAssert(mReady, "Not ready!");

   // Alignment of overall symbol string
   //if(alignment == AlignmentCenter)
   //   x -= mWidth / 2;

   FontManager::pushFontContext(mFontContext);

   for(S32 i = 0; i < mSymbols.size(); i++)
      mSymbols[i]->render(Point(x, y));

   FontManager::popFontContext();
}


////////////////////////////////////////
////////////////////////////////////////


SymbolShape::SymbolShape(S32 width, S32 height, const Color *color) : mColor(color)
{
   mWidth = width;
   mHeight = height;
   mHasColor = color != NULL;
   mLabelSizeAdjustor = 0;
}


// Destructor
SymbolShape::~SymbolShape()
{
   // Do nothing
}


S32 SymbolShape::getWidth() const
{
   return mWidth;
}


S32 SymbolShape::getHeight() const
{
   return mHeight;
}


bool SymbolShape::getHasGap() const
{
   return false;
}


Point SymbolShape::getLabelOffset(const string &label, S32 labelSize) const
{
   return mLabelOffset;
}


S32 SymbolShape::getLabelSizeAdjustor(const string &label, S32 labelSize) const
{
   return mLabelSizeAdjustor;
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor
SymbolBlank::SymbolBlank(S32 width, S32 height) : Parent(width, height, NULL)
{
   // Do nothing
}


// Destructor
SymbolBlank::~SymbolBlank()
{
   // Do nothing
}


void SymbolBlank::render(const Point &center) const
{
   // Do nothing -- it's blank, remember?
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor
SymbolHorizLine::SymbolHorizLine(S32 length, S32 height, const Color *color) : Parent(length, height, color)
{
   mVertOffset = 0;
}


// Constructor
SymbolHorizLine::SymbolHorizLine(S32 length, S32 vertOffset, S32 height, const Color *color) : Parent(length, height, color)
{
   mVertOffset = vertOffset;
}


// Destructor
SymbolHorizLine::~SymbolHorizLine()
{
   // Do nothing
}


void SymbolHorizLine::render(const Point &center) const
{
   if(mHasColor)
      glColor(mColor);

   drawHorizLine(center.x - mWidth / 2, center.x + mWidth / 2, center.y - mHeight / 2 + mVertOffset);
}


////////////////////////////////////////
////////////////////////////////////////

static const S32 BorderDecorationVertCenteringOffset = 2;   // Offset the border of keys and buttons to better center them in the flow of text
static const S32 SpacingAdjustor = 2;


// Constructor
SymbolRoundedRect::SymbolRoundedRect(S32 width, S32 height, S32 radius, const Color *color) : 
                                                                  Parent(width + SpacingAdjustor, height + SpacingAdjustor, color)
{
   mRadius = radius;
}


// Destructor
SymbolRoundedRect::~SymbolRoundedRect()
{
   // Do nothing
}


void SymbolRoundedRect::render(const Point &center) const
{
   if(mHasColor)
      glColor(mColor);

   drawRoundedRect(center - Point(0, (mHeight - SpacingAdjustor) / 2 - BorderDecorationVertCenteringOffset - 1), 
                   mWidth - SpacingAdjustor, mHeight - SpacingAdjustor, mRadius);
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
SymbolSmallRoundedRect::SymbolSmallRoundedRect(S32 width, S32 height, S32 radius, const Color *color) : 
                                                               Parent(width + SpacingAdjustor, height + SpacingAdjustor, radius, color)
{
   mLabelOffset.set(0, -1);
}


// Destructor
SymbolSmallRoundedRect::~SymbolSmallRoundedRect()
{
   // Do nothing
}


void SymbolSmallRoundedRect::render(const Point &center) const
{
   if(mHasColor)
      glColor(mColor);

   drawRoundedRect(center - Point(0, mHeight / 2 - BorderDecorationVertCenteringOffset - SpacingAdjustor + 2), 
                   mWidth - SpacingAdjustor, mHeight - SpacingAdjustor, mRadius);
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor
SymbolHorizEllipse::SymbolHorizEllipse(S32 width, S32 height, const Color *color) : Parent(width + 2, height, color)
{
   mLabelOffset.set(0, -1);
}


// Destructor
SymbolHorizEllipse::~SymbolHorizEllipse()
{
   // Do nothing
}


void SymbolHorizEllipse::render(const Point &center) const
{
   S32 w = mWidth / 2;
   S32 h = mHeight / 2;

   if(mHasColor)
      glColor(mColor);

   Point cen = center - Point(0, h - 1);

   // First the fill
   drawFilledEllipse(cen, w, h, 0);

   // Outline in white
   glColor(Colors::white);
   drawEllipse(cen, w, h, 0);
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor
SymbolRightTriangle::SymbolRightTriangle(S32 width, const Color *color) : Parent(width, 19, color)
{
   mLabelOffset.set(-5, -1);
   mLabelSizeAdjustor = -3;
}


// Destructor
SymbolRightTriangle::~SymbolRightTriangle()
{
   // Do nothing
}


static void drawButtonRightTriangle(const Point &center)
{
   Point p1(center + Point(-6, -15));
   Point p2(center + Point(-6,   4));
   Point p3(center + Point(21,  -6));

   F32 vertices[] = {
         p1.x, p1.y,
         p2.x, p2.y,
         p3.x, p3.y
   };
   renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, GL_LINE_LOOP);
}


void SymbolRightTriangle::render(const Point &center) const
{
   if(mHasColor)
      glColor(mColor);

   Point cen(center.x -mWidth / 4, center.y);  // Need to off-center the label slightly for this button
   drawButtonRightTriangle(cen);
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor
SymbolCircle::SymbolCircle(S32 radius, const Color *color) : Parent(radius * 2 + SpacingAdjustor, radius * 2 + SpacingAdjustor, color)
{
   // Do nothing
}


// Destructor
SymbolCircle::~SymbolCircle()
{
   // Do nothing
}


void SymbolCircle::render(const Point &pos) const
{
   if(mHasColor)
      glColor(mColor);

   // Adjust our position's y coordinate to be the center of the circle
   drawCircle(pos - Point(0, (mHeight) / 2 - BorderDecorationVertCenteringOffset - SpacingAdjustor), F32(mWidth - SpacingAdjustor) / 2);
}


static const S32 LabelAutoShrinkThreshold = 15;

S32 SymbolCircle::getLabelSizeAdjustor(const string &label, S32 labelSize) const
{
   // Shrink labels a little when the text is uncomfortably big for the button
   if(getStringWidth(labelSize, label.c_str()) > LabelAutoShrinkThreshold)
      return mLabelSizeAdjustor - 2;
   
   return mLabelSizeAdjustor;
}


Point SymbolCircle::getLabelOffset(const string &label, S32 labelSize) const
{
   if(getStringWidth(labelSize, label.c_str()) > LabelAutoShrinkThreshold)
      return mLabelOffset + Point(0, -1);

   return mLabelOffset;
}


////////////////////////////////////////
////////////////////////////////////////


SymbolButtonSymbol::SymbolButtonSymbol(Joystick::ButtonSymbol glyph)
{
   mGlyph = glyph;
}


SymbolButtonSymbol::~SymbolButtonSymbol()
{
   // Do nothing
}


void SymbolButtonSymbol::render(const Point &pos) const
{
   // Get symbol in the proper position for rendering -- it's either this or change all the render methods
   Point renderPos = pos + Point(0, -6);   

   switch(mGlyph)
   {
      case Joystick::ButtonSymbolPsCircle:
         JoystickRender::drawPlaystationCircle(renderPos);
         break;
      case Joystick::ButtonSymbolPsCross:
         JoystickRender::drawPlaystationCross(renderPos);
         break;
      case Joystick::ButtonSymbolPsSquare:
         JoystickRender::drawPlaystationSquare(renderPos);
         break;
      case Joystick::ButtonSymbolPsTriangle:
         JoystickRender::drawPlaystationTriangle(renderPos);
         break;
      case Joystick::ButtonSymbolSmallLeftTriangle:
         JoystickRender::drawSmallLeftTriangle(renderPos + Point(0, -1));
         break;
      case Joystick::ButtonSymbolSmallRightTriangle:
         JoystickRender::drawSmallRightTriangle(renderPos + Point(0, -1));
         break;
      case Joystick::ButtonSymbolNone:
      default:
         TNLAssert(false, "Shouldn't be here!");
   }
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor
SymbolGear::SymbolGear(S32 fontSize) : Parent(0, NULL)
{
   mWidth = S32(1.333f * fontSize);    // mWidth is effectively a diameter; we'll use mWidth / 2 for our rendering radius
   mHeight = mWidth;
}


// Destructor
SymbolGear::~SymbolGear()
{
   // Do nothing
}


void SymbolGear::render(const Point &pos) const
{
   // We are given the bottom y position of the line, but the icon expects the center
   Point center(pos.x, (pos.y - mHeight/2) + 2); // Slight downward adjustment to position to better align with text
   renderLoadoutZoneIcon(center, mWidth / 2);
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor with no vertical offset
SymbolText::SymbolText(const string &text, S32 fontSize, FontContext context, const Color *color) : 
                              Parent(getStringWidth(context, fontSize, text.c_str()), fontSize, color)
{
   mText = text;
   mFontContext = context;
   mFontSize = fontSize;
}


// Constructor with vertical offset -- not used?
SymbolText::SymbolText(const string &text, S32 fontSize, FontContext context, const Point &labelOffset, const Color *color) : 
                                       Parent(getStringWidth(context, fontSize, text.c_str()), fontSize, color)
{
   mText = text;
   mFontContext = context;
   mFontSize = fontSize;
   mLabelOffset = labelOffset;

   mHeight = fontSize;
}


// Destructor
SymbolText::~SymbolText()
{
   // Do nothing
}


void SymbolText::render(const Point &center) const
{
   if(mHasColor)
      glColor(mColor);

   FontManager::pushFontContext(mFontContext);
   drawStringc(center + mLabelOffset, (F32)mFontSize, mText.c_str());
   FontManager::popFontContext();
}


S32 SymbolText::getHeight() const
{
   return Parent::getHeight() + (S32)mLabelOffset.y;
}


bool SymbolText::getHasGap() const
{
   return true;
}


////////////////////////////////////////
////////////////////////////////////////


static S32 Margin = 3;              // Buffer within key around text
static S32 Gap = 3;                 // Distance between keys
static S32 TotalHeight = KeyFontSize + 2 * Margin;


static S32 getKeyWidth(const string &text, S32 height)
{
   S32 width = -1;
   if(text == "Up Arrow" || text == "Down Arrow" || text == "Left Arrow" || text == "Right Arrow")
      width = 0;     // Make a square button; mWidth will be set to mHeight below
   else
      width = getStringWidth(KeyContext, KeyFontSize, text.c_str()) + Margin * 2;

   return max(width, height) + BorderDecorationVertCenteringOffset * Gap;
}


SymbolKey::SymbolKey(const string &text, const Color *color) : Parent(text, KeyFontSize, KeyContext, color)
{
   mHeight = TotalHeight;
   mWidth = getKeyWidth(text, mHeight);
}


// Destructor
SymbolKey::~SymbolKey()
{
   // Do nothing
}


// Note: passed font size and context will be ignored
void SymbolKey::render(const Point &center) const
{
   // Compensate for the fact that boxes draw from center
   const Point boxVertAdj  = mLabelOffset + Point(0, BorderDecorationVertCenteringOffset - KeyFontSize / 2 - 3);   
   const Point textVertAdj = mLabelOffset + Point(0, BorderDecorationVertCenteringOffset - 3);

   if(mHasColor)
      glColor(mColor);

   // Handle some special cases:
   if(mText == "Up Arrow")
      renderUpArrow(center + textVertAdj, KeyFontSize);
   else if(mText == "Down Arrow")
      renderDownArrow(center + textVertAdj, KeyFontSize);
   else if(mText == "Left Arrow")
      renderLeftArrow(center + textVertAdj, KeyFontSize);
   else if(mText == "Right Arrow")
      renderRightArrow(center + textVertAdj, KeyFontSize);
   else
      Parent::render(center + textVertAdj);

   S32 width =  max(mWidth - 2 * Gap, mHeight);

   drawHollowRect(center + boxVertAdj, width, mHeight);
}


////////////////////////////////////////
////////////////////////////////////////


// Symbol to be used when we don't know what symbol to use

// Constructor
SymbolUnknown::SymbolUnknown(const Color *color) : Parent("~?~", &Colors::red)
{
   // Do nothing
}


// Destructor
SymbolUnknown::~SymbolUnknown()
{
   // Do nothing
}


} } // Nested namespace

