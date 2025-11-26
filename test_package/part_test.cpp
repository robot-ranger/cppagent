//
// Copyright Copyright 2009-2025, AMT – The Association For Manufacturing Technology (“AMT”)
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

#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "agent_test_helper.hpp"
#include "json_helper.hpp"
#include "mtconnect/agent.hpp"
#include "mtconnect/asset/asset.hpp"
#include "mtconnect/asset/part.hpp"
#include "mtconnect/entity/xml_parser.hpp"
#include "mtconnect/entity/xml_printer.hpp"
#include "mtconnect/printer//xml_printer_helper.hpp"
#include "mtconnect/source/adapter/adapter.hpp"

using json = nlohmann::json;
using namespace std;
using namespace mtconnect;
using namespace mtconnect::entity;
using namespace mtconnect::source::adapter;
using namespace mtconnect::asset;
using namespace mtconnect::printer;

// main
int main(int argc, char *argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

class PartAssetTest : public testing::Test
{
protected:
  void SetUp() override
  {  // Create an agent with only 16 slots and 8 data items.
    Part::registerAsset();
    PartArchetype::registerAsset();
    m_writer = make_unique<XmlWriter>(true);
  }

  void TearDown() override { m_writer.reset(); }

  std::unique_ptr<XmlWriter> m_writer;
  std::unique_ptr<AgentTestHelper> m_agentTestHelper;
};

TEST_F(PartAssetTest, should_parse_a_part_archetype)
{
  const auto doc =
      R"DOC(<PartArchetype assetId="PART1234" drawing="STEP222" family="HHH" revision="5">
  <Configuration>
    <Relationships>
      <AssetRelationship assetIdRef="MATERIAL" assetType="RawMaterial" id="A" type="PEER"/>
      <AssetRelationship assetIdRef="PROCESS" assetType="ProcessArchetype" id="B" type="PEER"/>
    </Relationships>
  </Configuration>
  <Customers>
    <Customer customerId="C00241" name="customer name">
      <Address>100 Fruitstand Rd, Ork Arkansas, 11111</Address>
      <Description>Some customer</Description>
    </Customer>
  </Customers>
</PartArchetype>
)DOC";

  ErrorList errors;
  entity::XmlParser parser;

  auto entity = parser.parse(Asset::getRoot(), doc, errors);
  ASSERT_EQ(0, errors.size());

  auto asset = dynamic_cast<Asset *>(entity.get());
  ASSERT_NE(nullptr, asset);

  ASSERT_EQ("PartArchetype", asset->getName());
  ASSERT_EQ("PART1234", asset->getAssetId());
  ASSERT_EQ("5", asset->get<string>("revision"));
  ASSERT_EQ("STEP222", asset->get<string>("drawing"));
  ASSERT_EQ("HHH", asset->get<string>("family"));

  auto configuration = asset->get<EntityPtr>("Configuration");
  ASSERT_TRUE(configuration);
  
  auto relationships = configuration->getList("Relationships");
  ASSERT_TRUE(relationships);
  ASSERT_EQ(2, relationships->size());
  
  {
    auto it = relationships->begin();
    ASSERT_EQ("A", (*it)->get<string>("id"));
    ASSERT_EQ("MATERIAL", (*it)->get<string>("assetIdRef"));
    ASSERT_EQ("PEER", (*it)->get<string>("type"));
    ASSERT_EQ("RawMaterial", (*it)->get<string>("assetType"));
    
    it++;
    ASSERT_EQ("B", (*it)->get<string>("id"));
    ASSERT_EQ("PROCESS", (*it)->get<string>("assetIdRef"));
    ASSERT_EQ("PEER", (*it)->get<string>("type"));
    ASSERT_EQ("ProcessArchetype", (*it)->get<string>("assetType"));
  }
  
  auto customers = asset->getList("Customers");
  ASSERT_TRUE(customers);
  ASSERT_EQ(1, customers->size());
  
  {
    auto customer = customers->front();
    ASSERT_EQ("C00241", customer->get<string>("customerId"));
    ASSERT_EQ("customer name", customer->get<string>("name"));
    ASSERT_EQ("100 Fruitstand Rd, Ork Arkansas, 11111", customer->get<string>("Address"));
    ASSERT_EQ("Some customer", customer->get<string>("Description"));
  }
  
  // Round trip test
  entity::XmlPrinter printer;
  printer.print(*m_writer, entity, {});

  string content = m_writer->getContent();
  ASSERT_EQ(content, doc);
}


