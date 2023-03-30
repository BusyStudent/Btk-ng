#include <Btk/context.hpp>
#include <Btk/widget.hpp>
#include <Btk/widgets/combobox.hpp>
#include <Btk/comctl.hpp>
#include <Btk/event.hpp>
#include <iostream>

using namespace BTK_NAMESPACE;

int main(int argc,char **argv) {
	UIContext context;
	Widget widget;
	ComboBox box(&widget);

	box.add_item("defualt");
	box.add_item("list item 1");
	box.add_item("list item 2");
	box.add_item("list item 3");
	box.add_item("list item 4");
	box.add_item("list item 5");
	box.add_item("list item 6");
	box.add_item("list item 7");
	box.add_item("list item 8");
	box.add_item("list item 9");
	box.add_item("list item 10");
	box.add_item("list item 11");
	box.add_item("list item 12");
	box.add_item("list item 13");

    widget.show();
    widget.resize(640, 480);

	box.resize(box.size_hint());

	context.run();
	return 0;
}