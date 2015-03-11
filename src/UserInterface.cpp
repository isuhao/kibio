// =============================================================================
//
// Copyright (c) 2015 Brannon Dorsey <http://brannondorsey.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// =============================================================================


#include "UserInterface.h"

namespace Kibio {
    
// ImageButton class
// =============================================================================

ImageButton::ImageButton(const std::string& imagePath,
                         UIButtonType _type,
                         bool sticky,
                         ofEvent<const UserInterfaceEvent>& buttonSelectEvent,
                         ofEvent<const UserInterfaceEvent>& buttonDeselectEvent,
                         ofColor color,
                         ofColor highlightColor,
                         ofColor shadowColor):
type(_type),
_bSticky(sticky),
_buttonSelectEvent(buttonSelectEvent),
_buttonDeselectEvent(buttonDeselectEvent),
_color(color),
_highlightColor(highlightColor),
_shadowColor(shadowColor),
_bSelected(false),
_bHovered(false)
{

    ofAddListener(ofEvents().mouseReleased, this, &ImageButton::mouseReleased);
    ofLoadImage(_texture, imagePath);
}
    
ImageButton::~ImageButton()
{
    ofRemoveListener(ofEvents().mouseReleased, this, &ImageButton::mouseReleased);
}
    
void ImageButton::set(int x, int y, int width, int height)
{
    _rect.set(x, y, width, height);
}
    
void ImageButton::update(const ofPoint& mouse)
{
    if (_rect.inside(mouse))
    {
        _bHovered = true;
    }
    else
    {
        _bHovered = false;
    }
}

void ImageButton::draw(const ofPoint& shadowOffset)
{
    ofPushStyle();
    
    // draw shadow
    if (shadowOffset != ofPoint::zero())
    {
        ofSetColor(_shadowColor);
        _texture.draw(_rect.x + shadowOffset.x,
                      _rect.y + shadowOffset.y,
                      _rect.width,
                      _rect.height);
    }
    
    if (isHovered() || isSelected())
    {
        ofSetColor(_highlightColor);
    }
    else
    {
        ofSetColor(_color);
    }
    
    _texture.draw(_rect.x, _rect.y, _rect.width, _rect.height);
    
    ofPopStyle();
}
    
void ImageButton::mouseReleased(ofMouseEventArgs& args)
{
    if (_rect.inside(args.x, args.y))
    {
        if (_bSticky)
        {
            setSelected(!isSelected());
        }
        else
        {
            select(); // select to notify event
            _bSelected = false; // deselect because this button is not sticky
        }
    }
}
    
void ImageButton::select()
{
    _bSelected = true;
    const UserInterfaceEvent args(type);
    ofNotifyEvent(_buttonSelectEvent, args, this);
}
  
void ImageButton::deselect()
{
    _bSelected = false;
    const UserInterfaceEvent args(type);
    ofNotifyEvent(_buttonDeselectEvent, args, this);
}
    
void ImageButton::setSelected(bool b)
{
    if(b)
    {
        select();
    }
    else
    {
        deselect();
    }
}
    
bool ImageButton::isSelected() const
{
    return _bSelected;
}

bool ImageButton::isHovered() const
{
    return _bHovered;
}
    
// UserInterface class
// =============================================================================
    
UserInterface::UserInterface():
_color(ofColor(255, 255, 255)),
_highlightColor(ofColor(255, 255, 0)),
_shadowColor(ofColor(30, 120, 165)),
_bDrawIconShadows(true),
_iconPadding(10),
_iconSize(30),
_fontSize(18),
_shadowOffset(ofVec2f(1, 1)),
_projectName(""),
_openProjectButton(ImageButton("images/archive.png",
                               BUTTON_OPEN_PROJECT,
                               false,
                               buttonSelectEvent,
                               buttonDeselectEvent,
                               _color,
                               _highlightColor,
                               _shadowColor)),
_newProjectButton(ImageButton("images/plus.png",
                              BUTTON_NEW_PROJECT,
                              false,
                              buttonSelectEvent,
                              buttonDeselectEvent,
                              _color,
                              _highlightColor,
                              _shadowColor)),
_saveProjectButton(ImageButton("images/save.png",
                               BUTTON_SAVE_PROJECT,
                               false,
                               buttonSelectEvent,
                               buttonDeselectEvent,
                               _color,
                               _highlightColor,
                               _shadowColor)),
_infoButton(ImageButton("images/info.png",
                        BUTTON_INFO,
                        true,
                        buttonSelectEvent,
                        buttonDeselectEvent,
                        _color,
                        _highlightColor,
                        _shadowColor)),
_toggleModeButton(ImageButton("images/light-down.png",
                              BUTTON_TOGGLE_MODE,
                              false,
                              buttonSelectEvent,
                              buttonDeselectEvent,
                              _color,
                              _highlightColor,
                              _shadowColor)),
_toolBrushButton(ImageButton("images/brush.png",
                             BUTTON_TOOL_BRUSH,
                             true,
                             buttonSelectEvent,
                             buttonDeselectEvent,
                             _color,
                             _highlightColor,
                             _shadowColor)),
_toolTranslateButton(ImageButton("images/hand.png",
                                 BUTTON_TOOL_TRANSLATE,
                                 true,
                                 buttonSelectEvent,
                                 buttonDeselectEvent,
                                 _color,
                                 _highlightColor,
                                 _shadowColor)),
_toolRotateButton(ImageButton("images/cycle.png",
                              BUTTON_TOOL_ROTATE,
                              true,
                              buttonSelectEvent,
                              buttonDeselectEvent,
                              _color,
                              _highlightColor,
                              _shadowColor)),
_toolScaleButton(ImageButton("images/resize-full-screen.png",
                             BUTTON_TOOL_SCALE,
                             true,
                             buttonSelectEvent,
                             buttonDeselectEvent,
                             _color,
                             _highlightColor,
                             _shadowColor))
{
    _font.load("media/Verdana.ttf", _fontSize);
    
    ofAddListener(buttonSelectEvent, this,&UserInterface::onButtonSelect);
    ofAddListener(buttonDeselectEvent, this,&UserInterface::onButtonDeselect);
    
    placeIcons();
    show();
}
    
UserInterface::~UserInterface()
{
    ofRemoveListener(buttonSelectEvent, this,&UserInterface::onButtonSelect);
    ofRemoveListener(buttonDeselectEvent, this,&UserInterface::onButtonDeselect);
}
    
void UserInterface::update()
{
    ofPoint mouse(ofGetMouseX(), ofGetMouseY());
    
    _openProjectButton.update(mouse);
    _newProjectButton.update(mouse);
    _saveProjectButton.update(mouse);
    _infoButton.update(mouse);
    _toggleModeButton.update(mouse);
    _toolBrushButton.update(mouse);
    _toolTranslateButton.update(mouse);
    _toolRotateButton.update(mouse);
    _toolScaleButton.update(mouse);
}
    
void UserInterface::draw()
{

    ofPushStyle();
    ofSetColor(_color);
    
    if (_font.isLoaded() && !_projectName.empty())
    {
        _font.drawString(_projectName, _iconPadding, _fontSize + _iconPadding);
    }
    
    _openProjectButton.draw(_shadowOffset);
    _newProjectButton.draw(_shadowOffset);
    _saveProjectButton.draw(_shadowOffset);
    _infoButton.draw(_shadowOffset);
    _toggleModeButton.draw(_shadowOffset);
    _toolBrushButton.draw(_shadowOffset);
    _toolTranslateButton.draw(_shadowOffset);
    _toolRotateButton.draw(_shadowOffset);
    _toolScaleButton.draw(_shadowOffset);
    
    ofPopStyle();
}

void UserInterface::placeIcons()
{
    
    int w = ofGetWidth();
    int h = ofGetHeight();
    
 
    int x = w - _iconPadding - _iconSize;
    int y = _iconPadding;
    
    // top right
    _toolScaleButton.set(x, y, _iconSize, _iconSize);
    x -= _iconSize + _iconPadding;
    
    _toolRotateButton.set(x, y, _iconSize, _iconSize);
    x -= _iconSize + _iconPadding;
    
    _toolTranslateButton.set(x, y, _iconSize, _iconSize);
    x -= _iconSize + _iconPadding * 2; // place brush further to left
    
    _toolBrushButton.set(x, y, _iconSize, _iconSize);

    // bottom left
    x = w - _iconPadding - _iconSize;
    y = h - _iconPadding - _iconSize;
    
    _toggleModeButton.set(x, y, _iconSize, _iconSize);
    x -= _iconSize + _iconPadding;
    
    _infoButton.set(x, y, _iconSize, _iconSize);
    
    // bottom left
    x = _iconSize * 3;
    
    _newProjectButton.set(x, y, _iconSize, _iconSize);
    x -= _iconSize + _iconPadding;
    
    _openProjectButton.set(x, y, _iconSize, _iconSize);
    x -= _iconSize + _iconPadding;
    
    _saveProjectButton.set(x, y, _iconSize, _iconSize);
    
}

void UserInterface::toggleVisible()
{
    _bVisible = !_bVisible;
}

void UserInterface::hide()
{
    _bVisible = false;
}

void UserInterface::show()
{
    _bVisible = true;
}

void UserInterface::setProjectName(const std::string& name)
{
    _projectName = name;
}

bool UserInterface::isVisible() const
{
    return _bVisible;
}
    
void UserInterface::setUIButtonSelectState(const UIButtonType& type, bool state)
{
    ImageButton& button = _getButton(type);
    button.setSelected(state);
}

void UserInterface::toggleUIButtonState(const UIButtonType& type)
{
    ImageButton& button = _getButton(type);
    button.setSelected(!button.isSelected());
}

bool UserInterface::getUIButtonSelectState(const UIButtonType& type)
{
    ImageButton& button = _getButton(type);
    return button.isSelected();
}
    
void UserInterface::onButtonSelect(const UserInterfaceEvent& args)
{
    // if button is mutually exclusive
    // deselect others
    if (args.type == BUTTON_TOOL_BRUSH ||
        args.type == BUTTON_TOOL_TRANSLATE ||
        args.type == BUTTON_TOOL_ROTATE ||
        args.type == BUTTON_TOOL_SCALE) {
        
        std::vector<ImageButton*> buttons = _getSelectedButtons();
        if (buttons.size() > 0)
        {
            for (std::size_t i = 0; i < buttons.size(); i++)
            {
                ImageButton* button = buttons[i];
                if (button->type != args.type &&
                    (args.type == BUTTON_TOOL_BRUSH ||
                     args.type == BUTTON_TOOL_TRANSLATE ||
                     args.type == BUTTON_TOOL_ROTATE ||
                     args.type == BUTTON_TOOL_SCALE))
                {
                    button->deselect();
                }
            }
        }
    }
}

void UserInterface::onButtonDeselect(const UserInterfaceEvent& args)
{
    
}
    
ImageButton&  UserInterface::_getButton(UIButtonType type)
{

    if (type == BUTTON_OPEN_PROJECT)
    {
        return _openProjectButton;
    }
    else if (BUTTON_NEW_PROJECT)
    {
        return _newProjectButton;
    }
    else if (BUTTON_SAVE_PROJECT)
    {
        return _saveProjectButton;
    }
    else if (BUTTON_INFO)
    {
        return _infoButton;
    }
    else if (BUTTON_TOGGLE_MODE)
    {
        return _toggleModeButton;
    }
    else if (BUTTON_TOOL_BRUSH)
    {
        return _toolBrushButton;
    }
    else if (BUTTON_TOOL_TRANSLATE)
    {
        return _toolTranslateButton;
    }
    else if (BUTTON_TOOL_ROTATE)
    {
        return _toolRotateButton;
    }
    else if (BUTTON_TOOL_SCALE)
    {
        return _toolScaleButton;
    }
}
    
std::vector<ImageButton*> UserInterface::_getSelectedButtons()
{
    std::vector<ImageButton*> buttons;
    
    if (_openProjectButton.isSelected())
    {
        buttons.push_back(&_openProjectButton);
    }
    
    if (_newProjectButton.isSelected())
    {
        buttons.push_back(&_newProjectButton);
    }
    
    if (_saveProjectButton.isSelected())
    {
        buttons.push_back(&_saveProjectButton);
    }
    
    if (_infoButton.isSelected())
    {
        buttons.push_back(&_infoButton);
    }
    
    if (_toggleModeButton.isSelected())
    {
        buttons.push_back(&_toggleModeButton);
    }
    
    if (_toolBrushButton.isSelected())
    {
        buttons.push_back(&_toolBrushButton);
    }
    
    if (_toolTranslateButton.isSelected())
    {
        buttons.push_back(&_toolTranslateButton);
    }
    
    if (_toolRotateButton.isSelected())
    {
        buttons.push_back(&_toolRotateButton);
    }
    
    if (_toolScaleButton.isSelected())
    {
        buttons.push_back(&_toolScaleButton);
    }
    
    return buttons;
}

} // namespace Kibio