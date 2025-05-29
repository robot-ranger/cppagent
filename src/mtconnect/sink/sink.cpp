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

#include "sink.hpp"

#include "mtconnect/logging.hpp"

namespace mtconnect {
  namespace sink {
    SinkPtr SinkFactory::make(const std::string &factoryName, const std::string &sinkName,
                              boost::asio::io_context &io, SinkContractPtr &&contract,
                              const ConfigOptions &options,
                              const boost::property_tree::ptree &block)
    {
      auto factory = m_factories.find(factoryName);
      if (factory != m_factories.end())
      {
        return factory->second(sinkName, io, std::move(contract), options, block);
      }
      else
      {
        LOG(error) << "Cannot find Sink for name: " << factoryName;
      }
      return nullptr;
    }
  }  // namespace sink
}  // namespace mtconnect
