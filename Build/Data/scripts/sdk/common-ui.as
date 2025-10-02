UIManager@ document;
string documentId; // This script's parent document

void setupScript(const string& in name, UIManager@ manager)
{
    documentId = name;
    @document = @manager;
}