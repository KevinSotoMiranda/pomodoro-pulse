# PomodoroPulse
**A Physical Kinetic Focus Tracker for Remote Workers & Students**

> Flip the block. Start the timer. Stay off your screen.

---

## Table of Contents

- [Project Overview](#project-overview)
- [Tech Stack](#tech-stack)
- [System Architecture](#system-architecture)
- [Bill of Materials](#bill-of-materials)
- [Build Checklist](#build-checklist)
  - [Phase 1 · Hardware Validation](#phase-1--hardware-validation)
  - [Phase 2 · Embedded State Machine + NVS](#phase-2--embedded-state-machine--nvs)
  - [Phase 3 · Wi-Fi, MQTT & Idempotent Sync](#phase-3--wi-fi-mqtt--idempotent-sync)
  - [Phase 4 · FastAPI Backend + InfluxDB](#phase-4--fastapi-backend--influxdb)
  - [Phase 5 · React Dashboard + Grafana](#phase-5--react-dashboard--grafana)
- [Learning Resources](#learning-resources)
- [Resume Bullets](#resume-bullets)

---

## Project Overview

PomodoroPulse is a 6-sided wooden or 3D-printed block that acts as a physical Pomodoro
and habit tracker. Each face of the block represents a focus state — Coding, Writing,
Break, Meeting, Deep Work, or Idle. Flipping the block depresses a face-specific switch,
triggering the ESP32 to log the session, start a countdown timer, and publish the event
to the cloud over MQTT.

**The problem it solves:** Software-based focus timers live on the same screen causing
the distraction. A single alt-tab destroys focus. PomodoroPulse is entirely off-screen.

---

## Tech Stack

| Layer | Technology | Reasoning |
|---|---|---|
| Microcontroller | ESP32 (FreeRTOS, C++) | Dual-core, built-in Wi-Fi, NVS Flash for offline cache, rich GPIO |
| Connectivity | MQTT QoS 1 over TLS | Lightweight, guaranteed-delivery protocol — industry standard for IoT |
| MQTT Broker | HiveMQ Cloud (Free Tier) | Managed, TLS-secured broker — no infrastructure to maintain |
| Backend | **FastAPI + paho-mqtt** (Fly.io) | Async, type-safe Python; Pydantic v2 validation; mature InfluxDB client ecosystem |
| Database | InfluxDB Cloud (Free Tier) | Purpose-built for time-series; Flux query language for productivity aggregations |
| Frontend | React + Vite + TypeScript (Vercel) | Fast, modern stack; free Vercel deploy on every push to `main` |
| Analytics | Grafana Cloud (Free Tier) | Native InfluxDB connector; production-grade charts embedded via iframe |

> **Why FastAPI over Express.js?**
> Python dominates data engineering at big tech companies. FastAPI's Pydantic v2 validation
> on incoming IoT payloads demonstrates software engineering discipline. The Python
> `influxdb-client` library is also more mature and better documented than its Node.js
> equivalent. Both languages are valid — Python tells a more coherent story when InfluxDB
> and data pipelines are the centrepiece.

> **Why Fly.io over Render?**
> Render's free tier spins services down after 15 minutes of inactivity, causing 30-second+
> cold starts during demos. Fly.io's free tier keeps one small machine running persistently.
> Add a `/health` cron ping as a safety net regardless.

> **Node-RED has been removed entirely.** It is a low-code visual drag-and-drop tool.
> Answering "walk me through your backend" with "I used Node-RED flows" signals you avoided
> writing code where the backend logic lives. The FastAPI service replaces it with ~150 lines
> of real, type-safe, testable Python.

---

## System Architecture

```
┌─────────────────────────────────────────┐
│            Physical Block               │
│  6x Button Switches (geometric layout)  │
│  RGB LED · Active Buzzer                │
└──────────────┬──────────────────────────┘
               │ GPIO Interrupt → FreeRTOS Queue
               ▼
┌─────────────────────────────────────────┐
│         ESP32 Edge Device               │
│                                         │
│  StateMachineTask  (high priority)      │
│   └─ debouncing, state transitions,     │
│      LED colour, buzzer tone            │
│                                         │
│  TimerTask         (high priority)      │
│   └─ FreeRTOS tick-based Pomodoro       │
│      countdown, drift-free timing       │
│                                         │
│  CommsSyncTask     (low priority)       │
│   └─ Wi-Fi manager, NVS flush,          │
│      MQTT QoS 1 publish                 │
│                                         │
│  NVS Flash Cache                        │
│   └─ offline session log on every       │
│      state change; survives power loss  │
└──────────────┬──────────────────────────┘
               │ MQTT QoS 1 / TLS / Port 8883
               ▼
┌─────────────────────────────────────────┐
│         HiveMQ Cloud Broker             │
│  topic: ppp/{device_id}/session         │
└──────────────┬──────────────────────────┘
               │ paho-mqtt subscriber
               │ (background thread in FastAPI lifespan)
               ▼
┌─────────────────────────────────────────┐
│      FastAPI Backend  (Fly.io)          │
│                                         │
│  • Pydantic v2 payload validation       │
│  • Idempotent InfluxDB writes           │
│    (session_id as dedup tag)            │
│  • Out-of-order packet handling         │
│    (start_ts as point timestamp)        │
│                                         │
│  GET /sessions?range=7d                 │
│  GET /streaks                           │
│  GET /health                            │
└──────────────┬──────────────────────────┘
               │ influxdb-client / Flux queries
               ▼
┌─────────────────────────────────────────┐
│         InfluxDB Cloud                  │
│  pre-aggregated productivity metrics    │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│      React Dashboard  (Vercel)          │
│                                         │
│  LiveStatus card    (polls /status 5 s) │
│  DailySummary chart (Recharts)          │
│  StreakTracker      (GET /streaks)      │
│  Grafana iframe embed                   │
│   └─ time-series · pie · stat panels   │
└─────────────────────────────────────────┘
```

---

## Bill of Materials

| Component | Source | Notes |
|---|---|---|
| ESP32 DevKit | From kit | Dual-core 240 MHz, Wi-Fi, 520 KB SRAM, 4 MB Flash |
| 6× Tactile Button Switches | From kit | Arranged geometrically inside enclosure |
| RGB LED (common cathode) | From kit | PWM-driven, colour maps to focus state |
| Active Buzzer (3.3 V) | From kit | End-of-cycle audio alert |
| Balsa wood or cardboard | ~$2 | 6-sided enclosure. 3D print filament works equally well. |
| **Total added cost** | **~$2** | All other components sourced from the hardware kit |

---

## Build Checklist

> These checkboxes are interactive on GitHub (`- [ ]` / `- [x]`).
> Work through phases sequentially — each phase is a dependency for the next.

---

### Phase 1 · Hardware Validation
*Confirm every physical component works in isolation before writing a single line of
application logic. Skipping this phase causes hours of debugging later.*

- [x] Install ESP-IDF **or** Arduino IDE with the ESP32 board support package
- [x] Wire all 6 button switches to separate GPIO pins; verify continuity at each pin with a multimeter before powering on
- [x] Write a blocking GPIO read loop and confirm each pin registers a press via the serial monitor
- [x] Implement a 10 ms software debounce filter; run 100 rapid flips per switch and verify zero ghost presses in the serial log
- [x] Wire the RGB LED; drive each colour channel independently via PWM and confirm no dead channels
- [ ] Wire the active buzzer; trigger a 200 ms tone on button press and confirm audible output
- [ ] Map each of the 6 switches to a named face state: `CODING`, `WRITING`, `BREAK`, `MEETING`, `DEEP_WORK`, `IDLE`
- [ ] Place the physical block flat on each face and confirm only one GPIO reads HIGH at a time (geometric/physical validation)

---

### Phase 2 · Embedded State Machine + NVS
*Build the FreeRTOS task architecture, the software state machine, and the offline-first
NVS Flash persistence layer. This is the hardest phase — do not rush it.*

- [ ] **Design on paper first:** draw all 6 states, every valid transition, and every guard condition before writing any C++ code
- [ ] Scaffold three FreeRTOS tasks: `StateMachineTask` (high priority), `TimerTask` (high priority), `CommsSyncTask` (low priority)
- [ ] Implement GPIO input via interrupts: the ISR posts an event to a `xQueueSendFromISR` queue; `StateMachineTask` pops from it — zero polling loops anywhere
- [ ] Implement the Pomodoro countdown timer inside `TimerTask` using `xTaskGetTickCount()` for drift-free, high-resolution timing
- [ ] Wire state transitions to RGB LED colour changes and distinct buzzer tone patterns per state
- [ ] Initialize NVS: call `nvs_flash_init()` on boot and open a `ppp_sessions` namespace for session records
- [ ] Write a `log_session()` function: on every state transition, persist `{ timestamp_ms, state_id, duration_ms }` to NVS Flash
- [ ] Write a `load_unsynced_sessions()` function: on boot, read all unsynced NVS entries into a `FreeRTOS Queue` for `CommsSyncTask` to consume
- [ ] **Stress test:** simulate 50+ rapid state flips with Wi-Fi disabled; power-cycle the board hard; verify every record is still in NVS
- [ ] Audit all inter-task communication: confirm zero shared globals without a mutex or semaphore guarding them

---

### Phase 3 · Wi-Fi, MQTT & Idempotent Sync
*Connect the device to the cloud and implement the offline-to-online sync routine that
makes the data pipeline resilient. The idempotent sync logic is a key interview talking point.*

- [ ] Implement a Wi-Fi connection manager inside `CommsSyncTask`: connect on boot, auto-reconnect on drop, exponential backoff with a 60 s ceiling
- [ ] Use a `FreeRTOS EventGroup` to broadcast `WIFI_CONNECTED` / `WIFI_DISCONNECTED` bits so other tasks can react to connectivity changes
- [ ] Connect to HiveMQ Cloud on port **8883 (TLS)**; embed the CA certificate and verify the TLS handshake succeeds before publishing
- [ ] Define the JSON payload schema and stick to it:
  ```json
  {
    "device_id": "ppp-001",
    "session_id": "<UUIDv4>",
    "state": "CODING",
    "start_ts": "2025-07-04T09:00:00Z",
    "end_ts":   "2025-07-04T09:25:00Z",
    "duration_secs": 1500
  }
  ```
- [ ] Publish live state-change events to MQTT topic `ppp/{device_id}/session` with **QoS 1** (at-least-once delivery)
- [ ] On Wi-Fi reconnect: flush the NVS queue in **strict chronological order**, re-publishing all unsynced back-dated sessions
- [ ] Mark an NVS entry as synced **only after receiving the MQTT `PUBACK`** — not on the return of `esp_mqtt_client_publish()`
- [ ] **Integration test:** disable Wi-Fi at the router level, run 10 Pomodoro sessions, reconnect, verify all 10 arrive on the HiveMQ dashboard with correct `start_ts` timestamps

---

### Phase 4 · FastAPI Backend + InfluxDB
*Build the type-safe server-side pipeline. Every engineering decision here — Pydantic
validation, idempotent writes, out-of-order handling — is a concrete interview talking point.*

- [ ] Create a Python virtual environment; install: `fastapi`, `uvicorn[standard]`, `paho-mqtt`, `influxdb-client`, `pydantic` (v2)
- [ ] Define Pydantic v2 models for the MQTT payload; add field validators (e.g. `duration_secs > 0`, `state` must be a valid enum member)
- [ ] Register a paho-mqtt subscriber in FastAPI's **`lifespan` context manager**: connect to HiveMQ on application start, clean disconnect on shutdown
- [ ] In the subscriber callback: parse the raw JSON, validate against the Pydantic model, log and silently discard malformed packets without crashing the thread
- [ ] Write validated sessions to InfluxDB using `session_id` **as a tag field** — InfluxDB will deduplicate on re-writes, making the pipeline idempotent against NVS replay
- [ ] Use the payload's `start_ts` as the **InfluxDB point timestamp** — never the server arrival time; this is what correctly stitches out-of-order back-dated sessions
- [ ] Build `GET /sessions?range=7d` — Flux query returning daily total focus minutes grouped by `state` tag
- [ ] Build `GET /streaks` — query InfluxDB for daily activity records; compute current streak and all-time longest streak in Python
- [ ] Build `GET /health` — returns `{ "status": "ok" }` as a lightweight liveness probe
- [ ] Write `docker-compose.yml` for local development: one FastAPI container + one InfluxDB OSS container
- [ ] Deploy to Fly.io: `fly launch` → `fly deploy`; add a cron job (e.g. `cron-job.org`) to ping `GET /health` every 5 minutes to prevent free-tier hibernation
- [ ] Provision InfluxDB Cloud free tier; set Fly.io secrets: `INFLUXDB_URL`, `INFLUXDB_TOKEN`, `INFLUXDB_ORG`, `INFLUXDB_BUCKET`

---

### Phase 5 · React Dashboard + Grafana
*Build the user-facing interface. Keep the native React components lightweight and use
Grafana for the heavy time-series charting.*

- [ ] Scaffold the React app: `npm create vite@latest -- --template react-ts`
- [ ] Build `<LiveStatus />`: polls `GET /status` every 5 s; renders the active state name, its mapped colour, and a live countdown timer
- [ ] Build `<DailySummary />`: horizontal stacked bar chart of focus minutes per state from `GET /sessions?range=1d`, built with Recharts
- [ ] Build `<StreakTracker />`: flame icon, current daily streak count, all-time best streak — data from `GET /streaks`
- [ ] Provision Grafana Cloud free account; add InfluxDB Cloud as a data source using the **Flux** query language
- [ ] Build a Grafana dashboard with three panels: time-series (daily focus minutes over 30 days), pie chart (% time per state), stat panel (total focus hours this week)
- [ ] Embed the Grafana dashboard in React via `<iframe src="...&kiosk&theme=dark" />` — the `kiosk` param removes the Grafana toolbar for a clean embed
- [ ] Serve a Wi-Fi provisioning page directly from the ESP32's built-in HTTP server: a captive portal form to configure SSID and password without re-flashing
- [ ] Deploy to Vercel: push code to GitHub → connect repo in Vercel dashboard → confirm auto-deploy fires on push to `main`
- [ ] Write the project `README.md`: architecture diagram, wiring schematic photo, step-by-step setup guide, Grafana dashboard screenshots, and a short screen-recorded demo GIF

---

## Learning Resources

### ESP32 / Embedded C++

- **[ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/)**
  Ground truth for all FreeRTOS, NVS, Wi-Fi, and MQTT APIs. Do not rely on tutorials
  for anything FreeRTOS-related — go to this source directly.

- **[The FreeRTOS Book (free PDF)](https://www.freertos.org/Documentation/RTOS_book.html)**
  Genuinely excellent. Read the chapters on tasks, queues, event groups, and semaphores
  before you write a single task.

- **Random Nerd Tutorials** — fine for initial GPIO wiring diagrams; stop there.
  The application-level code is not production quality.

### MQTT Protocol

- **[HiveMQ MQTT Essentials (11-part series)](https://www.hivemq.com/mqtt-essentials/)**
  The best written introduction to the protocol. Read all 11 parts before writing
  any MQTT code. QoS levels, retained messages, and Last Will are all covered.

### FastAPI + Python Backend

- **[FastAPI Official Docs](https://fastapi.tiangolo.com/)**
  Focus on: Pydantic v2 integration, the `lifespan` context manager, background tasks,
  and dependency injection.

- **[influxdb-client-python](https://github.com/influxdata/influxdb-client-python)**
  Official Influx Python client. Read the write API and Flux query examples closely.

- **[paho-mqtt Python Client](https://eclipse.dev/paho/files/paho.mqtt.python/html/)**
  Official docs. Pay attention to the threading model — the `on_message` callback fires
  on paho's internal thread, not FastAPI's event loop.

### InfluxDB + Flux

- **[Flux Query Language Docs](https://docs.influxdata.com/flux/v0/)**
  Time-series querying has a fundamentally different mental model from SQL. Spend
  real time here before writing the `/sessions` and `/streaks` endpoints.

### React + Grafana

- **[react.dev](https://react.dev)** — Official React docs. Use the Vite + TypeScript template;
  the `useEffect` and `useState` sections are what you need most.

- **[Recharts](https://recharts.org/en-US/)** — For the native React `<DailySummary />` chart component.

- **[Grafana Iframe Embedding](https://grafana.com/docs/grafana/latest/developers/embedding/)**
  Official embedding docs. The `kiosk` and `theme` URL parameters are documented here.

---

## Updated System Architecture Blueprint

```
[Physical Switches / RGB LED / Active Buzzer]
         │  (GPIO Debounced Interrupts → FreeRTOS Queue)
         ▼
[Edge: ESP32 — FreeRTOS Tasks + NVS Flash Cache]
         │  (MQTT QoS 1 / TLS / HiveMQ Cloud)
         ▼
[Cloud: HiveMQ Broker] ──► [Backend: FastAPI + paho-mqtt — Fly.io]
                                        │  (Pydantic v2 · influxdb-client)
                                        ▼
                               [TSDB: InfluxDB Cloud]
                                        │  (Flux queries)
                                        ▼
                            [UI: React + Grafana Embed — Vercel]
```

---

*PomodoroPulse — Built from scratch. No Node-RED. No drag-and-drop. No shortcuts.*
