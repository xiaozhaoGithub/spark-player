#include "mainmenu.h"

#include "dialog/media/codec_audio_dialog.h"
#include "dialog/media/codec_video_dialog.h"

MainMenu::MainMenu(QWidget* parent)
    : QMenuBar(parent)
{
    auto tool_menu = addMenu(tr("Tools"));
    tool_menu->addAction(tr("Codec Audio"), this, &MainMenu::CodecAudio);
    tool_menu->addAction(tr("Codec Video"), this, &MainMenu::CodecVideo);
}

MainMenu::~MainMenu() {}

void MainMenu::CodecAudio()
{
    CodecAudioDialog dlg(this);
    dlg.exec();
}

void MainMenu::CodecVideo()
{
    CodecVideoDialog dlg(this);
    dlg.exec();
}
