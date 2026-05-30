# IoT-Project — Water Filter Monitoring System
IoT system for real-time monitoring of a water filter, simulating pressure and flow data with an ESP32 and visualising it through a full IoT stack: MQTT → Node-RED → InfluxDB → Grafana.

---

## System Architecture

```
ESP32 (Sensors)
     │
     │  MQTT (broker.mqtt-dashboard.com)
     ▼
Node-RED (flow processing)
     │
     ▼
InfluxDB (time-series storage)
     │
     ▼
Grafana (dashboard & alerts)
```

---

## Hardware

| Component | Function |
|-----------|----------|
| ESP32 | Main microcontroller |
| Potentiometer 1 | Simulates inlet pressure P1 (kPa) |
| Potentiometer 2 | Simulates outlet pressure P2 (kPa) |
| Potentiometer 3 | Simulates water flow rate (%) |
| LED + resistor | Saturation indicator (ON = filter saturated) |

### Wiring Diagram

<img width="730" height="625" alt="image" src="https://github.com/user-attachments/assets/8aab54cd-56db-481a-b793-cba991960b44" />

---

## Repository Structure

```
IoT-Project/
├── README.md
├── wiring_diagram.png                   # Hardware connections (Wokwi)
├── code_water_flow_IoT_project.ino      # ESP32 firmware (Arduino)
├── flows_node-red.json                  # Node-RED complete flow
├── query_filtro_agua.flux               # Flux query used in Grafana
└── grafana_dashboard.json              # Grafana dashboard export
```

---

## Data & MQTT

**Broker:** `broker.mqtt-dashboard.com`

**InfluxDB:**
- Bucket: `IoT_LAB`
- Measurement: `filtro_agua`

| Field | Description |
|-------|-------------|
| `p1_kpa` | Inlet pressure (kPa) |
| `p2_kpa` | Outlet pressure (kPa) |
| `dp_kpa` | Differential pressure P1 − P2 (kPa) |
| `flow_pct` | Estimated flow rate (% of scale) |
| `saturado` | Saturation flag (0 = OK, 1 = saturated) |

| Tag | Values |
|-----|--------|
| `estado` | `OK` \| `SATURADO` |

---

## How It Works

1. The ESP32 reads three potentiometers and maps their values to P1, P2 and flow rate.
2. Differential pressure `dp_kpa = P1 − P2` is computed on-device.
3. When `dp_kpa` exceeds the saturation threshold, the LED turns ON and `estado` is set to `SATURADO`.
4. All values are published via MQTT to the public broker.
5. Node-RED subscribes to the topic, parses the payload, and writes to InfluxDB.
6. Grafana queries InfluxDB using the Flux query in `query_filtro_agua.flux` and displays live panels.

---

## Setup

### ESP32 Firmware

See [`code_water_flow_IoT_project.ino`](code_water_flow_IoT_project.ino).

Dependencies (Arduino IDE / PlatformIO):
- `PubSubClient` — MQTT client
- `ArduinoJson` — JSON serialisation

### Node-RED

Import [`flows_node-red.json`](flows_node-red.json) via **Menu → Import → Clipboard**.

### InfluxDB

Create a bucket named `IoT_LAB`. The Flux query in [`query_filtro_agua.flux`](query_filtro_agua.flux) can be used directly in the Grafana data source panel.

### Grafana

Import [`grafana_dashboard.json`](grafana_dashboard.json) via **Dashboards → Import → Upload JSON file**.

---

## Tools & Technologies

| Tool | Role |
|------|------|
| Arduino IDE | ESP32 firmware development |
| Wokwi | Circuit simulation |
| MQTT | Lightweight messaging protocol |
| Node-RED | Flow-based data processing |
| InfluxDB | Time-series database |
| Grafana | Data visualisation |

---

## Course

Internet of Things (IoT) — MEEC  
Instituto Politécnico de Bragança (IPB)
