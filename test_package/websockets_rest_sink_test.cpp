
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

#include <boost/algorithm/string.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "agent_test_helper.hpp"
#include "mtconnect/logging.hpp"
#include "mtconnect/sink/rest_sink/server.hpp"
#include "test_utilities.hpp"

using namespace std;
using namespace mtconnect;
using namespace mtconnect::sink::rest_sink;

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = boost::beast::http;
using tcp = boost::asio::ip::tcp;
namespace websocket = beast::websocket;

// main
int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

class WebsocketsRestSinkTest : public testing::Test
{
protected:
  void SetUp() override
  {
    m_agentTestHelper = make_unique<AgentTestHelper>();
    m_agentTestHelper->createAgent(
        "/samples/dyn_load.xml", 8, 64, "2.6", 25, true, true,
        {{configuration::JsonVersion, 2}, {configuration::DisableAgentDevice, true}});
    m_agentId = to_string(getCurrentTimeInSec());
  }

  void TearDown() override { m_agentTestHelper.reset(); }

  void addAdapter(ConfigOptions options = ConfigOptions {})
  {
    m_agentTestHelper->addAdapter(options, "localhost", 7878,
                                  m_agentTestHelper->m_agent->getDefaultDevice()->getName());
  }

public:
  std::string m_agentId;
  std::unique_ptr<AgentTestHelper> m_agentTestHelper;

  std::chrono::milliseconds m_delay {};
};

TEST_F(WebsocketsRestSinkTest, should_handle_simple_probe)
{
  {
    PARSE_XML_WS_RESPONSE(R"({ "id": "1234", "request": "probe"})");
    ASSERT_EQ("1234", id);

    ASSERT_XML_PATH_EQUAL(doc, "//m:Devices/m:Device@name", "LinuxCNC");
    ASSERT_XML_PATH_EQUAL(doc, "//m:Devices/m:Device@uuid", "000");
    ASSERT_XML_PATH_EQUAL(doc, "//m:Devices/m:Device/m:Components/m:Controller@id", "cont");
  }
}

TEST_F(WebsocketsRestSinkTest, should_handle_json_probe)
{
  {
    PARSE_JSON_WS_RESPONSE(R"({ "id": "1234", "request": "probe", "format": "json"})");
    ASSERT_EQ("1234", id);

    ASSERT_EQ("LinuxCNC",
              jdoc.at("/MTConnectDevices/Devices/Device/0/name"_json_pointer).get<string>());
    ASSERT_EQ("000", jdoc.at("/MTConnectDevices/Devices/Device/0/uuid"_json_pointer).get<string>());
  }
}

TEST_F(WebsocketsRestSinkTest, should_handle_simple_current)
{
  addAdapter();

  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|avail|AVAILABLE");
  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|mode|MANUAL");

  {
    PARSE_XML_WS_RESPONSE(R"({ "id": "1", "request": "current", "format": "xml"})");
    ASSERT_EQ("1", id);

    ASSERT_XML_PATH_EQUAL(doc, "//m:Availability", "AVAILABLE");
    ASSERT_XML_PATH_EQUAL(doc, "//m:ControllerMode", "MANUAL");
    ASSERT_XML_PATH_EQUAL(doc, "//m:Execution", "UNAVAILABLE");
  }
}

TEST_F(WebsocketsRestSinkTest, should_handle_current_at)
{
  addAdapter();

  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|avail|AVAILABLE");
  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|mode|MANUAL");

  auto at = m_agentTestHelper->m_agent->getCircularBuffer().getSequence() - 1;

  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|mode|AUTOMATIC");
  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|exec|READY");

  {
    PARSE_XML_WS_RESPONSE(
        format(R"({{ "id": "1", "request": "current", "format": "xml", "at": {} }})", at));
    ASSERT_EQ("1", id);

    ASSERT_XML_PATH_EQUAL(doc, "//m:Availability", "AVAILABLE");
    ASSERT_XML_PATH_EQUAL(doc, "//m:ControllerMode", "MANUAL");
    ASSERT_XML_PATH_EQUAL(doc, "//m:Execution", "UNAVAILABLE");

    m_agentTestHelper->waitForResponseSent(10ms, id);
  }

  {
    PARSE_XML_WS_RESPONSE(R"({ "id": "1", "request": "current", "format": "xml" })");
    ASSERT_EQ("1", id);

    ASSERT_XML_PATH_EQUAL(doc, "//m:Availability", "AVAILABLE");
    ASSERT_XML_PATH_EQUAL(doc, "//m:ControllerMode", "AUTOMATIC");
    ASSERT_XML_PATH_EQUAL(doc, "//m:Execution", "READY");
  }
}

