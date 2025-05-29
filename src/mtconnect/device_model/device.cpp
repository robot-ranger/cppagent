//
// Copyright Copyright 2009-2024, AMT – The Association For Manufacturing Technology (“AMT”)
// All rights reserved.
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
//

#include "device.hpp"

#include "mtconnect/configuration/config_options.hpp"
#include "mtconnect/entity/factory.hpp"
#include "mtconnect/logging.hpp"

using namespace std;

namespace mtconnect {
  using namespace entity;
  using namespace configuration;

  namespace device_model {
    entity::FactoryPtr Device::getFactory()
    {
      static FactoryPtr factory;
      if (!factory)
      {
        factory = make_shared<Factory>(*Component::getFactory());
        factory->getRequirement("name")->setMultiplicity(1, 1);
        factory->getRequirement("uuid")->setMultiplicity(1, 1);
        factory->addRequirements(
            {{"iso841Class", false}, {"mtconnectVersion", false}, {"hash", false}});
        factory->setFunction([](const std::string &name, Properties &ps) -> EntityPtr {
          auto device = make_shared<Device>("Device"s, ps);
          device->initialize();
          return device;
        });
        Component::getFactory()->registerFactory("Device", factory);
      }
      return factory;
    }

    entity::FactoryPtr Device::getRoot()
    {
      static auto factory = make_shared<Factory>(
          Requirements {{"Device", ValueType::ENTITY, getFactory(), 1, Requirement::Infinite}});

      return factory;
    }

    void Device::registerDataItem(DataItemPtr di)
    {
      if (auto it = m_dataItems.get<ById>().find(di->getId()); it != m_dataItems.get<ById>().end())
      {
        LOG(fatal) << "Device " << getName() << ": Duplicatie data item id  '" << di->getId()
                   << "', Exiting";
        exit(1);
      }

      if (di->hasProperty("Source") && di->getSource()->hasValue())
      {
        auto source = di->getSource()->getValue<std::string>();
        if (auto it = m_dataItems.get<BySource>().find(source);
            it != m_dataItems.get<BySource>().end())
        {
          LOG(warning) << "Device " << getName() << ": Duplicate source '" << source
                       << "' found in data item '" << di->getId() << "'. Previous data item: '"
                       << it->lock()->getId() << '\'';
        }
      }

      auto name = di->getName();
      if (name)
      {
        if (auto it = m_dataItems.get<ByName>().find(*name); it != m_dataItems.get<ByName>().end())
        {
          LOG(warning) << "Device " << getName() << ": Duplicate source '" << *name
                       << "' found in data item '" << di->getId() << "'. Previous data item: '"
                       << it->lock()->getId() << '\'';
          LOG(warning) << "    Name '" << *name
                       << "' may not resolve correctly on incoming streams";
        }
      }

      auto [id, added] = m_dataItems.emplace(di);
      if (!added)
      {
        LOG(fatal) << "Device " << getName() << ": DataItem '" << di->getId()
                   << " could not be added, exiting";
        exit(1);
      }
    }

    Device::Device(const std::string &name, entity::Properties &props) : Component(name, props)
    {
      NAMED_SCOPE("device");
      auto items = getList("DataItems");
      if (items)
      {
        for (auto &item : *items)
        {
          auto di = dynamic_pointer_cast<data_item::DataItem>(item);
          cachePointers(di);
        }
      }
    }

    void Device::setOptions(const ConfigOptions &options)
    {
      if (auto opt = GetOption<bool>(options, PreserveUUID))
        m_preserveUuid = *opt;
    }

    // TODO: Clean up these initialization methods for data items
    void Device::addDeviceDataItem(DataItemPtr dataItem)
    {
      auto [id, added] = m_dataItems.emplace(dataItem);
      if (!added)
      {
        LOG(error) << "Duplicate data item id: " << dataItem->getId() << " for device "
                   << get<string>("name") << ", skipping";
      }
    }

    void Device::addDataItem(DataItemPtr dataItem, entity::ErrorList &errors)
    {
      Component::addDataItem(dataItem, errors);
      cachePointers(dataItem);
    }

    void Device::cachePointers(DataItemPtr dataItem)
    {
      if (dataItem->getType() == "AVAILABILITY")
        m_availability = dataItem;
      else if (dataItem->getType() == "ASSET_CHANGED")
        m_assetChanged = dataItem;
      else if (dataItem->getType() == "ASSET_REMOVED")
        m_assetRemoved = dataItem;
      else if (dataItem->getType() == "ASSET_COUNT")
        m_assetCount = dataItem;
    }

    DataItemPtr Device::getDeviceDataItem(const std::string &name) const
    {
      if (auto it = m_dataItems.get<ById>().find(name); it != m_dataItems.get<ById>().end())
        return it->lock();

      if (auto it = m_dataItems.get<ByOriginalId>().find(name);
          it != m_dataItems.get<ByOriginalId>().end())
        return it->lock();

      if (auto it = m_dataItems.get<ByName>().find(name); it != m_dataItems.get<ByName>().end())
        return it->lock();

      if (auto it = m_dataItems.get<BySource>().find(name); it != m_dataItems.get<BySource>().end())
        return it->lock();

      return nullptr;
    }

    void Device::createUniqueIds(std::unordered_map<std::string, std::string> &idMap)
    {
      boost::uuids::detail::sha1 sha;
      sha.process_bytes(m_uuid->data(), m_uuid->size());

      Component::createUniqueId(idMap, sha);
      updateReferences(idMap);
      initialize();
    }

  }  // namespace device_model
}  // namespace mtconnect
