//
// Copyright Copyright 2009-2023, AMT – The Association For Manufacturing Technology (“AMT”)
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

#include "mqtt_service.hpp"

#include <nlohmann/json.hpp>

#include "mtconnect/configuration/config_options.hpp"
#include "mtconnect/entity/entity.hpp"
#include "mtconnect/entity/factory.hpp"
#include "mtconnect/entity/json_parser.hpp"
#include "mtconnect/mqtt/mqtt_client_impl.hpp"
#include "mtconnect/printer/json_printer.hpp"

using ptree = boost::property_tree::ptree;
using json = nlohmann::json;

using namespace std;
using namespace mtconnect;
using namespace mtconnect::asset;

namespace asio = boost::asio;
namespace config = ::mtconnect::configuration;

namespace mtconnect {
  namespace sink {
    namespace mqtt_sink {
      // get obeservation in
      // create a json printer
      // call print

      MqttService::MqttService(boost::asio::io_context &context, sink::SinkContractPtr &&contract,
                               const ConfigOptions &options, const ptree &config)
        : Sink("MqttService", std::move(contract)),
          m_context(context),
          m_strand(context),
          m_options(options),
          m_currentTimer(context)
      {
        // Unique id number for agent instance
        m_instanceId = getCurrentTimeInSec();

        auto jsonPrinter = dynamic_cast<printer::JsonPrinter *>(m_sinkContract->getPrinter("json"));

        m_jsonPrinter = make_unique<entity::JsonEntityPrinter>(jsonPrinter->getJsonVersion());

        m_printer = std::make_unique<printer::JsonPrinter>(jsonPrinter->getJsonVersion());

        GetOptions(config, m_options, options);
        AddOptions(config, m_options,
                   {{configuration::ProbeTopic, string()},
                    {configuration::MqttCaCert, string()},
                    {configuration::MqttPrivateKey, string()},
                    {configuration::MqttCert, string()},
                    {configuration::MqttClientId, string()},
                    {configuration::MqttUserName, string()},
                    {configuration::MqttPassword, string()}});
        AddDefaultedOptions(
            config, m_options,
            {{configuration::MqttHost, "127.0.0.1"s},
             {configuration::DeviceTopic, "MTConnect/Probe/[device]"s},
             {configuration::AssetTopic, "MTConnect/Asset/[device]"s},
             {configuration::MqttLastWillTopic, "MTConnect/Probe/[device]/Availability"s},
             {configuration::CurrentTopic, "MTConnect/Current/[device]"s},
             {configuration::SampleTopic, "MTConnect/Sample/[device]"s},
             {configuration::MqttCurrentInterval, 10000ms},
             {configuration::MqttSampleInterval, 500ms},
             {configuration::MqttSampleCount, 1000},
             {configuration::MqttPort, 1883},
             {configuration::MqttTls, false}});

        int maxTopicDepth {GetOption<int>(options, configuration::MqttMaxTopicDepth).value_or(7)};

        m_deviceTopic = GetOption<string>(m_options, configuration::ProbeTopic)
                            .value_or(get<string>(m_options[configuration::DeviceTopic]));
        m_assetTopic = getTopic(configuration::AssetTopic, maxTopicDepth);
        m_currentTopic = getTopic(configuration::CurrentTopic, maxTopicDepth);
        m_sampleTopic = getTopic(configuration::SampleTopic, maxTopicDepth);

        m_currentInterval = *GetOption<Milliseconds>(m_options, configuration::MqttCurrentInterval);
        m_sampleInterval = *GetOption<Milliseconds>(m_options, configuration::MqttSampleInterval);

        m_sampleCount = *GetOption<int>(m_options, configuration::MqttSampleCount);
      }

