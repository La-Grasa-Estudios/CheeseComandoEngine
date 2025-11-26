#include "sdk/common-ui.as"

void show()
{
    UIComponent@ vsyncBox = document.getElementByName(documentId, "vsync-box");
    vsyncBox.boolValue = Funkin::getSettingsValue("vsync", true);
}

void hide()
{
    UIComponent@ vsyncBox = document.getElementByName(documentId, "vsync-box");
    Funkin::setSettingsValue("vsync", vsyncBox.boolValue);
}

void onCheckboxClicked(UIComponent@ comp)
{
    if (comp.userData == "vsync")
    {
        setVsync(comp.boolValue);
    }
    Funkin::setSettingsValue(comp.userData, comp.boolValue);
}

void setup()
{
    UIComponent@ downscrollBox = document.getElementByName(documentId, "downscroll-box");
    UIComponent@ vsyncBox = document.getElementByName(documentId, "vsync-box");
    downscrollBox.userData = "downscroll";
    vsyncBox.userData = "vsync";
    downscrollBox.addEventHandler("click", onCheckboxClicked);
    vsyncBox.addEventHandler("click", onCheckboxClicked);

    bool valueDownscroll = Funkin::getSettingsValue("downscroll", false);
    bool valueVsync = Funkin::getSettingsValue("vsync", true);
    downscrollBox.boolValue = valueDownscroll;
    vsyncBox.boolValue = valueVsync;
    setVsync(valueVsync);
}