#include "serialize.hpp"

#include "type.hpp"

#include <cereal/archives/binary.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/tuple.hpp>
#include <cereal/types/variant.hpp>
#include <cereal/types/vector.hpp>
#include <phosphor-logging/lg2.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>

PHOSPHOR_LOG2_USING;

// Register class version with Cereal
CEREAL_CLASS_VERSION(pldm::serialize::Serialize, 1)

namespace pldm
{
namespace serialize
{
namespace fs = std::filesystem;

void Serialize::serialize(const std::string& path, const std::string& intf,
                          const std::string& name, dbus::PropertyValue value)
{
    if (path.empty() || intf.empty())
    {
        return;
    }

    if (!entityPathMaps.contains(path))
    {
        return;
    }

    uint16_t type = entityPathMaps[path].entity_type;
    uint16_t num = entityPathMaps[path].entity_instance_num;
    uint16_t cid = entityPathMaps[path].entity_container_id;

    if (!savedObjs.contains(type) || !savedObjs[type].contains(path))
    {
        std::map<std::string, std::map<std::string, pldm::dbus::PropertyValue>>
            maps{{{intf, {{name, value}}}}};
        savedObjs[type][path] = std::make_tuple(num, cid, maps);
    }
    else
    {
        auto& [num, cid, objs] = savedObjs[type][path];

        if (objs.empty())
        {
            objs[intf][name] = value;
        }
        else
        {
            if (value != objs[intf][name])
            {
                // The value is changed and is not equal to
                // the value in the in-memory cache, so update it
                // and update the persistent cache file
                objs[intf][name] = value;
            }
            else
            {
                // The value in memory cache is same as the new value
                // so no need to serialise it again
                return;
            }
        }
    }

    if (!storeEntityTypes.contains(entityPathMaps[path].entity_type))
    {
        return;
    }

    auto dir = filePath.parent_path();
    if (!fs::exists(dir))
    {
        fs::create_directories(dir);
    }

    std::ofstream os(filePath.c_str(), std::ios::binary);
    cereal::BinaryOutputArchive oarchive(os);
    oarchive(savedObjs, savedKeyVal);
}

void Serialize::serializeKeyVal(const std::string& key,
                                dbus::PropertyValue value)
{
    std::ofstream os(filePath.c_str(), std::ios::binary);
    cereal::BinaryOutputArchive oarchive(os);
    savedKeyVal[key] = value;
    oarchive(savedObjs, savedKeyVal);
}

bool Serialize::deserialize()
{
    if (!fs::exists(filePath))
    {
        error("File does not exist, FILE_PATH = {FILE_PATH}", "FILE_PATH",
              filePath.c_str());
        return false;
    }

    try
    {
        savedObjs.clear();
        savedKeyVal.clear();
        std::ifstream is(filePath.c_str(), std::ios::in | std::ios::binary);
        cereal::BinaryInputArchive iarchive(is);
        iarchive(savedObjs, savedKeyVal);

        return true;
    }
    catch (const cereal::Exception& e)
    {
        error("Failed to restore groups, ERROR = {ERR_EXCEP}", "ERR_EXCEP",
              e.what());
        fs::remove(filePath);
    }

    return false;
}

void Serialize::setEntityTypes(const std::set<uint16_t>& storeEntities)
{
    storeEntityTypes = storeEntities;
}

void Serialize::setObjectPathMaps(const ObjectPathMaps& maps)
{
    for (const auto& [objpath, nodeentity] : maps)
    {
        pldm_entity entity = pldm_entity_extract(nodeentity);
        entityPathMaps.emplace(objpath, entity);
    }
}

void Serialize::reSerialize(const std::vector<uint16_t> types)
{
    if (types.empty())
    {
        return;
    }

    for (const auto& type : types)
    {
        if (savedObjs.contains(type))
        {
            info(
                "Removing objects of type : {OBJ_TYP} from the persistent cache",
                "OBJ_TYP", (unsigned)type);
            savedObjs.erase(savedObjs.find(type));
        }
    }

    auto dir = filePath.parent_path();
    if (!fs::exists(dir))
    {
        fs::create_directories(dir);
    }

    std::ofstream os(filePath.c_str(), std::ios::binary);
    cereal::BinaryOutputArchive oarchive(os);
    oarchive(this->savedObjs, this->savedKeyVal);
}

} // namespace serialize
} // namespace pldm