      void MqttService::start()
      {
        if (!m_client)
        {
          auto clientHandler = make_unique<ClientHandler>();
          clientHandler->m_connected = [this](shared_ptr<MqttClient> client) {
            // Publish latest devices, assets, and observations
            auto &circ = m_sinkContract->getCircularBuffer();
            std::lock_guard<buffer::CircularBuffer> lock(circ);
            client->connectComplete();

            client->publish(m_lastWillTopic, "AVAILABLE");
            pubishInitialContent();
          };

          auto agentDevice = m_sinkContract->getDeviceByName("Agent");
          auto lwtTopic = get<string>(m_options[configuration::MqttLastWillTopic]);
          m_lastWillTopic = formatTopic(lwtTopic, agentDevice, "Agent");

          if (IsOptionSet(m_options, configuration::MqttTls))
          {
            m_client = make_shared<MqttTlsClient>(m_context, m_options, std::move(clientHandler),
                                                  m_lastWillTopic, "UNAVAILABLE"s);
          }
          else
          {
            m_client = make_shared<MqttTcpClient>(m_context, m_options, std::move(clientHandler),
                                                  m_lastWillTopic, "UNAVAILABLE"s);
          }
        }
        m_client->start();
      }

      void MqttService::stop()
      {
        // stop client side
        if (m_client)
          m_client->stop();

        m_currentTimer.cancel();
      }

      struct AsyncSample : public observation::AsyncObserver
      {
        AsyncSample(boost::asio::io_context::strand &strand,
                    mtconnect::buffer::CircularBuffer &buffer, FilterSet &&filter,
                    std::chrono::milliseconds interval, std::chrono::milliseconds heartbeat,
                    std::shared_ptr<MqttClient> client, DevicePtr device)
          : observation::AsyncObserver(strand, buffer, std::move(filter), interval, heartbeat),
            m_device(device),
            m_client(client)
        {}

        void fail(boost::beast::http::status status, const std::string &message) override
        {
          LOG(error) << "MQTT Sample Failed: " << message;
        }

        bool isRunning() override
        {
          if (m_sink.expired())
            return false;

          auto client = m_client.lock();
          return client && client->isRunning() && client->isConnected();
        }

        bool cancel() override { return true; }

        DevicePtr m_device;
        std::weak_ptr<MqttClient> m_client;
        std::weak_ptr<sink::Sink>
            m_sink;  //!  weak shared pointer to the sink. handles shutdown timer race
      };

      void MqttService::pubishInitialContent()
      {
        using std::placeholders::_1;
        for (auto &dev : m_sinkContract->getDevices())
        {
          publish(dev);

          AssetList list;
          m_sinkContract->getAssetStorage()->getAssets(list, 100000, true, *(dev->getUuid()));
          for (auto &asset : list)
          {
            publish(asset);
          }
        }

        auto seq = publishCurrent(boost::system::error_code {});
        for (auto &dev : m_sinkContract->getDevices())
        {
          FilterSet filterSet = filterForDevice(dev);
          auto sampler =
              make_shared<AsyncSample>(m_strand, m_sinkContract->getCircularBuffer(),
                                       std::move(filterSet), m_sampleInterval, 600s, m_client, dev);
          sampler->m_sink = getptr();
          sampler->m_handler = boost::bind(&MqttService::publishSample, this, _1);
          sampler->observe(seq, [this](const std::string &id) {
            return m_sinkContract->getDataItemById(id).get();
          });
          publishSample(sampler);
        }
      }

      /// @brief publish sample when observations arrive.
      SequenceNumber_t MqttService::publishSample(
          std::shared_ptr<observation::AsyncObserver> observer)
      {
        auto sampler = std::dynamic_pointer_cast<AsyncSample>(observer);
        auto topic = formatTopic(m_sampleTopic, sampler->m_device);
        LOG(debug) << "Publishing sample for: " << topic;

        std::unique_ptr<observation::ObservationList> observations;
        SequenceNumber_t end {0};
        std::string doc;
        SequenceNumber_t firstSeq, lastSeq;

        {
          auto &buffer = m_sinkContract->getCircularBuffer();
          std::lock_guard<buffer::CircularBuffer> lock(buffer);

          lastSeq = buffer.getSequence() - 1;
          observations =
              buffer.getObservations(m_sampleCount, sampler->getFilter(), sampler->getSequence(),
                                     nullopt, end, firstSeq, observer->m_endOfBuffer);
        }

        doc = m_printer->printSample(m_instanceId,
                                     m_sinkContract->getCircularBuffer().getBufferSize(), end,
                                     firstSeq, lastSeq, *observations, false);

        m_client->asyncPublish(topic, doc, [sampler, topic](std::error_code ec) {
          if (!ec)
          {
            sampler->handlerCompleted();
          }
          else
          {
            LOG(warning) << "Async publish failed for " << topic << ": " << ec.message();
          }
        });

        return end;
      }

