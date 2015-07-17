#include <QDebug>
#include <QFile>
#include <QFileInfo>

#include "preview.h"

static Preview *__instance = nullptr;

Preview* Preview::getInstance()
{
    if (!__instance)
        __instance = new Preview();

    return __instance;
}

Preview::Preview()
{
    xlink = new QLibrary("xlink");
    xlink->load();

    if(xlink->isLoaded()) {
        xlink_ping = (xlink_ping_t) xlink->resolve("xlink_ping");
        xlink_load = (xlink_load_t) xlink->resolve("xlink_load");
        xlink_poke = (xlink_poke_t) xlink->resolve("xlink_poke");
        xlink_peek = (xlink_peek_t) xlink->resolve("xlink_peek");
        xlink_fill = (xlink_fill_t) xlink->resolve("xlink_fill");
    }
}

void Preview::updateBackgroundColor() {
    auto state = State::getInstance();
    xlink_poke(0x37, 0x00, 0xd020, (uchar) state->getColorAtIndex(0));
    xlink_poke(0x37, 0x00, 0xd021, (uchar) state->getColorAtIndex(0));
}

void Preview::updateForegroundColor() {
    auto state = State::getInstance();
    uchar foreground = state->getColorAtIndex(3);
    foreground |= state->isMultiColor() ? 8 : 0;

    xlink_fill(0xb7, 0x00, 0xd800, foreground, 1000);
    xlink_poke(0x37, 0x00, 0x0286, foreground);
}

void Preview::updateMulticolor1() {
    auto state = State::getInstance();
    xlink_poke(0x37, 0x00, 0xd022, (uchar) state->getColorAtIndex(1));
}

void Preview::updateMulticolor2() {
    auto state = State::getInstance();
    xlink_poke(0x37, 0x00, 0xd023, (uchar) state->getColorAtIndex(2));
}

void Preview::updateColorMode() {

    auto state = State::getInstance();
    uchar control = 0x08;

    xlink_peek(0x37, 0x00, 0xd016, &control);
    xlink_poke(0x37, 0x00, 0xd016, state->isMultiColor() ? 0x18 : 0x08);

    updateForegroundColor();
}

void Preview::updateColorProperties()
{
    updateBackgroundColor();
    updateMulticolor1();
    updateMulticolor2();
    updateColorMode(); // also updates foreground color
}

void Preview::updateCharset() {
    auto state = State::getInstance();

    xlink_load(0xb7, 0x00, 0x3000, (uchar*) state->getChars(), State::CHAR_BUFFER_SIZE);
    xlink_poke(0x37, 0x00, 0xd018, 0x1c);
}

bool Preview::updateScreen(const QString& filename) {

    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly))
        return false;

    char *screen = (char*) calloc(1000, sizeof(char));
    if(screen == NULL) return false;

    int size = file.read(screen, 1000);
    file.close();

    xlink_load(0x37, 0x00, 0x0400, (uchar*) screen, size);

    free(screen);
    return true;
}

void Preview::fileLoaded()
{
    if(!xlink->isLoaded()) return;
    if(!xlink_ping()) return;

    updateScreen(":/c64-screen.bin");
    updateCharset();
    updateColorProperties();
}

void Preview::tileUpdated(int tileIndex)
{
    if(!xlink->isLoaded()) return;
    if(!xlink_ping()) return;

    auto state = State::getInstance();

    State::TileProperties properties = state->getTileProperties();

    int charIndex = state->getCharIndexFromTileIndex(tileIndex);
    int numChars = properties.size.width() * properties.size.height();

    if(properties.interleaved == 1) {
        xlink_load(0xb7, 0x00, 0x3000 + charIndex * 8, (uchar*) state->getCharAtIndex(charIndex), numChars*8);
    }
    else {
        for(int sent=0; sent<numChars; sent++) {
            xlink_load(0xb7, 0x00, 0x3000 + charIndex * 8, (uchar*) state->getCharAtIndex(charIndex), 8);
            charIndex += properties.interleaved;
        }
    }
}

void Preview::colorSelected() {

    if(!xlink->isLoaded()) return;
    if(!xlink_ping()) return;

    auto state = State::getInstance();

    switch(state->getSelectedColorIndex()) {
    case 0: updateBackgroundColor(); break;
    case 1: updateMulticolor1(); break;
    case 2: updateMulticolor2(); break;
    case 3: updateForegroundColor(); break;
    }
}

void Preview::colorPropertiesUpdated() { updateColorMode(); }

