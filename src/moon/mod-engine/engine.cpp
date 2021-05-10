#include "engine.h"
#include <iostream>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <dirent.h>

#include "moon/utils/moon-env.h"
#include "moon/mod-engine/interfaces/mod-module.h"
#include "moon/mod-engine/modules/test-module.h"
#include "interfaces/file-entry.h"
#include "textures/mod-texture.h"
#include "moon/libs/miniz/zip_file.hpp"
#include "moon/libs/nlohmann/json.hpp"
using json = nlohmann::json;


using namespace std;

extern "C" {
#include "moon/moon64.h"
#include "moon/libs/lua/lualib.h"
#include "moon/libs/lua/lauxlib.h"
#include "moon/libs/lua/lua.h"
#include "text/libs/io_utils.h"
#include "pc/platform.h"
#include "pc/fs/fs.h"
}

namespace Moon {
    vector<BitModule*> addons;

    void loadAddon(string addonPath){
        miniz_cpp::zip_file file(addonPath);

        cout << "Loading addon: " << addonPath << endl;

        if(file.has_file("properties.json")){
            string tjson = file.read("properties.json");

            json j = json::parse(tjson);
            if(j.contains("bit") && j["bit"].contains("name")){
                BitModule* bit = new BitModule();
                bit->name        = j["bit"]["name"];
                bit->description = j["bit"]["description"];
                bit->author      = j["bit"]["author"];
                bit->version     = j["bit"]["version"];
                bit->website     = j["bit"]["website"];
                bit->icon        = j["bit"]["icon"];
                bit->main        = j["bit"]["main"];
                bit->path        = addonPath;
                bit->readOnly    = false;

                if(file.has_file(bit->main)){
                    std::cout << file.read(bit->main) << std::endl;
                }

                if(file.has_file(bit->icon)){
                    vector<string> allowedTextures = {"png", "jpg", "jpeg"};
                    if(std::count(allowedTextures.begin(), allowedTextures.end(), string(get_filename_ext(bit->icon.c_str())))){
                        Moon::saveAddonTexture(bit, "mod-icons://"+bit->name, new TextureFileEntry({.path = bit->icon}));
                    }
                }

                if(file.has_file("assets/")){
                    for(auto &name : file.namelist()){
                        string graphicsPath = "assets/graphics/";
                        string textsPath = "assets/texts/";

                        if(!name.rfind(graphicsPath, 0)){
                            vector<string> allowedTextures = {"png", "jpg", "jpeg"};
                            if(std::count(allowedTextures.begin(), allowedTextures.end(), string(get_filename_ext(name.c_str())))){
                                string texName = name.substr(graphicsPath.length());
                                string rawname = texName.substr(0, texName.find_last_of("."));

                                TextureFileEntry *entry = new TextureFileEntry();
                                file.read_texture(name, &entry);
                                Moon::saveAddonTexture(bit, rawname, entry);
                            }
                        }
                        if(!name.rfind(textsPath, 0)){
                            if(!string(get_filename_ext(name.c_str())).compare("json")){
                                // std::cout << name << std::endl;
                            }
                        }
                    }
                }

                addons.push_back(bit);
            }
        } else {
            std::cout << "Failed to load addon: [" << file.get_filename() << "]" << std::endl;
            std::cout << "Missing properties.json" << std::endl;
        }
    }

    void scanAddonsDirectory() {
        string cwd = MoonInternal::getEnvironmentVar("MOON_CWD");
        string addonsDir = cwd.substr(0, cwd.find_last_of("/\\")) + "/addons/";

        DIR *dir = opendir(addonsDir.c_str());
        if (dir) {
            struct dirent *de;
            while ((de = readdir(dir)) != NULL) {
                string extension = string(get_filename_ext(de->d_name));
                if (extension.compare("bit") == 0) {
                    string file = addonsDir + de->d_name;
                    loadAddon(file);
                }
            }
            closedir(dir);
        }
    }

    void setupModEngine( string state ){
        if(state == "PreInit"){
            MoonInternal::buildDefaultAddon();
            return;
        }
        if(state == "Init"){
            Moon::loadAddon("/home/alex/Music/Moon64-Packs/converted/beta-hud.bit");
            Moon::loadAddon("/home/alex/disks/uwu/Projects/UnderVolt/Moon64-Packs/converted/owo.bit");
            Moon::loadAddon("/home/alex/Music/Moon64-Packs/converted/minecraft.bit");
            MoonInternal::buildTextureCache();
            return;
        }
    }
}