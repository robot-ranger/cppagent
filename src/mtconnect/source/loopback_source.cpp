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

#include "mtconnect/source/loopback_source.hpp"

#include "mtconnect/configuration/config_options.hpp"
#include "mtconnect/device_model/device.hpp"
#include "mtconnect/entity/xml_parser.hpp"
#include "mtconnect/pipeline/convert_sample.hpp"
#include "mtconnect/pipeline/deliver.hpp"
#include "mtconnect/pipeline/delta_filter.hpp"
#include "mtconnect/pipeline/duplicate_filter.hpp"
#include "mtconnect/pipeline/period_filter.hpp"
#include "mtconnect/pipeline/timestamp_extractor.hpp"
#include "mtconnect/pipeline/upcase_value.hpp"
#include "mtconnect/pipeline/validator.hpp"
#include "mtconnect/pipeline/correct_timestamp.hpp"

using namespace std;

namespace mtconnect::source {
  using namespace observation;
  using namespace asset;
  using namespace pipeline;
  void LoopbackPipeline::build(const ConfigOptions &options)
  {
    m_options = options;

    clear();
    TransformPtr next = m_start;

    next->bind(make_shared<DeliverAsset>(m_context));
    next->bind(make_shared<DeliverAssetCommand>(m_context));
    next->bind(make_shared<DeliverDevice>(m_context));
    next->bind(make_shared<DeliverDevices>(m_context));

    if (IsOptionSet(m_options, configuration::UpcaseDataItemValue))
      next = next->bind(make_shared<UpcaseValue>());

    // Filter dups, by delta, and by period
    next = next->bind(make_shared<DuplicateFilter>(m_context));
    next = next->bind(make_shared<DeltaFilter>(m_context));
    next = next->bind(make_shared<PeriodFilter>(m_context, m_strand));

    // Convert values
    if (IsOptionSet(m_options, configuration::ConversionRequired))
      next = next->bind(make_shared<ConvertSample>());
    
    if (IsOptionSet(m_options, configuration::CorrectTimestamps))
      next = next->bind(make_shared<CorrectTimestamp>(m_context));

    // Validate Values
    if (IsOptionSet(m_options, configuration::Validation))
      next = next->bind(make_shared<Validator>(m_context));

    // Deliver
    next->bind(make_shared<DeliverObservation>(m_context));
    applySplices();
  }

  SequenceNumber_t LoopbackSource::receive(DataItemPtr dataItem, entity::Properties props,
                                           std::optional<Timestamp> timestamp)
  {
    entity::ErrorList errors;

    Timestamp ts = timestamp ? *timestamp : chrono::system_clock::now();
    auto observation = observation::Observation::make(dataItem, props, ts, errors);
    if (observation && errors.empty())
    {
      return receive(observation);
    }
    else
    {
      LOG(error) << "Cannot add observation: ";
      for (auto &e : errors)
      {
        LOG(error) << "Cannot add observation: " << e->what();
      }
    }

    return 0;
  }

  SequenceNumber_t LoopbackSource::receive(DataItemPtr dataItem, const std::string &value,
                                           std::optional<Timestamp> timestamp)
  {
    if (dataItem->isCondition())
      return receive(dataItem, {{"level", value}}, timestamp);
    else
      return receive(dataItem, {{"VALUE", value}}, timestamp);
  }

  SequenceNumber_t LoopbackSource::receive(const std::string &data)
  {
    auto ent = make_shared<Entity>("Data", Properties {{"VALUE", data}, {"source", getIdentity()}});
    auto res = m_pipeline.run(std::move(ent));
    if (auto obs = std::dynamic_pointer_cast<observation::Observation>(res))
    {
      return obs->getSequence();
    }
    else
    {
      return 0;
    }
  }

  void LoopbackSource::receive(DevicePtr device) { m_pipeline.run(device); }

  AssetPtr LoopbackSource::receiveAsset(DevicePtr device, const std::string &document,
                                        const std::optional<std::string> &id,
                                        const std::optional<std::string> &type,
                                        const std::optional<std::string> &time,
                                        entity::ErrorList &errors)
  {
    // Parse the asset
    auto entity = entity::XmlParser::parse(asset::Asset::getRoot(), document, errors);
    if (!entity)
    {
      LOG(warning) << "Asset could not be parsed";
      LOG(warning) << document;
      for (auto &e : errors)
        LOG(warning) << e->what();
      return nullptr;
    }

    auto asset = dynamic_pointer_cast<asset::Asset>(entity);

    if (type && asset->getType() != *type)
    {
      stringstream msg;
      msg << "Asset types do not match: "
          << "Parsed type: " << asset->getType() << " does not match " << *type;
      LOG(warning) << msg.str();
      LOG(warning) << document;
      errors.emplace_back(make_unique<entity::EntityError>(msg.str()));
      return asset;
    }

    if (!id && !asset->hasProperty("assetId"))
    {
      stringstream msg;
      msg << "Asset does not have an assetId and assetId not given";
      LOG(warning) << msg.str();
      LOG(warning) << document;
      errors.emplace_back(make_unique<entity::EntityError>(msg.str()));
      return asset;
    }

    if (id)
      asset->setAssetId(*id);

    if (time)
      asset->setProperty("timestamp", *time);

    auto ad = asset->getDeviceUuid();
    if (!ad)
      asset->setProperty("deviceUuid", *device->getUuid());

    receive(asset);

    return asset;
  }

  void LoopbackSource::removeAsset(const std::optional<std::string> device, const std::string &id)
  {
    auto ac = make_shared<AssetCommand>("AssetCommand", Properties {});
    ac->m_timestamp = chrono::system_clock::now();
    ac->setValue("RemoveAsset"s);
    ac->setProperty("assetId", id);
    if (device)
      ac->setProperty("device", *device);

    m_pipeline.run(ac);
  }

}  // namespace mtconnect::source
