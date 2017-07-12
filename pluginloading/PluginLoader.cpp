//
// Created by yannick lamprecht on 24.02.17.
//


#include <iostream>
#include <fstream>
#if _WIN32 || _WIN64
#include <fibersapi.h>
#endif

#include "PluginLoader.h"

#if _WIN32 || _WIN64

#include <rpc.h>

#else
#include <dlfcn.h>
#endif


ysl::PluginLoader::PluginLoader(const std::string& filePath, const std::vector<std::string>& fileEndings) {
    this->filePath = filePath;
    this->fileEndings = fileEndings;
}

void ysl::PluginLoader::load() {
    this->pluginFiles= std::unordered_map<std::string, std::shared_ptr<IPlugin>>();
    this->pluginHandles = std::unordered_map<std::string, std::shared_ptr<PluginHandle>>();
    FileReader reader;
    std::vector<std::string> files = reader.readDir(filePath, fileEndings);
    std::cout << "Files available: " << files.size() << std::endl;
    for (const std::string& name : files) {
        this->load(name);
    }
}

std::unordered_map<std::string, std::shared_ptr<IPlugin>> ysl::PluginLoader::getLoadedPlugins() {
    return pluginFiles;
}


void ysl::PluginLoader::load(const std::string& pluginFileName) {
    std::shared_ptr<PluginHandle> handle = std::shared_ptr<PluginHandle>(new PluginHandle);

#if _WIN32 || _WIN64

    HINSTANCE hGetProcIDDLL = LoadLibrary(pluginFileName.c_str());

    if (!hGetProcIDDLL) {
        std::cout << "could not load the dynamic library" << std::endl;
        return;
    }

    handle.handle = hGetProcIDDLL;

    handle.create = (create_t *) GetProcAddress(hGetProcIDDLL, "create");
    if (!handle.create) {
        std::cout << "could not locate the function" << std::endl;
        return;
    }

#else

    handle->handle = dlopen(pluginFileName.c_str(), RTLD_LAZY);

    if (!handle->handle) {
        std::cerr << "Cannot load library: " << dlerror() << '\n';
        return;
    }

    // reset errors
    dlerror();

    // load the symbols
    handle->create = (create_t *) dlsym(handle->handle, "create");
    const char *dlsym_error = dlerror();
    if (dlsym_error) {
        std::cerr << "Cannot load symbol create: " << dlsym_error << '\n';
        return;
    }

#endif

    std::shared_ptr<IPlugin> iPlugin = handle->create();


    const std::string pluginName = iPlugin->getName();

    pluginFiles[pluginName] = iPlugin;
    pluginHandles[pluginName] = handle;
}



void ysl::PluginLoader::unload(const std::string pluginName) {

    pluginFiles.erase(pluginName);

    PluginHandle handle = *pluginHandles[pluginName];
    pluginHandles.erase(pluginName);

#if _WIN32 || _WIN64
    FreeLibrary((HMODULE) handle.handle);
#else
    std::cout << "check if success 0: "<<dlclose(handle.handle)<<std::endl;
#endif


}

void ysl::PluginLoader::unload() {
    for (auto &pluginPair : pluginFiles) {
        unload(pluginPair.first);
    }
}

void ysl::PluginLoader::enable(const std::string& pluginName) {
    pluginFiles[pluginName]->onEnable();
}

void ysl::PluginLoader::disable(const std::string& pluginName) {
    pluginFiles[pluginName]->onDisable();
}

void ysl::PluginLoader::disable() {
    for (const auto &pluginPair: pluginFiles) {
        pluginPair.second->onDisable();
    }
}


void ysl::PluginLoader::enable() {
    for (const auto &pluginPair: pluginFiles) {
        pluginPair.second->onEnable();
    }
}