#include "mainmenu.h"

#include "dialog/media/decode_audio_dialog.h"

MainMenu::MainMenu(QWidget* parent)
    : QMenuBar(parent)
{
    auto tool_menu = addMenu(tr("Tools"));
    tool_menu->addAction(tr("Decode Video"), this, &MainMenu::DecodeAudio);
}

MainMenu::~MainMenu() {}

void MainMenu::DecodeAudio()
{
    DecodeAudioDialog dlg(this);
    dlg.exec();
}
