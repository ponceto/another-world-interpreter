/*
 * data.cc - Copyright (c) 2004-2025
 *
 * Gregory Montoir, Fabien Sanglard, Olivier Poncet
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdarg>
#include <cstdint>
#include <climits>
#include <cassert>
#include <ctime>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <utility>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include "logger.h"
#include "intern.h"
#include "bytekiller.h"
#include "file.h"
#include "data.h"

// ---------------------------------------------------------------------------
// some useful macros
// ---------------------------------------------------------------------------

#ifndef NDEBUG
#define LOG_DEBUG(format, ...) log_debug(SYS_RESOURCES, format, ##__VA_ARGS__)
#else
#define LOG_DEBUG(format, ...) do {} while(0)
#endif

// ---------------------------------------------------------------------------
// Dictionary
// ---------------------------------------------------------------------------

const String Dictionary::dataEN[] = {
    { 0x0001, "P E A N U T  3000" },
    { 0x0002, "Copyright  } 1990 Peanut Computer, Inc.\nAll rights reserved.\n\nCDOS Version 5.01" },
    { 0x0003, "2" },
    { 0x0004, "3" },
    { 0x0005, "." },
    { 0x0006, "A" },
    { 0x0007, "@" },
    { 0x0008, "PEANUT 3000" },
    { 0x000a, "R" },
    { 0x000b, "U" },
    { 0x000c, "N" },
    { 0x000d, "P" },
    { 0x000e, "R" },
    { 0x000f, "O" },
    { 0x0010, "J" },
    { 0x0011, "E" },
    { 0x0012, "C" },
    { 0x0013, "T" },
    { 0x0014, "Shield 9A.5f Ok" },
    { 0x0015, "Flux % 5.0177 Ok" },
    { 0x0016, "CDI Vector ok" },
    { 0x0017, " %%%ddd ok" },
    { 0x0018, "Race-Track ok" },
    { 0x0019, "SYNCHROTRON" },
    { 0x001a, "E: 23%\ng: .005\n\nRK: 77.2L\n\nopt: g+\n\n Shield:\n1: OFF\n2: ON\n3: ON\n\nP~: 1\n" },
    { 0x001b, "ON" },
    { 0x001c, "-" },
    { 0x0021, "|" },
    { 0x0022, "--- Theoretical study ---" },
    { 0x0023, " THE EXPERIMENT WILL BEGIN IN    SECONDS" },
    { 0x0024, "  20" },
    { 0x0025, "  19" },
    { 0x0026, "  18" },
    { 0x0027, "  4" },
    { 0x0028, "  3" },
    { 0x0029, "  2" },
    { 0x002a, "  1" },
    { 0x002b, "  0" },
    { 0x002c, "L E T ' S   G O" },
    { 0x0031, "- Phase 0:\nINJECTION of particles\ninto synchrotron" },
    { 0x0032, "- Phase 1:\nParticle ACCELERATION." },
    { 0x0033, "- Phase 2:\nEJECTION of particles\non the shield." },
    { 0x0034, "A  N  A  L  Y  S  I  S" },
    { 0x0035, "- RESULT:\nProbability of creating:\n ANTIMATTER: 91.V %\n NEUTRINO 27:  0.04 %\n NEUTRINO 424: 18 %\n" },
    { 0x0036, "   Practical verification Y/N ?" },
    { 0x0037, "SURE ?" },
    { 0x0038, "MODIFICATION OF PARAMETERS\nRELATING TO PARTICLE\nACCELERATOR (SYNCHROTRON)." },
    { 0x0039, "       RUN EXPERIMENT ?" },
    { 0x003c, "t---t" },
    { 0x003d, "000 ~" },
    { 0x003e, ".20x14dd" },
    { 0x003f, "gj5r5r" },
    { 0x0040, "tilgor 25%" },
    { 0x0041, "12% 33% checked" },
    { 0x0042, "D=4.2158005584" },
    { 0x0043, "d=10.00001" },
    { 0x0044, "+" },
    { 0x0045, "*" },
    { 0x0046, "% 304" },
    { 0x0047, "gurgle 21" },
    { 0x0048, "{{{{" },
    { 0x0049, "Delphine Software" },
    { 0x004a, "By Eric Chahi" },
    { 0x004b, "  5" },
    { 0x004c, "  17" },
    { 0x012c, "0" },
    { 0x012d, "1" },
    { 0x012e, "2" },
    { 0x012f, "3" },
    { 0x0130, "4" },
    { 0x0131, "5" },
    { 0x0132, "6" },
    { 0x0133, "7" },
    { 0x0134, "8" },
    { 0x0135, "9" },
    { 0x0136, "A" },
    { 0x0137, "B" },
    { 0x0138, "C" },
    { 0x0139, "D" },
    { 0x013a, "E" },
    { 0x013b, "F" },
    { 0x013c, "        ACCESS CODE:" },
    { 0x013d, "PRESS BUTTON OR RETURN TO CONTINUE" },
    { 0x013e, "   ENTER ACCESS CODE" },
    { 0x013f, "   INVALID PASSWORD !" },
    { 0x0140, "ANNULER" },
    { 0x0141, "      INSERT DISK ?\n\n\n\n\n\n\n\n\nPRESS ANY KEY TO CONTINUE" },
    { 0x0142, " SELECT SYMBOLS CORRESPONDING TO\n THE POSITION\n ON THE CODE WHEEL" },
    { 0x0143, "    LOADING..." },
    { 0x0144, "              ERROR" },
    { 0x015e, "LDKD" },
    { 0x015f, "HTDC" },
    { 0x0160, "CLLD" },
    { 0x0161, "FXLC" },
    { 0x0162, "KRFK" },
    { 0x0163, "XDDJ" },
    { 0x0164, "LBKG" },
    { 0x0165, "KLFB" },
    { 0x0166, "TTCT" },
    { 0x0167, "DDRX" },
    { 0x0168, "TBHK" },
    { 0x0169, "BRTD" },
    { 0x016a, "CKJL" },
    { 0x016b, "LFCK" },
    { 0x016c, "BFLX" },
    { 0x016d, "XJRT" },
    { 0x016e, "HRTB" },
    { 0x016f, "HBHK" },
    { 0x0170, "JCGB" },
    { 0x0171, "HHFL" },
    { 0x0172, "TFBB" },
    { 0x0173, "TXHF" },
    { 0x0174, "JHJL" },
    { 0x0181, " BY" },
    { 0x0182, "ERIC CHAHI" },
    { 0x0183, "         MUSIC AND SOUND EFFECTS" },
    { 0x0184, " " },
    { 0x0185, "JEAN-FRANCOIS FREITAS" },
    { 0x0186, "IBM PC VERSION" },
    { 0x0187, "      BY" },
    { 0x0188, " DANIEL MORAIS" },
    { 0x018b, "       THEN PRESS FIRE" },
    { 0x018c, " PUT THE PADDLE ON THE UPPER LEFT CORNER" },
    { 0x018d, "PUT THE PADDLE IN CENTRAL POSITION" },
    { 0x018e, "PUT THE PADDLE ON THE LOWER RIGHT CORNER" },
    { 0x0258, "      Designed by ..... Eric Chahi" },
    { 0x0259, "    Programmed by...... Eric Chahi" },
    { 0x025a, "      Artwork ......... Eric Chahi" },
    { 0x025b, "Music by ........ Jean-francois Freitas" },
    { 0x025c, "            Sound effects" },
    { 0x025d, "        Jean-Francois Freitas\n             Eric Chahi" },
    { 0x0263, "              Thanks To" },
    { 0x0264, "           Jesus Martinez\n\n          Daniel Morais\n\n        Frederic Savoir\n\n      Cecile Chahi\n\n    Philippe Delamarre\n\n  Philippe Ulrich\n\nSebastien Berthet\n\nPierre Gousseau" },
    { 0x0265, "Now Go Out Of This World" },
    { 0x0190, "Good evening professor." },
    { 0x0191, "I see you have driven here in your\nFerrari." },
    { 0x0192, "IDENTIFICATION" },
    { 0x0193, "Monsieur est en parfaite sante." },
    { 0x0194, "Y\n" },
    { 0x0193, "AU BOULOT !!!\n" },
    { 0xffff, "" }
};

const String Dictionary::dataFR[] = {
    { 0x0001, "P E A N U T  3000" },
    { 0x0002, "Copyright  } 1990 Peanut Computer, Inc.\nAll rights reserved.\n\nCDOS Version 5.01" },
    { 0x0003, "2" },
    { 0x0004, "3" },
    { 0x0005, "." },
    { 0x0006, "A" },
    { 0x0007, "@" },
    { 0x0008, "PEANUT 3000" },
    { 0x000a, "R" },
    { 0x000b, "U" },
    { 0x000c, "N" },
    { 0x000d, "P" },
    { 0x000e, "R" },
    { 0x000f, "O" },
    { 0x0010, "J" },
    { 0x0011, "E" },
    { 0x0012, "C" },
    { 0x0013, "T" },
    { 0x0014, "Shield 9A.5f Ok" },
    { 0x0015, "Flux % 5.0177 Ok" },
    { 0x0016, "CDI Vector ok" },
    { 0x0017, " %%%ddd ok" },
    { 0x0018, "Race-Track ok" },
    { 0x0019, "SYNCHROTRON" },
    { 0x001a, "E: 23%\ng: .005\n\nRK: 77.2L\n\nopt: g+\n\n Shield:\n1: OFF\n2: ON\n3: ON\n\nP~: 1\n" },
    { 0x001b, "ON" },
    { 0x001c, "-" },
    { 0x0021, "|" },
    { 0x0022, "--- Etude theorique ---" },
    { 0x0023, " L'EXPERIENCE DEBUTERA DANS    SECONDES." },
    { 0x0024, "20" },
    { 0x0025, "19" },
    { 0x0026, "18" },
    { 0x0027, "4" },
    { 0x0028, "3" },
    { 0x0029, "2" },
    { 0x002a, "1" },
    { 0x002b, "0" },
    { 0x002c, "L E T ' S   G O" },
    { 0x0031, "- Phase 0:\nINJECTION des particules\ndans le synchrotron" },
    { 0x0032, "- Phase 1:\nACCELERATION des particules." },
    { 0x0033, "- Phase 2:\nEJECTION des particules\nsur le bouclier." },
    { 0x0034, "A  N  A  L  Y  S  E" },
    { 0x0035, "- RESULTAT:\nProbabilites de creer de:\n ANTI-MATIERE: 91.V %\n NEUTRINO 27:  0.04 %\n NEUTRINO 424: 18 %\n" },
    { 0x0036, "Verification par la pratique O/N ?" },
    { 0x0037, "SUR ?" },
    { 0x0038, "MODIFICATION DES PARAMETRES\nRELATIFS A L'ACCELERATEUR\nDE PARTICULES (SYNCHROTRON)." },
    { 0x0039, "SIMULATION DE L'EXPERIENCE ?" },
    { 0x003c, "t---t" },
    { 0x003d, "000 ~" },
    { 0x003e, ".20x14dd" },
    { 0x003f, "gj5r5r" },
    { 0x0040, "tilgor 25%" },
    { 0x0041, "12% 33% checked" },
    { 0x0042, "D=4.2158005584" },
    { 0x0043, "d=10.00001" },
    { 0x0044, "+" },
    { 0x0045, "*" },
    { 0x0046, "% 304" },
    { 0x0047, "gurgle 21" },
    { 0x0048, "{{{{" },
    { 0x0049, "Delphine Software" },
    { 0x004a, "By Eric Chahi" },
    { 0x004b, "5" },
    { 0x004c, "17" },
    { 0x012c, "0" },
    { 0x012d, "1" },
    { 0x012e, "2" },
    { 0x012f, "3" },
    { 0x0130, "4" },
    { 0x0131, "5" },
    { 0x0132, "6" },
    { 0x0133, "7" },
    { 0x0134, "8" },
    { 0x0135, "9" },
    { 0x0136, "A" },
    { 0x0137, "B" },
    { 0x0138, "C" },
    { 0x0139, "D" },
    { 0x013a, "E" },
    { 0x013b, "F" },
    { 0x013c, "       CODE D'ACCES:" },
    { 0x013d, "PRESSEZ LE BOUTON POUR CONTINUER" },
    { 0x013e, "   ENTRER LE CODE D'ACCES" },
    { 0x013f, "MOT DE PASSE INVALIDE !" },
    { 0x0140, "ANNULER" },
    { 0x0141, "     INSEREZ LA DISQUETTE ?\n\n\n\n\n\n\n\n\nPRESSEZ UNE TOUCHE POUR CONTINUER" },
    { 0x0142, "SELECTIONNER LES SYMBOLES CORRESPONDANTS\nA LA POSITION\nDE LA ROUE DE PROTECTION" },
    { 0x0143, "CHARGEMENT..." },
    { 0x0144, "             ERREUR" },
    { 0x015e, "LDKD" },
    { 0x015f, "HTDC" },
    { 0x0160, "CLLD" },
    { 0x0161, "FXLC" },
    { 0x0162, "KRFK" },
    { 0x0163, "XDDJ" },
    { 0x0164, "LBKG" },
    { 0x0165, "KLFB" },
    { 0x0166, "TTCT" },
    { 0x0167, "DDRX" },
    { 0x0168, "TBHK" },
    { 0x0169, "BRTD" },
    { 0x016a, "CKJL" },
    { 0x016b, "LFCK" },
    { 0x016c, "BFLX" },
    { 0x016d, "XJRT" },
    { 0x016e, "HRTB" },
    { 0x016f, "HBHK" },
    { 0x0170, "JCGB" },
    { 0x0171, "HHFL" },
    { 0x0172, "TFBB" },
    { 0x0173, "TXHF" },
    { 0x0174, "JHJL" },
    { 0x0181, "PAR" },
    { 0x0182, "ERIC CHAHI" },
    { 0x0183, "          MUSIQUES ET BRUITAGES" },
    { 0x0184, "DE" },
    { 0x0185, "JEAN-FRANCOIS FREITAS" },
    { 0x0186, "VERSION IBM PC" },
    { 0x0187, "      PAR" },
    { 0x0188, " DANIEL MORAIS" },
    { 0x018b, "PUIS PRESSER LE BOUTON" },
    { 0x018c, "POSITIONNER LE JOYSTICK EN HAUT A GAUCHE" },
    { 0x018d, " POSITIONNER LE JOYSTICK AU CENTRE" },
    { 0x018e, " POSITIONNER LE JOYSTICK EN BAS A DROITE" },
    { 0x0258, "       Conception ..... Eric Chahi" },
    { 0x0259, "    Programmation ..... Eric Chahi" },
    { 0x025a, "     Graphismes ....... Eric Chahi" },
    { 0x025b, "Musique de ...... Jean-francois Freitas" },
    { 0x025c, "              Bruitages" },
    { 0x025d, "        Jean-Francois Freitas\n             Eric Chahi" },
    { 0x0263, "               Merci a" },
    { 0x0264, "           Jesus Martinez\n\n          Daniel Morais\n\n        Frederic Savoir\n\n      Cecile Chahi\n\n    Philippe Delamarre\n\n  Philippe Ulrich\n\nSebastien Berthet\n\nPierre Gousseau" },
    { 0x0265, "Now Go Back To Another Earth" },
    { 0x0190, "Bonsoir professeur." },
    { 0x0191, "Je vois que Monsieur a pris\nsa Ferrari." },
    { 0x0192, "IDENTIFICATION" },
    { 0x0193, "Monsieur est en parfaite sante." },
    { 0x0194, "O\n" },
    { 0x0193, "AU BOULOT !!!\n" },
    { 0xffff, "" }
};

// ---------------------------------------------------------------------------
// MemList
// ---------------------------------------------------------------------------

MemList::MemList(const std::string& dataDir, const std::string& dumpDir)
    : _dataDir(dataDir)
    , _dumpDir(dumpDir)
{
}

auto MemList::loadMemList(Resource* resources) -> bool
{
    char path[PATH_MAX] = "";

    if(_dataDir.empty() == false) {
        (void) ::snprintf(path, sizeof(path), "%s/MEMLIST.BIN", _dataDir.c_str());
    }
    if(path[0] != '\0') {
        File file("stdio");
        if(file.open(path, "rb") != false) {
            if(resources != nullptr) {
                auto*    resource = resources;
                uint16_t index = 0;
                uint8_t  buffer[32];
                while(true) {
                    if(file.read(buffer, 20) != false) {
                        Data data(buffer);
                        resource->id           = index++;
                        resource->state        = data.fetchByte();
                        resource->type         = data.fetchByte();
                        resource->unused1      = data.fetchWordBE();
                        resource->unused2      = data.fetchWordBE();
                        resource->unused3      = data.fetchByte();
                        resource->bankId       = data.fetchByte();
                        resource->bankOffset   = data.fetchLongBE();
                        resource->unused4      = data.fetchWordBE();
                        resource->packedSize   = data.fetchWordBE();
                        resource->unused5      = data.fetchWordBE();
                        resource->unpackedSize = data.fetchWordBE();
                        resource->data         = nullptr;
                        if(resource->type != RT_END) {
                            ++resource;
                        }
                        else {
                            break;
                        }
                    }
                    else {
                        return false;
                    }
                }
                dumpMemList(resources);
            }
            return true;
        }
    }
    return false;
}

auto MemList::dumpMemList(Resource* resources) -> bool
{
#ifndef NDEBUG
    ResourceStats total;
    ResourceStats sound;
    ResourceStats music;
    ResourceStats bitmap;
    ResourceStats palette;
    ResourceStats bytecode;
    ResourceStats polygon1;
    ResourceStats polygon2;
    ResourceStats unknown;

    auto percent = [](uint32_t count, uint32_t total) -> float
    {
        if(total != 0) {
            return 100.0f * (0.0f + (static_cast<float>(count) / static_cast<float>(total)));
        }
        return 0.0f;
    };

    auto gain = [](uint32_t count, uint32_t total) -> float
    {
        if(total != 0) {
            return 100.0f * (1.0f - (static_cast<float>(count) / static_cast<float>(total)));
        }
        return 0.0f;
    };

    auto log_one_resource = [&](const char* type, const Resource& resource) -> void
    {
        LOG_DEBUG("| 0x%02x | %-8s | %7d bytes | %7d bytes | %6.2f%% |", resource.id, type, resource.packedSize, resource.unpackedSize, gain(resource.packedSize, resource.unpackedSize));
    };

    auto log_all_resources = [&]() -> void
    {
        LOG_DEBUG("+------+----------+---------------+---------------+---------+");
        LOG_DEBUG("| id   | type     |   packed-size | unpacked-size |  gain %% |");
        LOG_DEBUG("+------+----------+---------------+---------------+---------+");
        auto* resource = resources;
        while(resource->type != RT_END) {
            const char* type = nullptr;
            total.count    += 1;
            total.packed   += resource->packedSize;
            total.unpacked += resource->unpackedSize;
            switch(resource->type) {
                case RT_SOUND:
                    type = "SOUND";
                    sound.count    += 1;
                    sound.packed   += resource->packedSize;
                    sound.unpacked += resource->unpackedSize;
                    break;
                case RT_MUSIC:
                    type = "MUSIC";
                    music.count    += 1;
                    music.packed   += resource->packedSize;
                    music.unpacked += resource->unpackedSize;
                    break;
                case RT_BITMAP:
                    type = "BITMAP";
                    bitmap.count    += 1;
                    bitmap.packed   += resource->packedSize;
                    bitmap.unpacked += resource->unpackedSize;
                    break;
                case RT_PALETTE:
                    type = "PALETTE";
                    palette.count    += 1;
                    palette.packed   += resource->packedSize;
                    palette.unpacked += resource->unpackedSize;
                    break;
                case RT_BYTECODE:
                    type = "BYTECODE";
                    bytecode.count    += 1;
                    bytecode.packed   += resource->packedSize;
                    bytecode.unpacked += resource->unpackedSize;
                    break;
                case RT_POLYGON1:
                    type = "POLYGON1";
                    polygon1.count    += 1;
                    polygon1.packed   += resource->packedSize;
                    polygon1.unpacked += resource->unpackedSize;
                    break;
                case RT_POLYGON2:
                    type = "POLYGON2";
                    polygon2.count    += 1;
                    polygon2.packed   += resource->packedSize;
                    polygon2.unpacked += resource->unpackedSize;
                    break;
                default:
                    type = "UNKNOWN";
                    unknown.count    += 1;
                    unknown.packed   += resource->packedSize;
                    unknown.unpacked += resource->unpackedSize;
                    break;
            }
            log_one_resource(type, *resource);
            ++resource;
        }
        LOG_DEBUG("+------+----------+---------------+---------------+---------+");
    };

    auto log_one_stats = [&](const char* type, const ResourceStats& stats) -> void
    {
        LOG_DEBUG("| %-8s | %5d | %7d bytes | %7d bytes | %6.2f%% | %6.2f%% |", type, stats.count, stats.packed, stats.unpacked, gain(stats.packed, stats.unpacked), percent(stats.unpacked, total.unpacked));
    };

    auto log_all_stats = [&]() -> void
    {
        LOG_DEBUG("+----------+-------+---------------+---------------+---------+---------+");
        LOG_DEBUG("| type     | count |   packed-size | unpacked-size |  gain %% | total %% |");
        LOG_DEBUG("+----------+-------+---------------+---------------+---------+---------+");
        log_one_stats("SOUND"   , sound   );
        log_one_stats("MUSIC"   , music   );
        log_one_stats("BITMAP"  , bitmap  );
        log_one_stats("PALETTE" , palette );
        log_one_stats("BYTECODE", bytecode);
        log_one_stats("POLYGON1", polygon1);
        log_one_stats("POLYGON2", polygon2);
        log_one_stats("UNKNOWN" , unknown );
        log_one_stats("TOTAL"   , total   );
        LOG_DEBUG("+----------+-------+---------------+---------------+---------+---------+");
    };

    if(resources != nullptr) {
        log_all_resources();
        log_all_stats();
    }
#endif
    return true;
}

auto MemList::loadResource(Resource& resource) -> bool
{
    char path[PATH_MAX] = "";

    if(_dataDir.empty() == false) {
        (void) ::snprintf(path, sizeof(path), "%s/BANK%02X", _dataDir.c_str(), resource.bankId);
    }
    if(path[0] != '\0') {
        File file("stdio");
        if(file.open(path, "rb") != false) {
            if(file.seek(resource.bankOffset) == false) {
                return false;
            }
            if(file.read(resource.data, resource.packedSize) == false) {
                return false;
            }
            if(resource.packedSize != resource.unpackedSize) {
                ByteKiller bytekiller(resource.data, resource.packedSize, resource.unpackedSize);

                return bytekiller.unpack();
            }
            return true;
        }
    }
    return false;
}

auto MemList::dumpResource(const Resource& resource) -> void
{
    char path[PATH_MAX] = "";

    if(resource.data == nullptr) {
        return;
    }
    if(_dumpDir.empty() == false) {
        const char* type = nullptr;
        switch(resource.type) {
            case RT_SOUND:
                type = "sound";
                break;
            case RT_MUSIC:
                type = "music";
                break;
            case RT_BITMAP:
                type = "bitmap";
                break;
            case RT_PALETTE:
                type = "palette";
                break;
            case RT_BYTECODE:
                type = "bytecode";
                break;
            case RT_POLYGON1:
                type = "polygon1";
                break;
            case RT_POLYGON2:
                type = "polygon2";
                break;
            default:
                type = "unknown";
                break;
        }
        (void) ::snprintf(path, sizeof(path), "%s/%02x_%s.data", _dumpDir.c_str(), resource.id, type);
    }
    if(path[0] != '\0') {
        File file("stdio");
        if(file.open(path, "wb") != false) {
            file.write(resource.data, resource.unpackedSize);
        }
    }
}

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------
