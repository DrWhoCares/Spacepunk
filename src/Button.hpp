//! @file Button.hpp

#pragma once

#include "Rect.hpp"
#include "Main.hpp"
#include "Script.hpp"
#include "Font.hpp"
#include "WideVector.hpp"

class Renderer;
class Frame;
class Image;

//! A Button lives in a Frame and can have scripted actions or a native callback.
class Button {
public:
	Button();
	Button(Frame& _parent);
	~Button();

	//! the result of the button process
	struct result_t {
		bool highlighted;				//!< was highlighted this frame
		bool pressed;					//!< was pressed this frame
		bool clicked;					//!< was activated this frame
		Uint32 highlightTime;			//!< time since button was highlighted
		const char* tooltip = nullptr;	//!< button tooltip to be displayed
	};

	//! button style
	enum style_t {
		STYLE_NORMAL,
		STYLE_TOGGLE,
		STYLE_CHECKBOX,
		STYLE_MAX
	};

	//! draws the button
	//! @param renderer the renderer object used to draw the button
	//! @param _size size and position of button's parent frame
	//! @param _actualSize offset into the parent frame space (scroll)
	void draw(Renderer& renderer, Rect<int> _size, Rect<int> _actualSize);

	//! handles button clicks, etc.
	//! @param _size size and position of button's parent frame
	//! @param _actualSize offset into the parent frame space (scroll)
	//! @param usable true if another object doesn't have the mouse's attention, false otherwise
	//! @return resultant state of the button after processing
	result_t process(Rect<int> _size, Rect<int> _actualSize, const bool usable);

	const char*					getName() const { return name.get(); }
	const char*					getText() const { return text.get(); }
	const char*					getFont() const { return font.get(); }
	int							getBorder() const { return border; }
	const Rect<int>&			getSize() const { return size; }
	bool						isPressed() const { return pressed; }
	bool						isHighlighted() const { return highlighted; }
	bool						isDisabled() const { return disabled; }
	int							getStyle() const { return style; }
	Script::Args&				getParams() { return params; }
	const Script::Function*		getCallback() const { return callback; }

	void	setBorder(int _border) { border = _border; }
	void	setPos(int x, int y) { size.x = x; size.y = y; }
	void	setSize(Rect<int>& _size) { size = _size; }
	void	setColor(const WideVector& _color) { color = _color; }
	void	setTextColor(const WideVector& _color) { textColor = _color; }
	void	setName(const char* _name) { name = _name; }
	void	setText(const char* _text) { text = _text; }
	void	setFont(const char* _font) { font = _font; }
	void	setIcon(const char* _icon);
	void	setTooltip(const char* _tooltip) { tooltip = _tooltip; }
	void	setDisabled(bool _disabled) { disabled = _disabled; }
	void	setStyle(int _style) { style = static_cast<style_t>(_style); }
	void	setPressed(bool _pressed) { reallyPressed = pressed = _pressed; }
	void	setCallback(const Script::Function* fn) { callback = fn; }

private:
	Frame* parent = nullptr;						//!< parent frame
	const Script::Function* callback = nullptr;		//!< native callback for clicking
	String name;									//!< internal button name
	String text;									//!< button text, if any
	String font = Font::defaultFont;				//!< button font
	String icon;									//!< icon, if any (supersedes text content)
	String tooltip;									//!< if empty, button has no tooltip; otherwise, it does
	Script::Args params;							//!< optional function parameters to use when the button function is called	
	int border = 3;									//!< size of the button border in pixels
	Rect<int> size;									//!< size and position of the button within its parent frame
	bool pressed = false;							//!< button pressed state
	bool reallyPressed = false;						//!< the "actual" button state, pre-mouse process
	bool highlighted = false;						//!< true if mouse is hovering over button; false otherwise
	WideVector color;								//!< the button's color
	WideVector textColor;							//!< text color
	style_t style = STYLE_NORMAL;					//!< button style
	bool disabled = false;							//!< if true, the button is invisible and unusable
	Uint32 highlightTime = 0;						//!< records the time since the button was highlighted
};