TEST_F(WebsocketsRestSinkTest, should_handle_simple_sample)
{
  addAdapter();

  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|avail|AVAILABLE");
  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|mode|MANUAL");
  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|mode|AUTOMATIC");
  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|exec|READY");

  {
    PARSE_XML_WS_RESPONSE(R"({ "id": "1", "request": "sample", "format": "xml" })");
    ASSERT_EQ("1", id);

    ASSERT_XML_PATH_EQUAL(doc, "//m:Availability[1]", "UNAVAILABLE");
    ASSERT_XML_PATH_EQUAL(doc, "//m:Availability[2]", "AVAILABLE");
    ASSERT_XML_PATH_EQUAL(doc, "//m:ControllerMode[1]", "UNAVAILABLE");
    ASSERT_XML_PATH_EQUAL(doc, "//m:ControllerMode[2]", "MANUAL");
    ASSERT_XML_PATH_EQUAL(doc, "//m:ControllerMode[3]", "AUTOMATIC");
    ASSERT_XML_PATH_EQUAL(doc, "//m:Execution[1]", "UNAVAILABLE");
    ASSERT_XML_PATH_EQUAL(doc, "//m:Execution[2]", "READY");
  }
}

TEST_F(WebsocketsRestSinkTest, should_handle_sample_from)
{
  addAdapter();

  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|avail|AVAILABLE");
  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|mode|MANUAL");

  auto at = m_agentTestHelper->m_agent->getCircularBuffer().getSequence();

  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|mode|AUTOMATIC");
  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|exec|READY");
  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|exec|ACTIVE");
  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|exec|READY");

  {
    PARSE_XML_WS_RESPONSE(
        format(R"({{ "id": "1", "request": "sample", "format": "xml", "from": {} }})", at));
    ASSERT_EQ("1", id);

    ASSERT_XML_PATH_EQUAL(doc, "//m:ControllerMode[1]", "AUTOMATIC");
    ASSERT_XML_PATH_EQUAL(doc, "//m:Execution[1]", "READY");
    ASSERT_XML_PATH_EQUAL(doc, "//m:Execution[2]", "ACTIVE");
    ASSERT_XML_PATH_EQUAL(doc, "//m:Execution[3]", "READY");
  }
}

TEST_F(WebsocketsRestSinkTest, should_handle_asset_request)
{
  addAdapter();

  m_agentTestHelper->m_adapter->processData(
      "2021-02-01T12:00:00Z|@ASSET@|P1|FakeAsset|<FakeAsset assetId='P1'>TEST 1</FakeAsset>");

  {
    PARSE_XML_WS_RESPONSE(R"({ "id": "1", "request": "asset", "format": "xml"})");
    ASSERT_EQ("1", id);

    ASSERT_XML_PATH_EQUAL(doc, "//m:FakeAsset@assetId", "P1");
    ASSERT_XML_PATH_EQUAL(doc, "//m:FakeAsset", "TEST 1");
  }
}

