# 🚀 MTConnect C++ Agent
A modern and extensible C++ implementation of the MTConnect standard, designed to reliably collect machine data from diverse industrial devices and expose it in a consistent, structured format. The agent normalizes raw device signals, ensures compliance with the MTConnect standard, and makes the data accessible to client applications such as MES systems, monitoring dashboards, analytics platforms, or custom integrations.

The agent supports multiple industrial communication models (SHDR, JSON, MQTT), multiple devices, secure network transport, and pluggable data transformation logic, making it suitable for both small installations and large-scale production environments.

---

# 📑 Table of Contents

- [Overview](#overview)
- [Key Features](#key-features)
- [Architecture](#architecture)
- [Quick Start](#quick-start)
- [Installation](#installation)
  - [Linux Build](#linux-build)
  - [Windows Build](#windows-build)
  - [macOS Build](#macos-build)
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

This section helps developers get up and running with the agent in minutes.

### 1. Clone the Repository
```bash
git clone https://github.com/mtconnect/cppagent.git
cd cppagent
```

### 2. Build Using Conan
```bash
conan create . --build=missing
```

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
```bash
./agent run agent.cfg
```

### 5. Verify Output
- http://localhost:5000/probe  
- http://localhost:5000/current  
- http://localhost:5000/sample  

---

# 🛠 Installation

> **Note: We may want start with the download and install instructions, most people are not developers.**

<details>
<summary><b>Downloading Pre-Built Binaries</b></summary>
Pre-built binaries for Windows and Linux platforms are available on the [Releases](https://github.com/mtconnect/cppagent/releases) page.

## Instructions for using Windows Binaries
1. Download the latest `.zip` release for Windows.
2. Extract the contents to a desired location.
3. Open a Command Prompt and navigate to the extracted folder.
4. Run the agent with a configuration file:
   ```powershell
   .\agent.exe run agent.cfg
   ```
</details>
<br/>

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

---

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
