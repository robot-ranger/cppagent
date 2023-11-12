//
// Copyright Copyright 2009-2022, AMT – The Association For Manufacturing Technology (“AMT”)
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

// Ensure that gtest is the first header otherwise Windows raises an error
#include <gtest/gtest.h>
// Keep this comment to keep gtest.h above. (clang-format off/on is not working here!)

#include <chrono>

#include "mtconnect/device_model/device.hpp"
#include "mtconnect/observation/observation.hpp"
#include "mtconnect/pipeline/json_mapper.hpp"
#include "mtconnect/pipeline/pipeline_context.hpp"

using namespace mtconnect;
using namespace mtconnect::pipeline;
using namespace mtconnect::observation;
using namespace mtconnect::asset;
using namespace device_model;
using namespace data_item;
using namespace std;

// main
int main(int argc, char *argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

class MockPipelineContract : public PipelineContract
{
public:
  MockPipelineContract(std::map<string, DataItemPtr> &items, std::map<string, DevicePtr> &devices)
    : m_dataItems(items), m_devices(devices)
  {}
  DevicePtr findDevice(const std::string &name) override { return m_devices[name]; }
  DataItemPtr findDataItem(const std::string &device, const std::string &name) override
  {
    return m_dataItems[name];
  }
  void eachDataItem(EachDataItem fun) override {}
  void deliverObservation(observation::ObservationPtr obs) override {}
  void deliverAsset(AssetPtr) override {}
  void deliverDevices(std::list<DevicePtr>) override {}
  void deliverAssetCommand(entity::EntityPtr) override {}
  void deliverCommand(entity::EntityPtr) override {}
  void deliverConnectStatus(entity::EntityPtr, const StringList &, bool) override {}
  void sourceFailed(const std::string &id) override {}
  const ObservationPtr checkDuplicate(const ObservationPtr &obs) const override { return obs; }

  std::map<string, DataItemPtr> &m_dataItems;
  std::map<string, DevicePtr> &m_devices;
};

class JsonMappingTest : public testing::Test
{
protected:
  void SetUp() override
  {
    m_context = make_shared<PipelineContext>();
    m_context->m_contract = make_unique<MockPipelineContract>(m_dataItems, m_devices);
    m_mapper = make_shared<JsonMapper>(m_context);
    m_mapper->bind(make_shared<NullTransform>(TypeGuard<Entity>(RUN)));
  }

  void TearDown() override
  {
    m_dataItems.clear();
    m_devices.clear();
  }

  DataItemPtr makeDataItem(const std::string &device, const Properties &props)
  {
    auto dev = m_devices.find(device);
    if (dev == m_devices.end())
    {
      EXPECT_TRUE(false) << "Cannot find device: " << device;
      return nullptr;
    }

    Properties ps(props);
    ErrorList errors;
    auto di = DataItem::make(ps, errors);
    m_dataItems.emplace(di->getId(), di);

    dev->second->addDataItem(di, errors);

    return di;
  }

  DevicePtr makeDevice(const std::string &name, const Properties &props)
  {
    ErrorList errors;
    Properties ps(props);
    DevicePtr d = dynamic_pointer_cast<device_model::Device>(
        device_model::Device::getFactory()->make("Device", ps, errors));
    m_devices.emplace(d->getId(), d);

    return d;
  }

  shared_ptr<PipelineContext> m_context;
  shared_ptr<JsonMapper> m_mapper;
  std::map<string, DataItemPtr> m_dataItems;
  std::map<string, DevicePtr> m_devices;
};

inline DataSetEntry operator"" _E(const char *c, std::size_t) { return DataSetEntry(c); }
using namespace date::literals;

/// @test verify the json mapper can map an object with a timestamp and a series of observations
TEST_F(JsonMappingTest, should_parse_simple_observations)
{
  auto dev = makeDevice("Device", {{"id", "device"s}, {"name", "device"s}, {"uuid", "device"s}});
  makeDataItem("device", {{"id", "a"s}, {"type", "EXECUTION"s}, {"category", "EVENT"s}});
  makeDataItem("device", {{"id", "b"s}, {"type", "POSITION"s}, {"category", "SAMPLE"s}});

  Properties props {{"VALUE", R"(
{
  "timestamp": "2023-11-09T11:20:00Z",
  "a": "ACTIVE",
  "b": 123.456
}
)"s}};

  auto msg = std::make_shared<JsonMessage>("JsonMessage", props);
  msg->m_device = dev;

  auto res = (*m_mapper)(std::move(msg));
  ASSERT_TRUE(res);

  auto value = res->getValue();
  ASSERT_TRUE(std::holds_alternative<EntityList>(value));
  auto list = get<EntityList>(value);
  ASSERT_EQ(2, list.size());
  
  auto it = list.begin();
  
  auto obs = dynamic_pointer_cast<Observation>(*it);
  ASSERT_TRUE(obs);
  ASSERT_EQ("Execution", obs->getName());

  auto time = Timestamp(date::sys_days(2023_y / nov / 9_d)) + 11h + 20min;

  ASSERT_EQ(time, obs->getTimestamp());
  ASSERT_EQ("ACTIVE", obs->getValue<string>());
  it++;
  
  obs = dynamic_pointer_cast<Observation>(*it);
  ASSERT_TRUE(obs);
  ASSERT_EQ("Position", obs->getName());

  ASSERT_EQ(time, obs->getTimestamp());
  ASSERT_EQ(123.456, obs->getValue<double>());
}

/// @test verify the json mapper can map an object with a timestamp to a condition and message
TEST_F(JsonMappingTest, should_parse_conditions_and_messages) { GTEST_SKIP(); }

/// @test verify the json mapper can map an array of objects with a timestamps and a series of
/// observations
TEST_F(JsonMappingTest, should_parse_an_array_of_objects) { GTEST_SKIP(); }

/// @test verify the json mapper recognizes the device key
TEST_F(JsonMappingTest, should_parse_to_multiple_devices_with_device_key) { GTEST_SKIP(); }

/// @test verify the json mapper recognizes the device key and data item key
TEST_F(JsonMappingTest, should_parse_to_device_and_data_item_when_keys_are_supplied)
{
  GTEST_SKIP();
}

/// @test verify the json mapper defaults the timestamp when it is not given in the object
TEST_F(JsonMappingTest, should_default_the_time_to_now_when_not_given) { GTEST_SKIP(); }

/// @test verify the json mapper should be able to handle data sets and tables
TEST_F(JsonMappingTest, should_parse_data_sets_and_tables) { GTEST_SKIP(); }