TEST_F(WebsocketsRestSinkTest, should_handle_asset_with_id_array)
{
  addAdapter();

  m_agentTestHelper->m_adapter->processData(
      "2021-02-01T12:00:00Z|@ASSET@|P1|FakeAsset|<FakeAsset assetId='P1'>TEST 1</FakeAsset>");
  m_agentTestHelper->m_adapter->processData(
      "2021-02-01T12:00:00Z|@ASSET@|P2|FakeAsset|<FakeAsset assetId='P2'>TEST 2</FakeAsset>");
  m_agentTestHelper->m_adapter->processData(
      "2021-02-01T12:00:00Z|@ASSET@|P3|FakeAsset|<FakeAsset assetId='P3'>TEST 3</FakeAsset>");

  {
    PARSE_XML_WS_RESPONSE(
        R"({ "id": "1", "request": "asset", "assetIds": ["P1", "P2"], "format": "xml"})");
    ASSERT_EQ("1", id);

    ASSERT_XML_PATH_COUNT(doc, "//m:FakeAsset", 2);

    ASSERT_XML_PATH_EQUAL(doc, "//m:FakeAsset[1]@assetId", "P1");
    ASSERT_XML_PATH_EQUAL(doc, "//m:FakeAsset[1]", "TEST 1");

    ASSERT_XML_PATH_EQUAL(doc, "//m:FakeAsset[2]@assetId", "P2");
    ASSERT_XML_PATH_EQUAL(doc, "//m:FakeAsset[2]", "TEST 2");
  }
}

TEST_F(WebsocketsRestSinkTest, should_handle_sample_streaming)
{
  addAdapter();

  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|avail|AVAILABLE");
  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|mode|MANUAL");

  auto at = m_agentTestHelper->m_agent->getCircularBuffer().getSequence();

  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|mode|AUTOMATIC");
  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|exec|READY");
  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|exec|ACTIVE");
  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|exec|READY");

  {
    BEGIN_ASYNC_WS_REQUEST(format(
        R"({{ "id": "1", "request": "sample", "format": "xml", "interval": 10, "from": {} }})",
        at));
    ASSERT_EQ("1", id);

    m_agentTestHelper->waitForResponseSent(15ms, id);

    PARSE_NEXT_XML_RESPONSE(id);

    ASSERT_XML_PATH_EQUAL(doc, "//m:Header@requestId", "1");
    ASSERT_XML_PATH_EQUAL(doc, "//m:ControllerMode[1]", "AUTOMATIC");
    ASSERT_XML_PATH_EQUAL(doc, "//m:Execution[1]", "READY");
    ASSERT_XML_PATH_EQUAL(doc, "//m:Execution[2]", "ACTIVE");
    ASSERT_XML_PATH_EQUAL(doc, "//m:Execution[3]", "READY");
  }

  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|exec|ACTIVE");
  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|exec|READY");

  m_agentTestHelper->waitForResponseSent(15ms, "1");

  {
    PARSE_NEXT_XML_RESPONSE("1");

    ASSERT_XML_PATH_EQUAL(doc, "//m:Header@requestId", "1");
    ASSERT_XML_PATH_EQUAL(doc, "//m:Execution[1]", "ACTIVE");
    ASSERT_XML_PATH_EQUAL(doc, "//m:Execution[2]", "READY");
  }
}

