#include <Btk/plugins/builder.hpp>
#include <Btk/context.hpp>
#include <Btk/widget.hpp>
#include <Btk/string.hpp>
#include <iostream>

using namespace BTK_NAMESPACE;

auto xmldoc = R"(
    <!-- Root Widget -->
    <BoxLayout direction="left, right" background-transparent="true">
        <BoxLayout direction="top, bottom" margin="10" spacing="2">
            <Button> Hello Button </Button>
            <TextEdit>Hello TextEdit </TextEdit>
            <Button text="HelloWorld" flat="true"></Button>
            <Button text="HelloWorld" ></Button>
            <TextEdit text="HelloWorld" ></TextEdit>

            <Widget layout:stretch="10"></Widget>
        </BoxLayout>


        <BoxLayout layout:stretch="10">
            <ImageView opacity="0.5"></ImageView>
        </BoxLayout>

    </BoxLayout>

)";

int main() {
    UIContext ctxt;
    UIXmlBuilder builder;
    builder.load(xmldoc);

    std::unique_ptr<Widget> result(builder.build());
    if (!result) {
        std::cout << "Failed to build" << std::endl;
        return EXIT_FAILURE;
    }
    result->show();

    ctxt.run();
}