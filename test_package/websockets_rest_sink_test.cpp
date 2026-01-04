
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

#include "mtconnect/logging.hpp"
#include "mtconnect/sink/rest_sink/server.hpp"
#include "agent_test_helper.hpp"
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
    m_agentTestHelper->createAgent("/samples/dyn_load.xml", 8, 64, "2.6", 25, true,
                                   true, {{configuration::JsonVersion, 2},
      {configuration::DisableAgentDevice, true}
    });
    m_agentId = to_string(getCurrentTimeInSec());
  }
  
  void TearDown() override { m_agentTestHelper.reset(); }
  
  void addAdapter(ConfigOptions options = ConfigOptions {})
  {
    m_agentTestHelper->addAdapter(options, "localhost", 7878,
                                  m_agentTestHelper->m_agent->getDefaultDevice()->getName());
  }
  
  template <typename Rep, typename Period>
  bool waitFor(const chrono::duration<Rep, Period>& time, function<bool()> pred)
  {
    auto &context = m_agentTestHelper->m_ioContext;
    boost::asio::steady_timer timer(context);
    timer.expires_after(time);
    bool timeout = false;
    timer.async_wait([&timeout](boost::system::error_code ec) {
      if (!ec)
      {
        timeout = true;
      }
    });
    
    while (!timeout && !pred())
    {
      context.run_for(500ms);
    }
    timer.cancel();
    
    return pred();
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

    ASSERT_EQ("LinuxCNC",  jdoc.at("/MTConnectDevices/Devices/Device/0/name"_json_pointer).get<string>());
    ASSERT_EQ("000",  jdoc.at("/MTConnectDevices/Devices/Device/0/uuid"_json_pointer).get<string>());
  }
}

TEST_F(WebsocketsRestSinkTest, should_handle_simple_current)
{
  GTEST_SKIP() << "Test not implemented yet";
}

TEST_F(WebsocketsRestSinkTest, should_handle_current_at)
{
  GTEST_SKIP() << "Test not implemented yet";
}

TEST_F(WebsocketsRestSinkTest, should_handle_simple_sample)
{
  GTEST_SKIP() << "Test not implemented yet";
}

TEST_F(WebsocketsRestSinkTest, should_handle_sample_from)
{
  GTEST_SKIP() << "Test not implemented yet";
}

TEST_F(WebsocketsRestSinkTest, should_handle_asset_request)
{
  GTEST_SKIP() << "Test not implemented yet";
}

TEST_F(WebsocketsRestSinkTest, should_handle_asset_with_id_array)
{
  GTEST_SKIP() << "Test not implemented yet";
}

TEST_F(WebsocketsRestSinkTest, should_handle_sample_streaming)
{
  GTEST_SKIP() << "Test not implemented yet";
}

TEST_F(WebsocketsRestSinkTest, should_handle_multiple_streaming_reqeuests)
{
  GTEST_SKIP() << "Test not implemented yet";
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