TEST_F(PartAssetTest, process_archetype_can_have_multiple_customers)
{
  const auto doc =
      R"DOC(<PartArchetype assetId="PART1234" drawing="STEP222" family="HHH" revision="5">
  <Customers>
    <Customer customerId="C00241" name="customer name">
      <Address>100 Fruitstand Rd, Ork Arkansas, 11111</Address>
      <Description>Some customer</Description>
    </Customer>
    <Customer customerId="C1111" name="another customer">
      <Address>Somewhere in Austrailia</Address>
      <Description>Another customer</Description>
    </Customer>
  </Customers>
</PartArchetype>
)DOC";

  ErrorList errors;
  entity::XmlParser parser;
  
  auto entity = parser.parse(Asset::getRoot(), doc, errors);
  ASSERT_EQ(0, errors.size());
  
  auto asset = dynamic_cast<Asset *>(entity.get());
  ASSERT_NE(nullptr, asset);
  ASSERT_EQ("PartArchetype", asset->getName());

  auto customers = asset->getList("Customers");
  ASSERT_TRUE(customers);
  ASSERT_EQ(2, customers->size());
  
  {
    auto it = customers->begin();
    auto customer = *it;
    EXPECT_EQ("C00241", customer->get<string>("customerId"));
    EXPECT_EQ("customer name", customer->get<string>("name"));
    EXPECT_EQ("100 Fruitstand Rd, Ork Arkansas, 11111", customer->get<string>("Address"));
    EXPECT_EQ("Some customer", customer->get<string>("Description"));
    
    it++;
    customer = *it;
    EXPECT_EQ("C1111", customer->get<string>("customerId"));
    EXPECT_EQ("another customer", customer->get<string>("name"));
    EXPECT_EQ("Somewhere in Austrailia", customer->get<string>("Address"));
    EXPECT_EQ("Another customer", customer->get<string>("Description"));
  }

  // Round trip test
  entity::XmlPrinter printer;
  printer.print(*m_writer, entity, {});
  
  string content = m_writer->getContent();
  ASSERT_EQ(content, doc);
}

