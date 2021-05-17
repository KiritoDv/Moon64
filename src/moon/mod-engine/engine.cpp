#include "engine.h"

#include "moon/utils/moon-env.h"
#include "interfaces/file-entry.h"
#include "textures/mod-texture.h"

#include "moon/zip/straw.h"
#include "moon/libs/nlohmann/json.hpp"
using json = nlohmann::json;
using namespace std;

#include <iostream>
#include <string>
#include <limits.h>
#include <dirent.h>
#include <thread>
#include <future>

extern "C" {
#include "text/libs/io_utils.h"
#include "pc/platform.h"
#include "pc/fs/fs.h"
}

#include "moon/display/mloading-wnd.h"

namespace Moon {

    vector<BitModule*> addons;

    void loadAddon(string addonPath){
        MoonWindow::updateMessage("Loading addon: " + addonPath.substr(addonPath.find_last_of("/\\") + 1));

        StrawFile file(addonPath);
        file.open();
        if(file.exists("properties.json")){
            string tjson = file.read("properties.json");

            json j = json::parse(tjson);
            if(j.contains("bit") && j["bit"].contains("name")){
                BitModule* bit = new BitModule({
                    .name        = j["bit"]["name"],
                    .description = j["bit"].contains("icon") ? j["bit"]["description"] : "None",
                    .authors     = j["bit"]["authors"],
                    .version     = j["bit"]["version"],
                    .website     = j["bit"].contains("website") ? j["bit"]["website"]  : "None",
                    .icon        = j["bit"].contains("icon")    ? j["bit"]["icon"]     : "None"
                });

                if(j["bit"].contains("main")) bit->main = j["bit"]["main"];

                bit->path        = addonPath;
                bit->readOnly    = false;

                if(file.exists(bit->icon)){
                    vector<string> allowedTextures = {"png", "jpg", "jpeg"};
                    if(std::count(allowedTextures.begin(), allowedTextures.end(), string(get_filename_ext(bit->icon.c_str())))){
                        TextureFileEntry *entry = new TextureFileEntry();
                        file.read(bit->icon, entry);
                        Moon::saveAddonTexture(bit, "mod-icons://"+bit->name, entry);
                    }
                    if(!string(get_filename_ext(bit->icon.c_str())).compare("json")){
                        string modName = bit->icon.substr(bit->icon.length());
                        cout << "Found animated icon texture " << modName << endl;
                        json mods = json::parse(file.read(bit->icon));
                        for (json::iterator entry = mods.begin(); entry != mods.end(); ++entry) {
                            Moon::bindTextureModifier("mod-icons://"+bit->name, entry.key(), entry.value());
                        }
                    }
                }

                if(file.exists("assets/")){
                    for(auto &name : file.entries()){
                        string texturePaths[] = {
                            "assets/graphics/",
                            "assets/models/"
                        };

                        string textsPath = "assets/texts/";

                        for(auto &path : texturePaths){
                            if(!name.rfind(path, 0)){
                                vector<string> allowedTextures = {"png", "jpg", "jpeg"};
                                if(std::count(allowedTextures.begin(), allowedTextures.end(), string(get_filename_ext(name.c_str())))){
                                    string texName = name.substr(path.length());
                                    string rawname = texName.substr(0, texName.find_last_of("."));

                                    MoonWindow::updateMessage(rawname.substr(rawname.find_last_of("/\\") + 1));
                                    TextureFileEntry *entry = new TextureFileEntry();
                                    file.read(name, entry);
                                    Moon::saveAddonTexture(bit, rawname, entry);
                                }
                                if(!string(get_filename_ext(name.c_str())).compare("json")){
                                    string modName = name.substr(path.length());
                                    MoonWindow::updateMessage("Found animated texture " + modName);
                                    json mods = json::parse(file.read(name));
                                    for (json::iterator entry = mods.begin(); entry != mods.end(); ++entry) {
                                        Moon::bindTextureModifier(modName.substr(0, modName.find_last_of(".")), entry.key(), entry.value());
                                    }
                                }
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
            std::cout << "addon: [" << file.getPath() << "]" << std::endl;
            std::cout << "Missing properties.json" << std::endl;
        }
        MoonWindow::addAddonCounter();
    }
}

namespace MoonInternal {
    void scanAddonsDirectory() {
        string cwd = MoonInternal::getEnvironmentVar("MOON_CWD");
        string addonsDir = cwd.substr(0, cwd.find_last_of("/\\")) + "/addons/";
        vector<string> foundAddons;
        DIR *dir = opendir(addonsDir.c_str());
        if (dir) {
            struct dirent *de;
            while ((de = readdir(dir)) != NULL) {
                string extension = string(get_filename_ext(de->d_name));
                if (!extension.compare("bit"))
                    foundAddons.push_back(addonsDir + de->d_name);
            }
            closedir(dir);
        }
        MoonWindow::setMaxAddons(foundAddons.size());
        for(auto &path : foundAddons)
            Moon::loadAddon(path);
    }

    void setupModEngine( string state ){
        MoonInternal::setupTextureEngine(state);
        MoonWindow::setupLoadingWindow(state);
        if(state == "Init"){
            MoonInternal::scanAddonsDirectory();
            vector<int> order;
            for(int i = 0; i < Moon::addons.size(); i++) order.push_back(i);
            MoonInternal::buildTextureCache(order);
            return;
        }
        if(state == "Exit"){
            Moon::addons.clear();
            return;
        }
    }
}