TEST_F(WebsocketsRestSinkTest, should_handle_multiple_streaming_reqeuests)
{
  addAdapter();

  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|avail|AVAILABLE");
  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|mode|MANUAL");

  auto at = m_agentTestHelper->m_agent->getCircularBuffer().getSequence();

  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|mode|AUTOMATIC");
  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|exec|READY");
  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|exec|ACTIVE");
  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|exec|READY");

  {
    BEGIN_ASYNC_WS_REQUEST(format(
        R"({{ "id": "1", "request": "sample", "format": "xml", "interval": 10, "from": {} }})",
        at));
    ASSERT_EQ("1", id);
  }

  {
    BEGIN_ASYNC_WS_REQUEST(
        R"({ "id": "2", "request": "current", "format": "xml", "interval": 100})");
    ASSERT_EQ("2", id);
  }

  m_agentTestHelper->waitForResponseSent(15ms, "1");

  {
    PARSE_NEXT_XML_RESPONSE("1");

    ASSERT_XML_PATH_EQUAL(doc, "//m:Header@requestId", "1");
    ASSERT_XML_PATH_EQUAL(doc, "//m:ControllerMode[1]", "AUTOMATIC");
    ASSERT_XML_PATH_EQUAL(doc, "//m:Execution[1]", "READY");
    ASSERT_XML_PATH_EQUAL(doc, "//m:Execution[2]", "ACTIVE");
    ASSERT_XML_PATH_EQUAL(doc, "//m:Execution[3]", "READY");
  }

  {
    PARSE_NEXT_XML_RESPONSE("2");

    ASSERT_XML_PATH_EQUAL(doc, "//m:Header@requestId", "2");
    ASSERT_XML_PATH_EQUAL(doc, "//m:ControllerMode", "AUTOMATIC");
    ASSERT_XML_PATH_EQUAL(doc, "//m:Execution", "READY");
  }

  m_agentTestHelper->waitForResponseSent(100ms, "2");

  {
    PARSE_NEXT_XML_RESPONSE("2");

    ASSERT_XML_PATH_EQUAL(doc, "//m:Header@requestId", "2");
    ASSERT_XML_PATH_EQUAL(doc, "//m:ControllerMode", "AUTOMATIC");
    ASSERT_XML_PATH_EQUAL(doc, "//m:Execution", "READY");
  }

  m_agentTestHelper->waitForResponseSent(105ms, "2");

  {
    PARSE_NEXT_XML_RESPONSE("2");

    ASSERT_XML_PATH_EQUAL(doc, "//m:Header@requestId", "2");
    ASSERT_XML_PATH_EQUAL(doc, "//m:ControllerMode", "AUTOMATIC");
    ASSERT_XML_PATH_EQUAL(doc, "//m:Execution", "READY");
  }

  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|exec|ACTIVE");
  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|exec|READY");
  m_agentTestHelper->m_adapter->processData("2026-01-01T12:00:00Z|exec|ACTIVE");

  m_agentTestHelper->waitForResponseSent(15ms, "1");

  {
    PARSE_NEXT_XML_RESPONSE("1");

    ASSERT_XML_PATH_EQUAL(doc, "//m:Header@requestId", "1");
    ASSERT_XML_PATH_EQUAL(doc, "//m:Execution[1]", "ACTIVE");
    ASSERT_XML_PATH_EQUAL(doc, "//m:Execution[2]", "READY");
    ASSERT_XML_PATH_EQUAL(doc, "//m:Execution[3]", "ACTIVE");
  }

  m_agentTestHelper->waitForResponseSent(100ms, "2");

  {
    PARSE_NEXT_XML_RESPONSE("2");

    ASSERT_XML_PATH_EQUAL(doc, "//m:Header@requestId", "2");
    ASSERT_XML_PATH_EQUAL(doc, "//m:ControllerMode", "AUTOMATIC");
    ASSERT_XML_PATH_EQUAL(doc, "//m:Execution", "ACTIVE");
  }
}

TEST_F(WebsocketsRestSinkTest, should_handle_multiple_streaming_reqeuests_with_cancel)
{
  GTEST_SKIP() << "Test not implemented yet";
}

TEST_F(WebsocketsRestSinkTest, should_handle_asset_put)
{
  GTEST_SKIP() << "Test not implemented yet";
}

TEST_F(WebsocketsRestSinkTest, should_return_error_if_no_id)
{
  GTEST_SKIP() << "Test not implemented yet";
}

TEST_F(WebsocketsRestSinkTest, should_return_error_if_duplicate_id)
{
  GTEST_SKIP() << "Test not implemented yet";
}

TEST_F(WebsocketsRestSinkTest, should_return_error_wrong_parameter_type)
{
  GTEST_SKIP() << "Test not implemented yet";
}

TEST_F(WebsocketsRestSinkTest, should_return_error_for_unknown_command)
{
  GTEST_SKIP() << "Test not implemented yet";
}

TEST_F(WebsocketsRestSinkTest, should_return_error_for_malformed_json)
{
  GTEST_SKIP() << "Test not implemented yet";
}

TEST_F(WebsocketsRestSinkTest, should_return_error_for_unknown_parameter)
{
  GTEST_SKIP() << "Test not implemented yet";
}

TEST_F(WebsocketsRestSinkTest, should_return_error_for_unknown_device)
{
  GTEST_SKIP() << "Test not implemented yet";
}

TEST_F(WebsocketsRestSinkTest, should_return_error_for_bad_parameter_value)
{
  GTEST_SKIP() << "Test not implemented yet";
}

TEST_F(WebsocketsRestSinkTest, should_coerce_parameter_data_types)
{
  GTEST_SKIP() << "Test not implemented yet";
}