      SequenceNumber_t MqttService::publishCurrent(boost::system::error_code ec)
      {
        SequenceNumber_t firstSeq, seq = 0;

        if (ec)
        {
          LOG(warning) << "Mqtt2Service::publishCurrent: " << ec.message();
          return 0;
        }

        if (!m_client->isRunning() || !m_client->isConnected())
        {
          LOG(warning) << "Mqtt2Service::publishCurrent: client stopped";
          return 0;
        }

        for (auto &device : m_sinkContract->getDevices())
        {
          auto topic = formatTopic(m_currentTopic, device);
          LOG(debug) << "Publishing current for: " << topic;

          ObservationList observations;
          auto filterSet = filterForDevice(device);

          {
            auto &buffer = m_sinkContract->getCircularBuffer();
            std::lock_guard<buffer::CircularBuffer> lock(buffer);

            firstSeq = buffer.getFirstSequence();
            seq = buffer.getSequence();
            m_sinkContract->getCircularBuffer().getLatest().getObservations(observations,
                                                                            filterSet);
          }

          auto doc = m_printer->printSample(m_instanceId,
                                            m_sinkContract->getCircularBuffer().getBufferSize(),
                                            seq, firstSeq, seq - 1, observations);

          m_client->publish(topic, doc);
        }

        using std::placeholders::_1;
        m_currentTimer.expires_after(m_currentInterval);
        m_currentTimer.async_wait(boost::asio::bind_executor(
            m_strand, boost::bind(&MqttService::publishCurrent, this, _1)));

        return seq;
      }

      bool MqttService::publish(observation::ObservationPtr &observation)
      {
        // Since we are doing periodic publishing, there is nothing to do here.
        return true;
      }

      bool MqttService::publish(device_model::DevicePtr device)
      {
        m_filters.clear();

        auto topic = formatTopic(m_deviceTopic, device);
        auto doc = m_jsonPrinter->print(device);

        stringstream buffer;
        buffer << doc;

        if (m_client)
          m_client->publish(topic, buffer.str());

        return true;
      }

      bool MqttService::publish(asset::AssetPtr asset)
      {
        auto uuid = asset->getDeviceUuid();
        DevicePtr dev;
        if (uuid)
          dev = m_sinkContract->findDeviceByUUIDorName(*uuid);
        auto topic = formatTopic(m_assetTopic, dev);
        if (topic.back() != '/')
          topic.append("/");
        topic.append(asset->getAssetId());

        LOG(debug) << "Publishing Asset to topic: " << topic;

        asset::AssetList list {asset};
        auto doc = m_printer->printAssets(
            m_instanceId, uint32_t(m_sinkContract->getAssetStorage()->getMaxAssets()), 1, list);
        stringstream buffer;
        buffer << doc;

        if (m_client)
          m_client->publish(topic, buffer.str());

        return true;
      }

      // Register the service with the sink factory
      void MqttService::registerFactory(SinkFactory &factory)
      {
        factory.registerFactory(
            "MqttService",
            [](const std::string &name, boost::asio::io_context &io, SinkContractPtr &&contract,
               const ConfigOptions &options, const boost::property_tree::ptree &block) -> SinkPtr {
              auto sink = std::make_shared<MqttService>(io, std::move(contract), options, block);
              return sink;
            });
      }
    }  // namespace mqtt_sink
  }    // namespace sink
}  // namespace mtconnect
