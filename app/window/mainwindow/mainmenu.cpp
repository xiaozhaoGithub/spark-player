#include "mainmenu.h"

#include "dialog/media/codec_audio_dialog.h"

MainMenu::MainMenu(QWidget* parent)
    : QMenuBar(parent)
{
    auto tool_menu = addMenu(tr("Tools"));
    tool_menu->addAction(tr("Codec Audio"), this, &MainMenu::CodecAudio);
}

MainMenu::~MainMenu() {}

void MainMenu::CodecAudio()
{
    CodecAudioDialog dlg(this);
    dlg.exec();
}
