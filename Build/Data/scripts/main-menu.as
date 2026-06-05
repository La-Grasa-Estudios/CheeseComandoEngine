#include "sdk/common-ui.as"

void freeplayClicked(UIComponent@ component)
{
    document.hideDocument(documentId);
    document.invokeEvent("switch-freeplay-panel", component);
}

void optionsClicked(UIComponent@ component)
{
    document.hideDocument(documentId);
    document.invokeEvent("switch-options-panel", component);
}

void notImplementedPanel(UIComponent@ component)
{
    document.showDocument("not_implemented_popup");
}

void hoverSound(UIComponent@ component)
{
    Audio::playOneShot("fnf/sounds/scrollMenu.mp3", 0.5f);
}

void setup()
{
    UIComponent@ freeplayButton = document.getElementByName(documentId, "freeplay-button");
    UIComponent@ optionsButton = document.getElementByName(documentId, "options-button");
    UIComponent@ storyButton = document.getElementByName(documentId, "story-button");
    UIComponent@ promoButton = document.getElementByName(documentId, "promo-button");
    UIComponent@ creditsButton = document.getElementByName(documentId, "credits-button");
    array<UIComponent@> buttons = {
        freeplayButton,
        optionsButton,
        storyButton,
        promoButton,
        creditsButton
    };

    freeplayButton.addEventHandler("click", freeplayClicked);
    optionsButton.addEventHandler("click", optionsClicked);
    storyButton.addEventHandler("click", notImplementedPanel);
    promoButton.addEventHandler("click", notImplementedPanel);
    creditsButton.addEventHandler("click", notImplementedPanel);

    for (uint i = 0; i < buttons.length(); ++i)
    {
        buttons[i].addEventHandler("hover", hoverSound);
    }
}


