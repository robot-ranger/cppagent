# 🚀 MTConnect C++ Agent
A modern and extensible C++ implementation of the MTConnect standard, designed to reliably collect machine data from diverse industrial devices and expose it in a consistent, structured format. The agent normalizes raw device signals, ensures compliance with the MTConnect standard, and makes the data accessible to client applications such as MES systems, monitoring dashboards, analytics platforms, or custom integrations.

The agent supports multiple industrial communication models (SHDR, JSON, MQTT), multiple devices, secure network transport, and pluggable data transformation logic, making it suitable for both small installations and large-scale production environments.

---

# 📑 Table of Contents

- [Overview](#overview)
- [Key Features](#key-features)
- [Architecture](#architecture)
- [Quick Start](#quick-start)
- [Building From Source](#building-from-source)
- [Basic Configuration](#basic-configuration)
- [Advanced Configuration](#advanced-configuration)
  - [Multiple Adapters](#multiple-adapters)
  - [HTTP & TLS](#http--tls)
  - [MQTT](#mqtt)
  - [Ruby Extensions](#ruby-extensions)
- [Sample Outputs](#sample-outputs)
- [Directory Structure](#directory-structure)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [Version History](#version-history)
- [Useful Links](#useful-links)
- [License](#license)

---

# 🧭 Overview

The MTConnect Agent acts as the intermediary between raw machine data and higher-level software systems that require structured, contextualized insights. It transforms messy, vendor-specific machine signals into clean, semantic MTConnect XML/JSON outputs. The agent follows a one-way, read-only information model, meaning it never sends instructions back to machines—making it safe and easy to integrate into existing manufacturing setups.

By handling multiple adapters, buffering high-frequency data, supporting secure communication, and offering real-time streaming capabilities, the agent is suitable for both small machine shops and enterprise-level Industry 4.0 deployments.

---

# ⭐ Key Features

### 🔌 Wide Input Support
- SHDR adapters for CNC interoperability  
- MQTT ingestion for IoT environments  
  - Native JSON input for modern sensors and custom connectors
  - Customizable JSON handling for propriatary models

### 🌐 Flexible Output Options
- HTTP server for `/probe`, `/current`, `/sample`, `/assets`
- WebSocket streaming for real-time dashboards
- TLS support for secure communication
- MQTT output using flattended or hierarchial topics

### 🧠 Extensibility
- Ruby-based scripting for custom transformation pipelines  
- Modular components allowing custom adapters and output
- Namespace support for extended functionality  

### ⚙️ Industrial-Grade Infrastructure
- High-performance circular buffer for storing samples  
- Cross-platform support (Windows, Linux, macOS)  
- Low-overhead runtime for embedded or resource-constrained machines  

---

# 🧱 Architecture
The agent transforms heterogeneous raw machine signals into structured, queryable MTConnect data streams. This architecture ensures consistent data modeling regardless of vendor or device type.

```
              ┌─────────────────────────┐
              │      Client Layer       │
              │  MES, Dashboards, Apps  │
              └──────────┬──────────────┘
                         │
            HTTP / WebSockets / TLS / MQTT
                         │
               ┌─────────┴───────────┐
               │   MTConnect Agent   │
               │   (C++ Reference)   │
               └─────┬────────┬──────┘
                     │        │
        Normalization &    Buffering
                     │
      ┌──────────────┴─────────────────┐
      │                                │
   SHDR Adapters                     MQTT Broker
      │                                │
Raw Machine Signals         Sensor/Device Messages
```

This architecture is designed to be modular, allowing organizations to gradually expand their MTConnect ecosystem without reconfiguring existing systems.

---

# ⚡ Quick Start

> We should change this to download pre-built binaries first.

This section helps developers get up and running with the agent in minutes.

### 1. Downloading Pre-Built Binaries
Pre-built binaries for Windows and Linux platforms are available on the [Releases](https://github.com/mtconnect/cppagent/releases) page.

### 2. Instructions for using Windows Binaries
1. Download the latest `.zip` release for Windows.
2. Extract the contents to a desired location.

### 3. Create a Minimal Configuration
```txt
Devices = Devices.xml
Port = 5000

Adapters {
  MyCNC {
    Host = localhost
    Port = 7878
  }
}
```

### 4. Run the Agent
```powershell
C> agent.exe run agent.cfg
```

### 5. Verify Output
- http://localhost:5000/probe  
- http://localhost:5000/current  
- http://localhost:5000/sample  

---

# 🛠 Building From Source

[Wiki Page for Building From Source](https://github.com/mtconnect/cppagent/wiki/Building-From-Source)

---

> Remove this section 
<details>
<summary><b>Building From Source</b></summary>
The agent uses CMake + Conan for dependency management, making cross-platform builds consistent.

> Note: I think there is an easier way of doing this. You can create a release zip file with cpack and all the dependencies if needed. 
>
> See the docker files for examples:
> ```bash
>  && conan create cppagent \
>       --build=missing \
>       -o cpack=True \
>       -o "cpack_destination=$HOME/agent" \
>       -o cpack_name=dist \
>       -o cpack_generator=TGZ \
>       -pr "$CONAN_PROFILE" \
>       ${WITH_TESTS_ARG} \
```


## Linux Build
Install prerequisites:
```bash
sudo apt-get install build-essential cmake libssl-dev libcurl4-openssl-dev
```
Build:
```bash
conan install . --build=missing
cmake --preset conan-release
cmake --build build-release
```

---

## Windows Build
- Install Visual Studio 2022
- Install CMake + Conan
```powershell
conan install . --build=missing
cmake --preset conan-release
cmake --build build-release --config Release
```

---

## macOS Build 
macOS is ideal for development and prototyping.
```bash
brew install cmake openssl conan
conan install . --build=missing
cmake --preset conan-release
cmake --build build-release
```
</details>

---

---

> Continue Here
---

# 🔧 Basic Configuration

The agent is controlled via two primary files:

### **1. Devices.xml**
Defines the machine structure, including components and DataItems.
This file gives meaning to raw data received by adapters.

### **2. agent.cfg**
Sets up communication ports, adapters, buffer sizes, logging, security, and optional features.

A minimal configuration looks like:

Example:
```
Devices = Devices.xml
Port = 5000
AllowPut = true
Buffersize = 131072
```

# 🔧 Advanced Configuration

## **Overview of Advanced Features**

Some capabilities of the agent require additional setup or are only needed in more complex environments. These include:

- connecting multiple devices to a single agent,
- securing communication through TLS,
- ingesting MQTT-based data sources,
- applying custom logic using Ruby,
- enabling optional services like asset ingestion, data transforms, or extended namespaces.

The following sections explain when and why you would use these features, along with short examples.

## Multiple Adapters
The agent supports gathering data from multiple devices at the same time. Each device communicates using its own adapter defined by a host and port.

Use this when:

- you want one agent instance to serve multiple machines,
- you want to reduce infrastructure overhead,
- machines are located on the same subnet or facility network.

Example:
```
Adapters {
  Mill1 { 
    Host = 10.0.0.10 
    Port = 7878 
  }
  Lathe1 { 
    Host = 10.0.0.20 
    Port = 7879 
  }
  Router { 
    Host = router.local 
    Port = 9000 
  }
}
```

Each entry represents one physical machine producing SHDR output.

---

## HTTP & TLS
HTTP is the standard protocol for MTConnect clients to retrieve data.
TLS (HTTPS) is optional but recommended when:

- data crosses network boundaries,
- sensitive operation data must be encrypted,
- compliance or security policies require secure transport.

TLS enables the agent to serve MTConnect data securely using certificates.

Example configuration:
```
TlsDefaults {
  PrivateKeyFile = server.key
  CertificateFile = server.crt
  VerifyClientCertificate = true
}
```

After configuration, the agent serves encrypted data over `https://`.

---

## MQTT
The agent supports MQTT ingestion for devices or sensors that publish data to a broker instead of using SHDR.

Use MQTT when:

- working with IoT sensors, gateways, or PLCs,
- devices already publish metrics to an MQTT broker,
- you need a lightweight protocol for high-frequency data.

The agent requires the broker address, topics, and connection details:
```
MqttService {
  Host = tcp://broker:1883
  Topics = factory/mtconnect/#
  ClientId = agent01
}
```

Additional message mapping rules may be needed depending on your topic structure and payload format.

---

## Ruby Extensions
Ruby extensions allow you to customize how data is processed by the agent before it is exposed to clients. These scripts can:

- normalize vendor-specific data formats,
- apply mathematical transformations (e.g., unit conversions),
- filter or enrich incoming data,
- modify assets or samples before buffering,
- implement custom logic not present in the MTConnect standard.

The agent loads Ruby scripts from a directory:
```
Ruby {
  module = mymodule.rb
}
```

You can then write Ruby code that hooks into processing events.

## SHDR (Simple Hierarchical Data Representation)

### What SHDR Is
SHDR is the original MTConnect-defined line-based protocol for transmitting machine state and data items. It streams information as timestamped key-value pairs over a plain TCP connection. It is simple, deterministic, and extremely reliable — which is why it continues to be the standard for CNC machine adapters.

Example SHDR line:
```
2025-05-01T10:24:31.123Z|Xpos|12.345
```
### Why SHDR Exists

- CNC controls and legacy equipment often cannot speak MQTT/JSON directly.
- SHDR provides a low-overhead, real-time stream that adapters can implement with minimal dependencies.
- It was intentionally created by MTConnect to ensure interoperability across machine vendors.

### When to Use SHDR
Choose SHDR if:
- you are connecting to Fanuc, Mazak, Okuma, Haas, etc.
- you need deterministic, line-based real-time updates
- you are using existing MTConnect adapters shipped with machine vendors

### Alternatives

| Protocol | Best for | Notes |
|-------|--------|----------|
| SHDR | CNC & legacy equipment | Most common MTConnect adapter input |
| MQTT | IoT sensors, PLC networks, mixed devices | Requires broker & topic mapping |

> Note: JSON is a representation, not a transport protocol. It can be used with MQTT.

### Documentation & Reference

- MTConnect Adapters: [https://github.com/mtconnect/adapter](https://github.com/mtconnect/adapter)

### Why SHDR Is a One-Off
Unlike MQTT or JSON, SHDR is not a general IoT messaging protocol. It only exists in MTConnect ecosystems and is purpose-built for CNC-level real-time telemetry. It remains widely deployed but many modern environments now run blended architectures:

```
CNC → SHDR → Agent
IoT Sensor → MQTT → Agent
```
---

# 📤 Sample Outputs
Understanding what the agent returns helps developers build clients effectively.

### /probe
Shows device structure:
```xml
<Device id="dev1" name="CNC1">
  <Components>
    <Axes>...</Axes>
  </Components>
</Device>
```

### /current
Shows the latest sample for every data item.
```xml
<Position dataItemId="X">12.345</Position>
```

### /sample
Shows buffered time series data.
```xml
<CuttingSpeed timestamp="...">3500</CuttingSpeed>
```

These outputs comply with the MTConnect schema and can be validated using standard tools.

---
# Configuration Parameters
[Configuration Parameters Wiki Page](https://github.com/mtconnect/cppagent/wiki/Configuration-Parameters)
<details>
<summary>Configuration Parameters List</summary>
---------

### Top level configuration items ####

* `AgentDeviceUUID` - Set the UUID of the agent device

    *Default*: UUID derived from the IP address and port of the agent

* `BufferSize` - The 2^X number of slots available in the circular
  buffer for samples, events, and conditions.

    *Default*: 17 -> 2^17 = 131,072 slots.

* `CheckpointFrequency` - The frequency checkpoints are created in the
  stream. This is used for current with the at argument. This is an
  advanced configuration item and should not be changed unless you
  understand the internal workings of the agent.

    *Default*: 1000

* `CreateUniqueIds`: Changes all the ids in each element to a UUID that will be unique across devices. This is used for merging devices from multiple sources.

    *Default*: `false`

* `Devices` - The XML file to load that specifies the devices and is
  supplied as the result of a probe request. If the key is not found
  the defaults are tried.

    *Defaults*: probe.xml or Devices.xml

* `DisableAgentDevice` - When the schema version is >= 1.7, disable the
  creation of the Agent device.

    *Default*: false

* `JsonVersion`     - JSON Printer format. Old format: 1, new format: 2

    *Default*: 2

* `LogStreams` - Debugging flag to log the streamed data to a file. Logs to a file named: `Stream_` + timestamp + `.log` in the current working directory. This is only for the Rest Sink.

    *Default*: `false`

* `MaxAssets` - The maximum number of assets the agent can hold in its buffer. The
  number is the actual count, not an exponent.

    *Default*: 1024

* `MaxCachedFileSize` - The maximum size of a raw file to cache in memory.

    *Default*: 20 kb

* `MinCompressFileSize` - The file size where we begin compressing raw files sent to the client.

    *Default*: 100 kb

* `MinimumConfigReloadAge` - The minimum age of a config file before an agent reload is triggered (seconds).

    *Default*: 15 seconds

* `MonitorConfigFiles` - Monitor agent.cfg and Devices.xml files and restart agent if they change.

    *Default*: false

* `MonitorInterval` - The interval between checks if the agent.cfg or Device.xml files have changed.

    *Default*: 10 seconds

* `Pretty` - Pretty print the output with indententation

    *Default*: false

* `PidFile` - UNIX only. The full path of the file that contains the
  process id of the daemon. This is not supported in Windows.

    *Default*: agent.pid

* `SchemaVersion` - Change the schema version to a different version number.

    *Default*: *Current Supported Version*

* `Sender` - The value for the sender header attribute.

    *Default*: Local machine name

* `ServiceName` - Changes the service name when installing or removing
  the service. This allows multiple agents to run as services on the same machine.

    *Default*: MTConnect Agent

* `SuppressIPAddress` - Suppress the Adapter IP Address and port when creating the Agent Device ids and names. This applies to all adapters.

    *Default*: `false`

* `VersionDeviceXml` - Create a new versioned file every time the Device.xml file changes from an external source.

    *Default*: `false`

* `CorrectTimestamps` - Verify time is always progressing forward for each data item and correct if not.

    *Default*: `false`

* `Validation` - Turns on validation of model components and observations

    *Default*: `false`

* `WorkerThreads` - The number of operating system threads dedicated to the Agent

    *Default*: 1

#### Adapter General Configuration

These can be overridden on a per-adapter basis

* `Protocol` –Specify protocol. Options: [`shdr`|`mqtt`]

    *Default*: shdr

* `ConversionRequired` - Global default for data item units conversion in the agent.
  Assumes the adapter has already done unit conversion.

    *Default*: true

* `EnableSourceDeviceModels` - Allow adapters and data sources to supply Device
  configuration

    *Default*: false

* `Heartbeat` – Overrides the heartbeat interval sent back from the adapter in the
   `* PONG <hb>`. The heartbeat will always be this value in milliseconds.

    *Default*: _None_

* `IgnoreTimestamps` - Overwrite timestamps with the agent time. This will correct
  clock drift but will not give as accurate relative time since it will not take into
  consideration network latencies. This can be overridden on a per adapter basis.

    *Default*: false

* `LegacyTimeout`	- The default length of time an adapter can be silent before it
  is disconnected. This is only for legacy adapters that do not support heartbeats.

    *Default*: 600

* `PreserveUUID` - Do not overwrite the UUID with the UUID from the adapter, preserve
  the UUID in the Devices.xml file. This can be overridden on a per adapter basis.

    *Default*: true

* `ReconnectInterval` - The amount of time between adapter reconnection attempts.
  This is useful for implementation of high performance adapters where availability
  needs to be tracked in near-real-time. Time is specified in milliseconds (ms).

    *Default*: 10000

* `ShdrVersion` - Specifies the SHDR protocol version used by the adapter. When greater than one (1),
  allows multiple complex observations, like `Condition` and `Message` on the same line. If it equials one (1),
  then any observation requiring more than a key/value pair need to be on separate lines. This is the default for all adapters.

    *Default*: 1

* `UpcaseDataItemValue` - Always converts the value of the data items to upper case.

    *Default*: true

#### REST Service Configuration

* `AllowPut`	- Allow HTTP PUT or POST of data item values or assets.

    *Default*: false

* `AllowPutFrom`	- Allow HTTP PUT or POST from a specific host or
  list of hosts. Lists are comma (,) separated and the host names will
  be validated by translating them into IP addresses.

    *Default*: none

* `HttpHeaders`     - Additional headers to add to the HTTP Response for CORS Security

    > Example: ```
    > HttpHeaders {
    >   Access-Control-Allow-Origin = *
    >   Access-Control-Allow-Methods = GET
    >   Access-Control-Allow-Headers = Content-Type
    > }```

* `Port`	- The port number the agent binds to for requests.

    *Default*: 5000

* `ServerIp` - The server IP Address to bind to. Can be used to select the interface in IPV4 or IPV6.

    *Default*: 0.0.0.0


#### Configuration Pameters for TLS (https) Support ####

The following parameters must be present to enable https requests. If there is no password on the certificate, `TlsCertificatePassword` may be omitted.

* `TlsCertificateChain` - The name of the file containing the certificate chain created from signing authority

    *Default*: *NULL*

* `TlsCertificatePassword` -  The password used when creating the certificate. If none was supplied, do not use.

    *Default*: *NULL*

* `TlsClientCAs` - For `TlsVerifyClientCertificate`, specifies a file that contains additional certificate authorities for verification

    *Default*: *NULL*

* `TlsDHKey` -  The name of the file containing the Diffie–Hellman key

    *Default*: *NULL*

* `TlsOnly` -  Only allow secure connections, http requests will be rejected

    *Default*: `false`

* `TlsPrivateKey` -  The name of the file containing the private key for the certificate

    *Default*: *NULL*

* `TlsVerifyClientCertificate` - Request and verify the client certificate against root authorities

    *Default*: false

### MQTT Configuration

* `MqttCaCert` - CA Certificate for MQTT TLS connection to the MTT Broker

    *Default*: *NULL*

* `MqttHost` - IP Address or name of the MQTT Broker

    *Default*: 127.0.0.1

* `MqttPort` - Port number of MQTT Broker

    *Default*: 1883

* `MqttTls` - TLS Certificate for secure connection to the MQTT Broker

    *Default*: `false`

* `MqttWs` - Instructs MQTT to connect using web sockets

    *Default*: `false`

#### MQTT Sink

> **⚠️ Deprecation Notice:** The origional `MqttService` has been deprecated. `MqttService` is now an alias for `Mqtt2Service`. Please use `MqttService` for new configurations. `Mqtt2Service` will continue to work for backward compatibility but may be removed in a future version.

**Deprecated Configuration (still supported):**

Enabled in `agent.cfg` by specifying:

```
Sinks {
  MqttService {
    # Configuration Options below...
  }
}
```

> **Note:** Both `MqttService` and `Mqtt2Service` use the same underlying implementation and support the same configuration options listed below.

#### MQTT Sink Configuration

Enabled in `agent.cfg` by specifying:

```
Sinks {
  Mqtt2Service {
    # Configuration Options...
  }
}
```

* `AssetTopic` - Prefix for the Assets

    *Default*: `MTConnect/Asset/[device]`

* `CurrentTopic` - Prefix for the Current

    *Default*: `MTConnect/Current/[device]`

* `ProbeTopic` or `DeviceTopic` - Prefix for the Device Model topic

    > Note: `[device]` will be replaced with the uuid of each device. Other patterns can be created,
    > for example: `MTConnect/[device]/Probe` will group by device instead of operation.
    > `DeviceTopic` will also work.

    *Default*: `MTConnect/Probe/[device]`

* `SampleTopic` - Prefix for the Sample

    *Default*: `MTConnect/Sample/[device]`

* `MqttLastWillTopic` - The topic used for the last will and testement for an agent

    > Note: The value will be `AVAILABLE` when the Agent is publishing and connected and will
    > publish `UNAVAILABLE` when the agent disconnects from the broker.

    *Default*: `MTConnect/Probe/[device]/Availability"`

* `MqttCurrentInterval` - The frequency to publish currents. Acts like a keyframe in a video stream.

    *Default*: 10000ms

* `MqttSampleInterval` - The frequency to publish samples. Works the same way as the `interval` in the rest call. Groups observations up and publishes with the minimum interval given. If nothing is availble, will wait until an observation arrives to publish.

    *Default*: 500ms

* `MqttSampleCount` - The maxmimum number of observations to publish at one time.

    *Default*: 1000

* `MqttRetain` - For the MQTT Sinks, sets the retain flag for publishing.

    *Default*: True

* `MqttQOS`: - For the MQTT Sinks, sets the Quality of Service. Must be one of `at_least_once`, `at_most_once`, `exactly_once`.

    *Default*: `at_least_once`

* `MqttXPath`: - The xpath filter to apply to all current and samples published to MQTT. If the XPath is invalid, it will fall back to publishing all data items.

    *Default*: All data items

## MQTT Entity Sink Documentation

For detailed configuration, usage, and message format for the MQTT Entity Sink, see: [docs: MTConnect MQTT Entity Sink](src/mtconnect/sink/mqtt_entity_sink/README.md)

### Adapter Configuration Items ###

* `Adapters` - Contains a list of device blocks. If there are no Adapters
  specified and the Devices file contains one device, the Agent defaults
  to an adapter located on the localhost at port 7878.  Data passed from
  the Adapter is associated with the default device.

    *Default*: localhost 7878, associated with the default device

    * `Device` - The name of the device that corresponds to the name of
      the device in the Devices file. Each adapter can map to one
      device. Specifying a "*" will map to the default device.

        *Default*: The name of the block for this adapter or if that is
        not found the default device if only one device is specified
        in the devices file.

    * `Host` - The host the adapter is located on.

        *Default*: localhost

    * `Port` - The port to connect to the adapter.

        *Default*: 7878

    * `Manufacturer` - Replaces the manufacturer attribute in the device XML.

        *Default*: Current value in device XML.

    * `Station` - Replaces the Station attribute in the device XML.

        *Default*: Current value in device XML.

    * `SerialNumber` - Replaces the SerialNumber attribute in the device XML.

        *Default*: Current value in device XML.

    * `UUID` - Replaces the UUID attribute in the device XML.

        *Default*: Current value in device XML.

    * `AutoAvailable` - For devices that do not have the ability to provide available events, if `yes`, this sets the `Availability` to AVAILABLE upon connection.

        *Default*: no (new in 1.2, if AVAILABILITY is not provided for device it will be automatically added and this will default to yes)

    * `AdditionalDevices` - Comma separated list of additional devices connected to this adapter. This provides availability support when one adapter feeds multiple devices.

        *Default*: nothing

    * `FilterDuplicates` - If value is `yes`, filters all duplicate values for data items. This is to support adapters that are not doing proper duplicate filtering.

        *Default*: no

    * `LegacyTimeout` - length of time an adapter can be silent before it
        is disconnected. This is only for legacy adapters that do not support
        heartbeats. If heartbeats are present, this will be ignored.

        *Default*: 600

    * `ReconnectInterval` - The amount of time between adapter reconnection attempts.
       This is useful for implementation of high performance adapters where availability
       needs to be tracked in near-real-time. Time is specified in milliseconds (ms).
       Defaults to the top level ReconnectInterval.

        *Default*: 10000

    * `IgnoreTimestamps` - Overwrite timestamps with the agent time. This will correct
      clock drift but will not give as accurate relative time since it will not take into
      consideration network latencies. This can be overridden on a per adapter basis.

        *Default*: Top Level Setting

    * `PreserveUUID` - Do not overwrite the UUID with the UUID from the adapter, preserve
		  the UUID in the Devices.xml file. This can be overridden on a per adapter basis.

		    *Default*: false

    * `RealTime` - Boost the thread priority of this adapter so that events are handled faster.

        *Default*: false

    * `RelativeTime` - The timestamps will be given as relative offsets represented as a floating
      point number of milliseconds. The offset will be added to the arrival time of the first
      recorded event. If the timestamp is given as a time, the difference between the agent time
      and the fitst incoming timestamp will be used as a constant clock adjustment.

        *Default*: false

    * `ConversionRequired` - Adapter setting for data item units conversion in the agent.
      Assumes the adapter has already done unit conversion. Defaults to global.

        *Default*: Top Level Setting

    * `UpcaseDataItemValue` - Always converts the value of the data items to upper case.

        *Default*: Top Level Setting

    * `ShdrVersion` - Specifies the SHDR protocol version used by the adapter. When greater than one
      (1), allows  multiple complex observations, like `Condition` and `Message` on the same line.
      If it equials one (1), then any observation requiring more than a key/value pair need to be on
      separate lines. Applies to only this adapter.

	    *Default*: 1

    * `SuppressIPAddress` - Suppress the Adapter IP Address and port when creating the Agent Device ids and names.

        *Default*: false

	* `AdapterIdentity` - Adapter Identity name used to prefix dataitems within the Agent device ids and names.

        *Default*:
		* If `SuppressIPAddress` == false:\
		`AdapterIdentity` = ```_ {IP}_{PORT}```\
		example:`_localhost_7878`

		* If `SuppressIPAddress` == true:\
		`AdapterIdentity` = ```_ sha1digest({IP}_{PORT})```\
		example: `__71020ed1ed`

#### MQTT Adapter/Source

* `MqttHost` - IP Address or name of the MQTT Broker

    *Default*: 127.0.0.1

* `MqttPort` - Port number of MQTT Broker

    *Default*: 1883

* `topics` - list of topics to subscribe to. Note : Only raw SHDR strings supported at this time

    *Required*

* `MqttClientId` - Port number of MQTT Broker

    *Default*: Auto-generated

	> **⚠️Note:** Mqtt Sinks and Mqtt Adapters create separate connections to their respective brokers, but currently use the same client ID by default. Because of this, when using a single broker for source and sink, best practice is to explicitly specify their respective `MqttClientId`
	>

	Example mqtt adapter block:
	```
	mydevice {
			Protocol = mqtt
			MqttHost = localhost
			MqttPort = 1883
			MqttClientId = myUniqueID
			Topics = /ingest
		}
	```

* `AdapterIdentity` - Adapter Identity name used to prefix dataitems within the Agent device ids and names.

	*Default*:
	* If `SuppressIPAddress` == false:\
	`AdapterIdentity` = ```_ {IP}_{PORT}```\
	example:`_localhost_7878`

	* If `SuppressIPAddress` == true:\
	`AdapterIdentity` = ```_ sha1digest({IP}_{PORT})```\
	example: `__71020ed1ed`

### Agent Adapter Configuration

* `Url` - The URL of the source agent. `http:` or `https:` are accepted for the protocol.
* `SourceDevice` – The Device name or UUID for the source of the data
* `Count` – the number of items request during a single sample

    *Default*: 1000

* `Polling Interval` – The interval used for streaming or polling in milliseconds

    *Default*: 500ms

* `Reconnect Interval` – The interval between reconnection attampts in milliseconds

    *Default*: 10000ms

* `Use Polling` – Force the adapter to use polling instead of streaming. Only set to `true` if x-multipart-replace blocked.

    *Default*: false

* `Heartbeat` – The heartbeat interval from the server

    *Default*: 10000ms

logger_config configuration items
-----

* `logger_config` - The logging configuration section.

    * `logging_level` - The logging level: `trace`, `debug`, `info`, `warn`,
        `error`, or `fatal`.

        *Default*: `info`

        Note: when running Agent with `agent debug`, `logging_level` will be set to `debug`.

    * `output` - The output file or stream. If using a file, specify
      as: `"file <filename>"`. cout and cerr can be used to specify the
      standard output and standard error streams. *Default*s to the same
      directory as the executable.

        *Default*: file `adapter.log`

    * `max_size` - The maximum log file size. Suffix can be K for kilobytes, M for megabytes, or
      G for gigabytes. No suffix will default to bytes (B). Case is ignored.

        *Default*: 10M

    * `max_index` - The maximum number of log files to keep.

        *Default*: 9

    * `schedule` - The scheduled time to start a new file. Can be DAILY, WEEKLY, or NEVER.

        *Default*: NEVER

</details>

---

# 📁 Directory Structure

> Note: What directory structure is this? Is this the repo or the deploy?

```
cppagent/
├── agent/
├── adapters/
├── scripts/
├── test/
└── samples/
└── web/
```

This layout is designed to separate concerns and make contributions easier.

---

# 🩺 Troubleshooting

| Issue | Cause | Solution |
|-------|--------|----------|
| Missing data | Adapter not connected | Check host/port |
| Device not found | Wrong Devices.xml | Fix file path |
| MQTT ignored | Wrong topic | Check configuration |
| TLS errors | Bad certs | Regenerate certificates |

---

# 🤝 Contributing
We welcome all contributions to the agent, from bug fixes to documentation improvements.

This repository typically follows GitHub Flow:

1. Fork the repository  
2. Create a feature branch  
3. Add/update tests  
4. Ensure builds pass  
5. Submit PR  

---

> Note: We need to have links to the configuration docs for all the options. We can reference the wiki.

# 📝 Version History

### v2.5
- Added new MQTT ingest pipeline  
- Improved Ruby extension lifecycle  
- Updated TLS handling  
- Better error logs  

---

# 🔗 Useful Links

- https://mtconnect.org  
- https://github.com/mtconnect  
- https://github.com/mtconnect/adapter  

---

# 📄 License

Apache License 2.0
