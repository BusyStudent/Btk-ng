#pragma once

#include <Btk/widget.hpp>
#include <Btk/widgets/menu.hpp>
#include <Btk/widgets/view.hpp>

BTK_NS_BEGIN

/**
 * @brief ComboBox
 * 
 */
class BTKAPI ComboBox : public Widget {
    public:
        ComboBox(Widget *parent = nullptr);
        ~ComboBox();

		/**
		 * @brief max visible items
		 * 
		 * @return int 
		 */
        int max_visible_items() const;
		/**
		 * @brief Set the max visible items for view
		 * 
		 * @param max_items 
		 */
        void set_max_visible_items(int max_items);
		/**
		 * @brief count item in this
		 * 
		 * @return int 
		 */
        int count() const;
		/**
		 * @brief find the text index in this
		 * 
		 * @param text 
		 * @return int 
		 */
        int find_text(u8string_view text);
		/**
		 * @brief return current selected index
		 * 
		 * @return int 
		 */
        int current_index() const;
		/**
		 * @brief return current selected text
		 * 
		 * @return u8string 
		 */
        u8string current_text() const;
		/**
		 * @brief Set the item text object
		 * 
		 * @param index 
		 * @param text 
		 * @return true (set success)
		 * @return false (set falied,maybe index invalid)
		 */
        bool set_item_text(int index, u8string_view text);
		bool set_item_icon(int index, const PixBuffer& icon);
		/**
		 * @brief add item
		 * 
		 * @param str 
		 */
        void add_item(u8string_view str);
		void add_item(const PixBuffer& icon);
		void add_item(u8string_view str, const PixBuffer& icon);
		/**
		 * @brief remove item
		 * 
		 * @param what 
		 */
        void remove_item(u8string_view what);

    protected: // Event handlers
        bool paint_event(PaintEvent &event) override;
        
		bool mouse_press  (MouseEvent &event) override;
        bool mouse_release(MouseEvent &event) override;
        bool mouse_enter  (MotionEvent &event) override;
        bool mouse_leave  (MotionEvent &event) override;
        bool mouse_motion (MotionEvent &event) override;
		bool mouse_wheel  (WheelEvent &event) override;
        
		bool change_event (ChangeEvent &event) override;
		
		bool focus_gained (FocusEvent &event) override;
		bool focus_lost   (FocusEvent &event) override; 

	public:
        Size size_hint() const override;

    public: // SLOTS
		/**
		 * @brief Set the current index
		 * 
		 * @param index 
		 */
        void set_current_index(int index);
		/**
		 * @brief Set the current text
		 * 
		 * @param text 
		 */
        void set_current_text(u8string_view text);
		/**
		 * @brief Set the current item
		 * 
		 * @param item 
		 */
		void set_current_item(ListItem* item);


    public: // SIGNALS
		/**
		 * @brief emit while text be changed
		 * @param index
		 * @param text
		 */
        BTK_EXPOSE_SIGNAL(_text_changed);
		/**
		 * @brief emit while current index(text) changed
		 * @param text
		 */
        BTK_EXPOSE_SIGNAL(_current_text_changed);
		/**
		 * @brief emit while current index(text) changed
		 * @param index
		 */
        BTK_EXPOSE_SIGNAL(_current_index_changed);

    protected:
        Signal<void(int, u8string_view)> _text_changed;
        Signal<void(u8string_view)> _current_text_changed;
        Signal<void(int)> _current_index_changed;

    private:
        int _current_index = 0;
        int _max_visible_items = 5;
		int _count = 0;
		float blank;
        TextLayout _text_layout;
		Alignment  _align = Alignment::Left | Alignment::Middle;
        ListBox *_listbox_view = nullptr;
		PopupWidget *_popup_widget = nullptr;
};

BTK_NS_END