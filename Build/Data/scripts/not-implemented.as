#include "sdk/common-ui.as"

void close(UIComponent@ component)
{
    document.hideDocument(documentId);
}

void setup()
{
    auto element = document.getElementByName(documentId, "ok-button");
    element.addEventHandler("click", close);
}