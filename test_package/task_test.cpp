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
#include "mtconnect/asset/task.hpp"
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

class TaskAssetTest : public testing::Test
{
protected:
  void SetUp() override
  {  // Create an agent with only 16 slots and 8 data items.
    Task::registerAsset();
    TaskArchetype::registerAsset();
    m_writer = make_unique<XmlWriter>(true);
  }

  void TearDown() override { m_writer.reset(); }

  std::unique_ptr<XmlWriter> m_writer;
  std::unique_ptr<AgentTestHelper> m_agentTestHelper;
};

/// @section PartArchetype tests

TEST_F(TaskAssetTest, should_parse_a_part_archetype)
{
  const auto doc =
      R"DOC(<TaskArchetype assetId="1aa7eece248093" deviceUuid="mxi_m001" hash="fCI1rCQv8BcHbzZeoMxt3kHmb9k=" timestamp="2024-12-10T05:17:05.531454Z">
  <TaskType>MATERIAL_UNLOAD</TaskType>
  <Priority>10</Priority>
  <Targets>
    <TargetDevice deviceUuid="Mazak123"/>
    <TargetDevice deviceUuid="Mazak456"/>
    <TargetGroup groupId="MyRobots">
      <TargetDevice deviceUuid="UR123"/>
      <TargetDevice deviceUuid="UR456"/>
    </TargetGroup>
  </Targets>
  <Coordinator>
    <Collaborator collaboratorId="machine" type="CNC">
      <Targets>
        <TargetDevice deviceUuid="Mazak123"/>
        <TargetDevice deviceUuid="Mazak456"/>
      </Targets>
    </Collaborator>
  </Coordinator>
  <Collaborators>
    <Collaborator collaboratorId="Robot" type="ROBOT">
      <Targets>
        <TargetRequirement requirementId="ab">
          <CapabilityTable>
            <Entry key="PAYLOAD">
              <Cell key="maximum">1000</Cell>
            </Entry>
            <Entry key="REACH">
              <Cell key="minimum">1500</Cell>
            </Entry>
          </CapabilityTable>
        </TargetRequirement>
        <TargetRef groupIdRef="MyRobots"/>
      </Targets>
    </Collaborator>
    <Collaborator collaboratorId="robot2" type="ROBOT">
      <Targets>
        <TargetDevice deviceUuid="UR890"/>
      </Targets>
    </Collaborator>
  </Collaborators>
  <SubTaskRefs>
    <SubTaskRef order="1">UnloadConv</SubTaskRef>
    <SubTaskRef order="2">LoadCnc</SubTaskRef>
  </SubTaskRefs>
</TaskArchetype>
)DOC";

  ErrorList errors;
  entity::XmlParser parser;

  auto entity = parser.parse(Asset::getRoot(), doc, errors);
  ASSERT_EQ(0, errors.size());

  auto asset = dynamic_cast<Asset *>(entity.get());
  ASSERT_NE(nullptr, asset);

  EXPECT_EQ("TaskArchetype", asset->getName());
  EXPECT_EQ("1aa7eece248093", asset->getAssetId());
  EXPECT_EQ("mxi_m001", asset->get<string>("deviceUuid"));

  auto targets = asset->getList("Targets");
  ASSERT_TRUE(targets);
  ASSERT_EQ(3, targets->size());

  {
    auto it = targets->begin();
    EXPECT_EQ("TargetDevice", (*it)->getName());
    EXPECT_EQ("Mazak123", (*it)->get<string>("deviceUuid"));

    it++;
    EXPECT_EQ("TargetDevice", (*it)->getName());
    EXPECT_EQ("Mazak456", (*it)->get<string>("deviceUuid"));

    auto targetGroup = *++it;
    EXPECT_EQ("TargetGroup", targetGroup->getName());
    EXPECT_EQ("MyRobots", targetGroup->get<string>("groupId"));

    auto targetDevices = targetGroup->get<EntityList>("LIST");
    ASSERT_EQ(2, targetDevices.size());

    {
      auto dit = targetDevices.begin();
      EXPECT_EQ("TargetDevice", (*dit)->getName());
      EXPECT_EQ("UR123", (*dit)->get<string>("deviceUuid"));
      dit++;

      EXPECT_EQ("TargetDevice", (*dit)->getName());
      EXPECT_EQ("UR456", (*dit)->get<string>("deviceUuid"));
    }
  }
  
  auto coordinator = asset->get<EntityPtr>("Coordinator");
  ASSERT_TRUE(coordinator);
  EXPECT_EQ("Coordinator", coordinator->getName());
  auto collaborator = coordinator->get<EntityPtr>("Collaborator");
  ASSERT_TRUE(collaborator);
  EXPECT_EQ("machine", collaborator->get<string>("collaboratorId"));
  EXPECT_EQ("CNC", collaborator->get<string>("type"));
  
  auto collTargets = collaborator->getList("Targets");
  ASSERT_EQ(2, collTargets->size());
  {
    auto it = collTargets->begin();
    EXPECT_EQ("TargetDevice", (*it)->getName());
    EXPECT_EQ("Mazak123", (*it)->get<string>("deviceUuid"));
    it++;
    EXPECT_EQ("TargetDevice", (*it)->getName());
    EXPECT_EQ("Mazak456", (*it)->get<string>("deviceUuid"));
  }
  
  auto collaborators = asset->getList("Collaborators");
  ASSERT_EQ(2, collaborators->size());
  {
    auto it = collaborators->begin();
    auto collab1 = *it;
    EXPECT_EQ("Robot", collab1->get<string>("collaboratorId"));
    EXPECT_EQ("ROBOT", collab1->get<string>("type"));
    auto targets = collab1->getList("Targets");
    ASSERT_EQ(2, targets->size());
    {
      auto tit = targets->begin();
      EXPECT_EQ("TargetRequirement", (*tit)->getName());
      EXPECT_EQ("ab", (*tit)->get<string>("requirementId"));
      
      auto table = (*tit)->get<DataSet>("CapabilityTable");
      ASSERT_EQ(2, table.size());
      
      const auto &row1 = get<TableRow>(table.find(DataSetEntry("PAYLOAD"))->m_value);
      ASSERT_EQ(1, row1.size());
      EXPECT_EQ(1000, get<int64_t>(row1.find(TableCell("maximum"))->m_value));

      const auto &row2 = get<TableRow>(table.find(DataSetEntry("REACH"))->m_value);
      ASSERT_EQ(1, row2.size());
      EXPECT_EQ(1500, get<int64_t>(row2.find(TableCell("minimum"))->m_value));
      
      tit++;
      ASSERT_EQ("TargetRef", (*tit)->getName());
      ASSERT_EQ("MyRobots", (*tit)->get<string>("groupIdRef"));
    }
    
    it++;
    auto collab2 = *it;
    EXPECT_EQ("robot2", collab2->get<string>("collaboratorId"));
    EXPECT_EQ("ROBOT", collab2->get<string>("type"));
    
    auto targets2 = collab2->getList("Targets");
    ASSERT_EQ(1, targets2->size());
    {
      auto target = targets2->front();
      EXPECT_EQ("TargetDevice", target->getName());
      EXPECT_EQ("UR890", target->get<string>("deviceUuid"));
    }
  }

  auto subtasks = asset->getList("SubTaskRefs");
  ASSERT_TRUE(subtasks);
  ASSERT_EQ(2, subtasks->size());
  
  {
    auto it = subtasks->begin();
    EXPECT_EQ("SubTaskRef", (*it)->getName());
    EXPECT_EQ(1, (*it)->get<int64_t>("order"));
    EXPECT_EQ("UnloadConv", (*it)->getValue<string>());

    it++;
    EXPECT_EQ("SubTaskRef", (*it)->getName());
    EXPECT_EQ(2, (*it)->get<int64_t>("order"));
    EXPECT_EQ("LoadCnc", (*it)->getValue<string>());
  }
  
  // Round trip test
  entity::XmlPrinter printer;
  printer.print(*m_writer, entity, {});

  string content = m_writer->getContent();
  ASSERT_EQ(content, doc);
}

TEST_F(TaskAssetTest, task_archetype_should_produce_json)
{
  GTEST_SKIP();
}

TEST_F(TaskAssetTest, task_archetype_must_have_collaborators)
{
  GTEST_SKIP();
}

TEST_F(TaskAssetTest, task_archetype_must_have_a_coordinator)
{
  GTEST_SKIP();
}

TEST_F(TaskAssetTest, task_archetype_should_have_optional_fields_for_sub_task_refs)
{
  GTEST_SKIP();
}

TEST_F(TaskAssetTest, should_parse_task)
{
  GTEST_SKIP();
}

TEST_F(TaskAssetTest, task_should_produce_json)
{
  GTEST_SKIP();
}