/*
TEST_F(PartAssetTest, process_steps_can_be_optional)
{
  const auto doc =
      R"DOC(<ProcessArchetype assetId="PROCESS_ARCH_ID" revision="1">
  <Routings>
    <Routing precedence="1" routingId="routng1">
      <ProcessStep optional="true" sequence="5" stepId="10">
        <Description>Process Step 10</Description>
        <StartTime>2025-11-24T00:00:00Z</StartTime>
        <Duration>23000</Duration>
      </ProcessStep>
    </Routing>
  </Routings>
</ProcessArchetype>
)DOC";
    
  ErrorList errors;
  entity::XmlParser parser;
  
  auto entity = parser.parse(Asset::getRoot(), doc, errors);
  ASSERT_EQ(0, errors.size());
  
  auto asset = dynamic_cast<Asset *>(entity.get());
  ASSERT_NE(nullptr, asset);
  
  auto routings = asset->getList("Routings");
  ASSERT_TRUE(routings);
  ASSERT_EQ(1, routings->size());
  
  auto routing = routings->front();
  ASSERT_EQ("routng1", routing->get<string>("routingId"));
  
  auto processSteps = routing->get<EntityList>("ProcessStep");
  ASSERT_EQ(1, processSteps.size());
  auto step = processSteps.front();
  
  ASSERT_EQ("10", step->get<string>("stepId"));
  ASSERT_EQ(5, step->get<int64_t>("sequence"));
  ASSERT_EQ(true, step->get<bool>("optional"));

  // Round trip test
  entity::XmlPrinter printer;
  printer.print(*m_writer, entity, {});
  
  string content = m_writer->getContent();
  ASSERT_EQ(content, doc);
}

TEST_F(PartAssetTest, process_archetype_must_have_at_least_one_routing)
{
  const auto doc =
      R"DOC(<ProcessArchetype assetId="PROCESS_ARCH_ID" revision="1">
  <Targets>
    <TargetDevice targetId="device1"/>
    <TargetGroup groupId="group1">
      <TargetDevice targetId="device2"/>
      <TargetDevice targetId="device3"/>
    </TargetGroup>
  </Targets>
</ProcessArchetype>
)DOC";
  
  ErrorList errors;
  entity::XmlParser parser;
  
  auto entity = parser.parse(Asset::getRoot(), doc, errors);
  ASSERT_EQ(1, errors.size());
  auto error = dynamic_cast<PropertyError*>(errors.front().get());
  
  ASSERT_EQ("ProcessArchetype(Routings): Property Routings is required and not provided"s, error->what());
  ASSERT_EQ("ProcessArchetype", error->getEntity());
  ASSERT_EQ("Routings", error->getProperty());
}

TEST_F(PartAssetTest, process_archetype_routing_must_have_a_process_step)
{
  const auto doc =
      R"DOC(<ProcessArchetype assetId="PROCESS_ARCH_ID" revision="1">
  <Routings>
    <Routing precedence="1" routingId="routng1">
    </Routing>
  </Routings>
  <Targets>
    <TargetDevice targetId="device1"/>
    <TargetGroup groupId="group1">
      <TargetDevice targetId="device2"/>
      <TargetDevice targetId="device3"/>
    </TargetGroup>
  </Targets>
</ProcessArchetype>
)DOC";
  
  ErrorList errors;
  entity::XmlParser parser;
  
  auto entity = parser.parse(Asset::getRoot(), doc, errors);
  ASSERT_EQ(5, errors.size());
  
  auto it = errors.begin();
  {
    auto error = dynamic_cast<PropertyError*>(it->get());
    ASSERT_TRUE(error);
    EXPECT_EQ("Routing(ProcessStep): Property ProcessStep is required and not provided"s, error->what());
    EXPECT_EQ("Routing", error->getEntity());
    EXPECT_EQ("ProcessStep", error->getProperty());
  }
  
  it++;
  {
    auto error = it->get();
    ASSERT_TRUE(error);
    EXPECT_EQ("Routings: Invalid element 'Routing'"s, error->what());
    EXPECT_EQ("Routings", error->getEntity());
  }
  
  it++;
  {
    auto error = dynamic_cast<PropertyError*>(it->get());
    ASSERT_TRUE(error);
    EXPECT_EQ("Routings(Routing): Entity list requirement Routing must have at least 1 entries, 0 found"s, error->what());
    EXPECT_EQ("Routings", error->getEntity());
    EXPECT_EQ("Routing", error->getProperty());
  }

  it++;
  {
    auto error = it->get();
    ASSERT_TRUE(error);
    EXPECT_EQ("ProcessArchetype: Invalid element 'Routings'"s, error->what());
    EXPECT_EQ("ProcessArchetype", error->getEntity());
  }

  it++;
  {
    auto error = dynamic_cast<PropertyError*>(it->get());
    ASSERT_TRUE(error);
    EXPECT_EQ("ProcessArchetype(Routings): Property Routings is required and not provided"s, error->what());
    EXPECT_EQ("ProcessArchetype", error->getEntity());
    EXPECT_EQ("Routings", error->getProperty());
  }
  
}

TEST_F(PartAssetTest, activity_can_have_a_sequence_precidence_and_be_options)
{
  const auto doc =
      R"DOC(<ProcessArchetype assetId="PROCESS_ARCH_ID" revision="1">
  <Routings>
    <Routing precedence="1" routingId="routng1">
      <ProcessStep optional="true" sequence="5" stepId="10">
        <ActivityGroups>
          <ActivityGroup activityGroupId="act1" name="fred">
            <Activity activityId="a1" optional="true" precedence="3" sequence="2">
              <Description>First Activity</Description>
            </Activity>
          </ActivityGroup>
        </ActivityGroups>
      </ProcessStep>
    </Routing>
  </Routings>
</ProcessArchetype>
)DOC";

  ErrorList errors;
  entity::XmlParser parser;
  
  auto entity = parser.parse(Asset::getRoot(), doc, errors);
  ASSERT_EQ(0, errors.size());
  
  auto asset = dynamic_cast<Asset *>(entity.get());
  ASSERT_NE(nullptr, asset);
  
  auto routings = asset->getList("Routings");
  ASSERT_TRUE(routings);
  ASSERT_EQ(1, routings->size());
  
  auto routing = routings->front();
  ASSERT_EQ("routng1", routing->get<string>("routingId"));
  
  auto processSteps = routing->get<EntityList>("ProcessStep");
  ASSERT_EQ(1, processSteps.size());
  auto step = processSteps.front();

  auto activityGroups = step->getList("ActivityGroups");
  ASSERT_TRUE(activityGroups);
  ASSERT_EQ(1, activityGroups->size());
  
  auto activityGroup = activityGroups->front();
  ASSERT_EQ("act1", activityGroup->get<string>("activityGroupId"));
  ASSERT_EQ("fred", activityGroup->get<string>("name"));
  
  auto activities = activityGroup->get<EntityList>("Activity");
  ASSERT_EQ(1, activities.size());
  
  auto activity = activities.front();
  ASSERT_EQ("a1", activity->get<string>("activityId"));
  ASSERT_EQ(2, activity->get<int64_t>("sequence"));
  ASSERT_EQ("First Activity", activity->get<string>("Description"));
  ASSERT_EQ(true, activity->get<bool>("optional"));
  ASSERT_EQ(3, activity->get<int64_t>("precedence"));

  // Round trip test
  entity::XmlPrinter printer;
  printer.print(*m_writer, entity, {});
  
  string content = m_writer->getContent();
  ASSERT_EQ(content, doc);
}

TEST_F(PartAssetTest, should_generate_json)
{
  const auto doc =
      R"DOC(<ProcessArchetype assetId="PROCESS_ARCH_ID" revision="1">
  <Configuration>
    <Relationships>
      <AssetRelationship assetIdRef="PART_ID" assetType="PART_ARCHETYPE" id="reference_id" type="PEER"/>
    </Relationships>
  </Configuration>
  <Routings>
    <Routing precedence="1" routingId="routng1">
      <ProcessStep stepId="10">
        <Description>Process Step 10</Description>
        <StartTime>2025-11-24T00:00:00Z</StartTime>
        <Duration>23000</Duration>
        <Targets>
          <TargetRef groupIdRef="group1"/>
        </Targets>
        <ActivityGroups>
          <ActivityGroup activityGroupId="act1" name="fred">
            <Activity activityId="a1" sequence="1" optional="true" precedence="2">
              <Description>First Activity</Description>
            </Activity>
          </ActivityGroup>
        </ActivityGroups>
      </ProcessStep>
    </Routing>
  </Routings>
  <Targets>
    <TargetDevice targetId="device1"/>
    <TargetGroup groupId="group1">
      <TargetDevice targetId="device2"/>
      <TargetDevice targetId="device3"/>
    </TargetGroup>
  </Targets>
</ProcessArchetype>
)DOC";
  
  ErrorList errors;
  entity::XmlParser parser;
  
  auto entity = parser.parse(Asset::getRoot(), doc, errors);
  ASSERT_EQ(0, errors.size());
  
  entity::JsonEntityPrinter jprinter(2, true);
  
  auto sdoc = jprinter.print(entity);
  EXPECT_EQ(R"({
  "ProcessArchetype": {
    "Configuration": {
      "Relationships": {
        "AssetRelationship": [
          {
            "assetIdRef": "PART_ID",
            "assetType": "PART_ARCHETYPE",
            "id": "reference_id",
            "type": "PEER"
          }
        ]
      }
    },
    "Routings": {
      "Routing": [
        {
          "ProcessStep": [
            {
              "ActivityGroups": {
                "ActivityGroup": [
                  {
                    "Activity": [
                      {
                        "Description": "First Activity",
                        "activityId": "a1",
                        "optional": true,
                        "precedence": 2,
                        "sequence": 1
                      }
                    ],
                    "activityGroupId": "act1",
                    "name": "fred"
                  }
                ]
              },
              "Description": "Process Step 10",
              "Duration": 23000.0,
              "StartTime": "2025-11-24T00:00:00Z",
              "Targets": {
                "TargetRef": [
                  {
                    "groupIdRef": "group1"
                  }
                ]
              },
              "stepId": "10"
            }
          ],
          "precedence": 1,
          "routingId": "routng1"
        }
      ]
    },
    "Targets": {
      "TargetDevice": [
        {
          "targetId": "device1"
        }
      ],
      "TargetGroup": [
        {
          "TargetDevice": [
            {
              "targetId": "device2"
            },
            {
              "targetId": "device3"
            }
          ],
          "groupId": "group1"
        }
      ]
    },
    "assetId": "PROCESS_ARCH_ID",
    "revision": "1"
  }
})", sdoc);
}

TEST_F(PartAssetTest, should_parse_and_generate_a_process)
{
  const auto doc =
      R"DOC(<Process assetId="PROCESS_ARCH_ID" revision="1">
  <Configuration>
    <Relationships>
      <AssetRelationship assetIdRef="PART_ID" assetType="PART_ARCHETYPE" id="reference_id" type="PEER"/>
    </Relationships>
  </Configuration>
  <Routings>
    <Routing precedence="1" routingId="routng1">
      <ProcessStep stepId="10">
        <Description>Process Step 10</Description>
        <StartTime>2025-11-24T00:00:00Z</StartTime>
        <Duration>23000</Duration>
        <Targets>
          <TargetRef groupIdRef="group1"/>
        </Targets>
        <ActivityGroups>
          <ActivityGroup activityGroupId="act1">
            <Activity activityId="a1" sequence="1">
              <Description>First Activity</Description>
            </Activity>
          </ActivityGroup>
        </ActivityGroups>
      </ProcessStep>
    </Routing>
  </Routings>
  <Targets>
    <TargetDevice targetId="device1"/>
    <TargetGroup groupId="group1">
      <TargetDevice targetId="device2"/>
      <TargetDevice targetId="device3"/>
    </TargetGroup>
  </Targets>
</Process>
)DOC";
  
  ErrorList errors;
  entity::XmlParser parser;
  
  auto entity = parser.parse(Asset::getRoot(), doc, errors);
  ASSERT_EQ(0, errors.size());
  
  // Round trip test
  entity::XmlPrinter printer;
  printer.print(*m_writer, entity, {});
  
  string content = m_writer->getContent();
  ASSERT_EQ(content, doc);
}

TEST_F(PartAssetTest, process_can_only_have_one_routings)
{
  const auto doc =
      R"DOC(<Process assetId="PROCESS_ARCH_ID" revision="1">
  <Configuration>
    <Relationships>
      <AssetRelationship assetIdRef="PART_ID" assetType="PART_ARCHETYPE" id="reference_id" type="PEER"/>
    </Relationships>
  </Configuration>
  <Routings>
    <Routing precedence="1" routingId="routng1">
      <ProcessStep stepId="10">
        <Description>Process Step 10</Description>
        <StartTime>2025-11-24T00:00:00Z</StartTime>
        <Duration>23000</Duration>
      </ProcessStep>
    </Routing>
    <Routing precedence="2" routingId="routng2">
      <ProcessStep stepId="11">
        <Description>Process Step 11</Description>
        <StartTime>2025-11-25T00:00:00Z</StartTime>
        <Duration>20000</Duration>
      </ProcessStep>
    </Routing>
  </Routings>
  <Targets>
    <TargetDevice targetId="device1"/>
    <TargetGroup groupId="group1">
      <TargetDevice targetId="device2"/>
      <TargetDevice targetId="device3"/>
    </TargetGroup>
  </Targets>
</Process>
)DOC";
  
  ErrorList errors;
  entity::XmlParser parser;
  
  auto entity = parser.parse(Asset::getRoot(), doc, errors);
  ASSERT_EQ(3, errors.size());
  
  auto it = errors.begin();
  {
    auto error = dynamic_cast<PropertyError*>(it->get());
    ASSERT_TRUE(error);
    EXPECT_EQ("Routings(Routing): Entity list requirement Routing must have at least 1 and no more than 1 entries, 2 found"s, error->what());
    EXPECT_EQ("Routings", error->getEntity());
    EXPECT_EQ("Routing", error->getProperty());
  }
  
  it++;
  {
    auto error = it->get();
    ASSERT_TRUE(error);
    EXPECT_EQ("Process: Invalid element 'Routings'"s, error->what());
    EXPECT_EQ("Process", error->getEntity());
  }

  it++;
  {
    auto error = dynamic_cast<PropertyError*>(it->get());
    ASSERT_TRUE(error);
    EXPECT_EQ("Process(Routings): Property Routings is required and not provided"s, error->what());
    EXPECT_EQ("Process", error->getEntity());
    EXPECT_EQ("Routings", error->getProperty());
  }
}
*/
