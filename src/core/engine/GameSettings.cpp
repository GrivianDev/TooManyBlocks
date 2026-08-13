#include "GameSettings.h"

#include <json/JsonParser.h>

#include <fstream>

#include "Logger.h"
#include "foundation/util/Utility.h"

template <typename T>
void readSetting(const Json::JsonObject& json, const char* key, T& value) {
    try {
        auto it = json.find(key);
        if (it == json.end()) return;

        const Json::JsonValue& jsonValue = it->second;

        if constexpr (std::is_same_v<T, bool>) {
            value = jsonValue.toBool();
        } else if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
            if (jsonValue.isInt()) {
                value = static_cast<T>(jsonValue.toInt());
            } else if (jsonValue.isDouble()) {
                value = static_cast<T>(jsonValue.toDouble());
            }
        } else {
            value = static_cast<T>(jsonValue);
        }
    } catch (const Json::JsonTypeException& e) {
        lgr::lout.error("Failed to load setting \"" + std::string(key) + "\": " + std::string(e.what()));
    }
}

static Json::JsonValue toJson(const GraphicsSettings& settings) {
    Json::JsonObject json;

    json["windowMode"] = settings.windowMode;
    json["vsync"] = settings.vsync;

    json["renderDistance"] = settings.renderDistance;
    json["simulationDistance"] = settings.simulationDistance;

    json["renderScale"] = static_cast<double>(settings.renderScale);
    json["fieldOfView"] = static_cast<double>(settings.fieldOfView);

    json["entityShadows"] = settings.entityShadows;
    json["particles"] = settings.particles;
    json["clouds"] = settings.clouds;
    json["ambientOcclusion"] = settings.ambientOcclusion;

    json["shadowQuality"] = settings.shadowQuality;
    json["textureQuality"] = settings.textureQuality;
    json["particleQuality"] = settings.particleQuality;

    json["maxFramerate"] = settings.maxFramerate;

    return json;
}

static Json::JsonValue toJson(const AudioSettings& settings) {
    Json::JsonObject json;

    json["outputDevice"] = settings.outputDevice;

    json["masterVolume"] = static_cast<double>(settings.masterVolume);
    json["musicVolume"] = static_cast<double>(settings.musicVolume);
    json["effectVolume"] = static_cast<double>(settings.effectVolume);
    json["ambientVolume"] = static_cast<double>(settings.ambientVolume);

    return json;
}

static void fromJson(GraphicsSettings& settings, const Json::JsonValue& value) {
    if (!value.isObject()) return;

    const Json::JsonObject& json = value.toObject();

    readSetting(json, "windowMode", settings.windowMode);
    readSetting(json, "vsync", settings.vsync);

    readSetting(json, "renderDistance", settings.renderDistance);
    readSetting(json, "simulationDistance", settings.simulationDistance);

    readSetting(json, "renderScale", settings.renderScale);
    readSetting(json, "fieldOfView", settings.fieldOfView);

    readSetting(json, "entityShadows", settings.entityShadows);
    readSetting(json, "particles", settings.particles);
    readSetting(json, "clouds", settings.clouds);
    readSetting(json, "ambientOcclusion", settings.ambientOcclusion);

    readSetting(json, "shadowQuality", settings.shadowQuality);
    readSetting(json, "textureQuality", settings.textureQuality);
    readSetting(json, "particleQuality", settings.particleQuality);

    readSetting(json, "maxFramerate", settings.maxFramerate);
}

static void fromJson(AudioSettings& settings, const Json::JsonValue& value) {
    if (!value.isObject()) return;

    const Json::JsonObject& json = value.toObject();

    readSetting(json, "outputDevice", settings.outputDevice);

    readSetting(json, "masterVolume", settings.masterVolume);
    readSetting(json, "musicVolume", settings.musicVolume);
    readSetting(json, "effectVolume", settings.effectVolume);
    readSetting(json, "ambientVolume", settings.ambientVolume);
}

void loadSettings(GameSettings& settings) {
    std::filesystem::path settingsFile = getAppDataPath() / "settings.json";
    if (!std::filesystem::exists(settingsFile)) return;

    const Json::JsonValue settingsJson = Json::parseJson(readFile(settingsFile));
    fromJson(settings.graphics, settingsJson["GraphicsSettings"]);
    fromJson(settings.audio, settingsJson["AudioSettings"]);
}

void saveSettings(const GameSettings& settings) {
    std::filesystem::path settingsFile = getAppDataPath() / "settings.json";
    std::filesystem::create_directories(settingsFile.parent_path());

    Json::JsonObject settingsJson;
    settingsJson["GraphicsSettings"] = toJson(settings.graphics);
    settingsJson["AudioSettings"] = toJson(settings.audio);

    std::ofstream file(settingsFile.string());
    file << Json::toJsonString(settingsJson);
    file.close();
}
