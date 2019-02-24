//
// Copyright Copyright 2009-2019, AMT – The Association For Manufacturing Technology (“AMT”)
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

#include "component_test.hpp"
#include "device.hpp"

using namespace std;

namespace mtconnect {
  namespace test {
    // Registers the fixture into the 'registry'
    CPPUNIT_TEST_SUITE_REGISTRATION(ComponentTest);
    
    // ComponentTest public methods
    void ComponentTest::setUp()
    {
      std::map<string, string> attributes1;
      attributes1["id"] = "1";
      attributes1["name"] = "ComponentTest1";
      attributes1["nativeName"] = "NativeName";
      attributes1["uuid"] = "UnivUniqId1";
      m_compA = new Component("Axes", attributes1);
      
      std::map<string, string> attributes2;
      attributes2["id"] = "3";
      attributes2["name"] = "ComponentTest2";
      attributes2["uuid"] = "UnivUniqId2";
      attributes2["sampleRate"] = "123.4";
      m_compB = new Component("Controller", attributes2);
    }
    
    
    void ComponentTest::tearDown()
    {
      delete m_compA; m_compA = nullptr;
      delete m_compB; m_compB = nullptr;
    }
    
    
    void ComponentTest::testGetters()
    {
      CPPUNIT_ASSERT_EQUAL((string) "Axes", m_compA->getClass());
      CPPUNIT_ASSERT_EQUAL((string) "1", m_compA->getId());
      CPPUNIT_ASSERT_EQUAL((string) "ComponentTest1", m_compA->getName());
      CPPUNIT_ASSERT_EQUAL((string) "UnivUniqId1", m_compA->getUuid());
      CPPUNIT_ASSERT_EQUAL((string) "NativeName", m_compA->getNativeName());
      
      CPPUNIT_ASSERT_EQUAL((string) "Controller", m_compB->getClass());
      CPPUNIT_ASSERT_EQUAL((string) "3", m_compB->getId());
      CPPUNIT_ASSERT_EQUAL((string) "ComponentTest2", m_compB->getName());
      CPPUNIT_ASSERT_EQUAL((string) "UnivUniqId2", m_compB->getUuid());
      CPPUNIT_ASSERT(m_compB->getNativeName().empty());
    }
    
    
    void ComponentTest::testGetAttributes()
    {
      const auto &attributes1 = m_compA->getAttributes();
      
      CPPUNIT_ASSERT_EQUAL((string) "1", attributes1.at("id"));
      CPPUNIT_ASSERT_EQUAL((string) "ComponentTest1", attributes1.at("name"));
      CPPUNIT_ASSERT_EQUAL((string) "UnivUniqId1", attributes1.at("uuid"));
      CPPUNIT_ASSERT(attributes1.find("sampleRate") == attributes1.end());
      
      const auto &attributes2 = m_compB->getAttributes();
      
      CPPUNIT_ASSERT_EQUAL((string) "3", attributes2.at("id"));
      CPPUNIT_ASSERT_EQUAL((string) "ComponentTest2", attributes2.at("name"));
      CPPUNIT_ASSERT_EQUAL((string) "UnivUniqId2", attributes2.at("uuid"));
      CPPUNIT_ASSERT_EQUAL((string) "123.4", attributes2.at("sampleInterval"));
    }
    
    
    void ComponentTest::testDescription()
    {
      map<string, string> attributes;
      attributes["manufacturer"] = "MANUFACTURER";
      attributes["serialNumber"] = "SERIAL_NUMBER";
      
      m_compA->addDescription((string) "Machine 1", attributes);
      auto description1 = m_compA->getDescription();
      
      CPPUNIT_ASSERT_EQUAL((string) "MANUFACTURER", description1["manufacturer"]);
      CPPUNIT_ASSERT_EQUAL((string) "SERIAL_NUMBER", description1["serialNumber"]);
      CPPUNIT_ASSERT(description1["station"].empty());
      CPPUNIT_ASSERT_EQUAL((string) "Machine 1", m_compA->getDescriptionBody());
      
      attributes["station"] = "STATION";
      m_compB->addDescription((string) "", attributes);
      auto description2 = m_compB->getDescription();
      
      CPPUNIT_ASSERT_EQUAL((string) "MANUFACTURER", description2["manufacturer"]);
      CPPUNIT_ASSERT_EQUAL((string) "SERIAL_NUMBER", description2["serialNumber"]);
      CPPUNIT_ASSERT_EQUAL((string) "STATION", description2["station"]);
      CPPUNIT_ASSERT(m_compB->getDescriptionBody().empty());
      
    }
    
    
    void ComponentTest::testRelationships()
    {
      // Test get/set parents
      map<string, string> dummy;
      Component linear("Linear", dummy);
      
      m_compA->setParent(linear);
      CPPUNIT_ASSERT_EQUAL(&linear, m_compA->getParent());
      
      Device device(dummy);
      auto devPointer = dynamic_cast<Component *>(&device);
      
      CPPUNIT_ASSERT(devPointer);
      linear.setParent(*devPointer);
      CPPUNIT_ASSERT_EQUAL(devPointer, linear.getParent());
      
      // Test get device
      CPPUNIT_ASSERT_EQUAL(&device, m_compA->getDevice());
      CPPUNIT_ASSERT_EQUAL(&device, linear.getDevice());
      CPPUNIT_ASSERT_EQUAL(&device, device.getDevice());
      
      // Test add/get children
      CPPUNIT_ASSERT(m_compA->getChildren().empty());
      
      Component axes("Axes", dummy), thermostat("Thermostat", dummy);
      m_compA->addChild(axes);
      m_compA->addChild(thermostat);
      
      CPPUNIT_ASSERT_EQUAL((size_t) 2, m_compA->getChildren().size());
      CPPUNIT_ASSERT_EQUAL(&axes, m_compA->getChildren().front());
      CPPUNIT_ASSERT_EQUAL(&thermostat, m_compA->getChildren().back());
    }
    
    
    void ComponentTest::testDataItems()
    {
      CPPUNIT_ASSERT(m_compA->getDataItems().empty());
      
      map<string, string> dummy;
      
      DataItem data1(dummy), data2(dummy);
      m_compA->addDataItem(data1);
      m_compA->addDataItem(data2);
      
      CPPUNIT_ASSERT_EQUAL((size_t) 2, m_compA->getDataItems().size());
      CPPUNIT_ASSERT_EQUAL(&data1, m_compA->getDataItems().front());
      CPPUNIT_ASSERT_EQUAL(&data2, m_compA->getDataItems().back());
    }
    
    
    void ComponentTest::testReferences()
    {
      string id("a"), name("xxx");
      Component::Reference ref(id, name, Component::Reference::DATA_ITEM);
      
      m_compA->addReference(ref);
      CPPUNIT_ASSERT_EQUAL((size_t) 1, m_compA->getReferences().size());
      
      CPPUNIT_ASSERT_EQUAL((string) "xxx", m_compA->getReferences().front().m_name);
      CPPUNIT_ASSERT_EQUAL((string) "a", m_compA->getReferences().front().m_id);
    }
  }
}
