#include <Btk/context.hpp>
#include <Btk/widget.hpp>
#include <Btk/widgets/mainwindow.hpp>
#include <Btk/widgets/combobox.hpp>
#include <Btk/comctl.hpp>
#include <Btk/event.hpp>
#include <Btk/widgets/button.hpp>
#include <iostream>

using namespace BTK_NAMESPACE;

class MenuWindow : public MainWindow {
	public:
		using MainWindow::MainWindow;

		bool fullscreen = false;
	protected:
		bool key_press(KeyEvent &event) {
			if (event.key() == Key::F11) {
				fullscreen = !fullscreen;
				root()->set_fullscreen(fullscreen);
			}
			return true;
		}
};

int main(int argc,char **argv) {
	UIContext context;
	MenuWindow menuwindow;
	Widget widget;

	menuwindow.set_widget(&widget);

	ComboBox box1(&widget);
	ComboBox box2(&widget);

	TextEdit text_edit1(&widget);

	Button add_item_box1(&widget);
	Button add_item_box2(&widget);

	Button delete_item_box1(&widget);
	Button delete_item_box2(&widget);

	box1.add_item("defualt");

	text_edit1.set_rect(0, 30, 600, 30);
	
	add_item_box1.set_text("add");
	delete_item_box1.set_text("delete");
	add_item_box1.set_rect(0, 80, 100, 30);
	delete_item_box1.set_rect(110, 80, 100, 30);
	box1.set_rect(0, 120, 210, 30);

	add_item_box1.signal_clicked().connect([&text_edit1, &box1](){
		if (!text_edit1.text().empty()) {
			box1.add_item(text_edit1.text());
		}
	});
	delete_item_box1.signal_clicked().connect([&text_edit1, &box1](){
		if (!text_edit1.text().empty()) {
			box1.remove_item(text_edit1.text());
		}
	});

	add_item_box2.set_text("add");
	delete_item_box2.set_text("delete");
	add_item_box2.set_rect(300, 80, 100, 30);
	delete_item_box2.set_rect(410, 80, 100, 30);
	box2.set_rect(300, 120, 220, 30);
	add_item_box2.signal_clicked().connect([&text_edit1, &box2](){
		if (!text_edit1.text().empty()) {
			box2.add_item(text_edit1.text());
		}
	});
	delete_item_box2.signal_clicked().connect([&text_edit1, &box2](){
		if (!text_edit1.text().empty()) {
			box2.remove_item(text_edit1.text());
		}
	});

	MenuBar &menu = menuwindow.menubar();
	Menu *file = menu.add_menu("File");
	MenuItem *open_item = new MenuItem("Open");
	open_item->set_image(PixBuffer::FromFile("icon.jpeg"));

	file->add_item(open_item);
	file->add_separator();
	file->add_item("Close")->signal_triggered().connect(&MenuWindow::close, &menuwindow);

	menu.add_item(new MenuItem("First"))->signal_triggered().connect([]() {
		BTK_LOG("First triggered\n");
	});
	menu.add_item(new MenuItem("Next"));
	menu.add_item(new MenuItem("Third"));
	menu.add_item(new MenuItem("SSSS"));

	auto imgitem = new MenuItem("Icon");
	imgitem->set_image(PixBuffer::FromFile("icon.jpeg"));

	auto submenu = new Menu("Sub");
	menu.add_menu(submenu);

	submenu->add_item(new MenuItem("SubItem 1"));
	submenu->add_item(new MenuItem("SubItem 2"));
	submenu->add_item(new MenuItem("SubItem 3"));
	submenu->add_item(new MenuItem("SubItem 4"));

	menu.add_item(imgitem);
	menu.add_separator();
	menu.add_item("Debug")->signal_triggered().connect([&]() {
		menuwindow.set_attribute(WidgetAttrs::Debug, true);
	});

    menuwindow.show();
    menuwindow.resize(640, 480);

	context.run();
	return 0;
}