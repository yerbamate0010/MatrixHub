# MatrixHub CSI Alarm — implementation plan for Codex Goal Mode

Ten plik jest kontraktem implementacyjnym dla agenta. Celem jest dodanie osobnego alarmu WiFi CSI motion, bez mieszania go z istniejącym RSSI `wifi_motion`, z optymalizacją pod ESP32-S3 + PSRAM i bez blokowania ścieżki CSI.

## 0. Najważniejsza decyzja architektoniczna

Implementacja MVP **nie dodaje drugiego taska tylko dla detektora CSI motion**. W repo jest już osobny statyczny task `CsiService::processingTask()` / `"CsiProcess"`, który pracuje poza Wi-Fi CSI RX callbackiem. Detektor ma działać właśnie tam.

Docelowy przepływ:

```text
Wi-Fi CSI RX callback
    ↓ copy-only, ISR-safe
CsiDataQueue
    ↓ packet storage in PSRAM
CsiProcess task
    ↓ gain compensation + CSI motion detector
CsiMotionSnapshot
    ↓ state-change/keepalive only
AlarmService::submitInput({ wifiCsiMotion = 0.0f/1.0f })
    ↓
normalny alarm pipeline / cooldown / notifications
```

Drugi task `CsiMotionTask` wolno rozważyć dopiero po pomiarach pokazujących realny problem z CPU/latencją w `CsiProcess`. W MVP drugi task byłby gorszy: dodatkowy stack w internal RAM, dodatkowa kolejka, context switch i opóźnienie.

## 1. Zasady nie do negocjacji

1. Nie zmieniaj semantyki istniejącego `wifi_motion`. To zostaje RSSI variance.
2. Dodaj nowe źródło alarmu: `wifi_csi_motion` / `AlarmSource::WifiCsiMotion`.
3. Alarmy nie znają CSI. Do alarmów trafia tylko `wifiCsiMotion = 0.0f | 1.0f`.
4. `motionScore` może iść diagnostycznie przez istniejący CSI wire format, ale alarmy nie używają surowych CSI danych.
5. Nie zmieniaj CSI wire format ani `CSI_HEADER_BYTES = 13`.
6. CSI alarm ma działać headless, bez otwartej strony CSI.
7. Nie dodawaj ciężkiej logiki do `wifi_csi_rx_cb()`.
8. Nie ruszaj `lib/framework/**` ani `src/wifisensing/csi/vendor/**`.
9. Nie wysyłaj notyfikacji bezpośrednio z CSI.
10. Kod, logi, komentarze i identyfikatory pisz po angielsku.
11. Po zmianie layoutu RTC zwiększ `RTC::kSchemaVersion` w `src/system/rtc/RtcConfig.h` z `42` na `43`, jeżeli aktualna wersja nadal wynosi `42`.

## 2. ESP32-S3 / PSRAM / non-blocking contract

Ta sekcja ma pierwszeństwo przed szczegółami funkcjonalnymi. Agent ma ją traktować jako twardy kontrakt.

### 2.1. Task model

Użyj istniejącego taska:

```text
src/wifisensing/csi/core/CsiServiceTask.cpp
CsiService::processingTask()
xTaskCreateStaticPinnedToCore(..., "CsiProcess", ...)
```

Nie hardcoduj priorytetu, rdzenia ani stack size. Używaj istniejących stałych:

```cpp
CONFIG::TASKS::STACK_WIFI_SENSING_CSI
CONFIG::TASKS::PRIO_WIFI_SENSING
CONFIG::TASKS::CORE_WIFI_SENSING
```

Nie przenoś stacka CSI do PSRAM. Aktualny kod słusznie trzyma task stack i TCB w internal DRAM, bo task dotyka ścieżek Wi-Fi/networking-facing. Oszczędzamy internal DRAM na dużych buforach, nie na FreeRTOS control blocks.

### 2.2. Memory placement

| Obiekt | Miejsce | Wymaganie |
|---|---|---|
| CSI queue packet storage | PSRAM | już istnieje, nie zmieniaj na DRAM |
| CSI queue `StaticQueue_t` | internal DRAM | FreeRTOS/ISR bookkeeping |
| CSI batch buffer | PSRAM | już istnieje, nie zmieniaj na DRAM |
| CSI task stack | internal DRAM | zostaje internal |
| CSI task TCB | internal DRAM | zostaje internal |
| detector `energy/mean/m2/noise/valid/top` | PSRAM | alokacja raz, poza hot path |
| detector config/snapshot/counters | internal/default | małe struktury |
| alarm callback data | stack / mała struktura | bez heapu |

Duże tablice detektora nie mogą być bezpośrednimi polami `CsiService`, bo `CsiService` jest tworzony przez `std::make_unique` i kilka KB może trafić do domyślnego heapu. Zrób osobny storage alokowany przez `heap_caps_calloc(..., MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)`.

Preferowany storage:

```cpp
struct CsiMotionStorage {
    float energy[MAX_CSI_SUBCARRIERS];
    float mean[MAX_CSI_SUBCARRIERS];
    float m2[MAX_CSI_SUBCARRIERS];
    float noise[MAX_CSI_SUBCARRIERS];
    uint8_t valid[MAX_CSI_SUBCARRIERS];
    float topScores[32];
};
```

W detektorze:

```cpp
CsiMotionStorage* _storage = nullptr;
```

Na ESP32:

```cpp
_storage = static_cast<CsiMotionStorage*>(heap_caps_calloc(
    1,
    sizeof(CsiMotionStorage),
    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
```

Dla native tests dodaj fallback `calloc/free` za małym wrapperem, żeby testy hostowe nie zależały od `heap_caps_*`.

Jeżeli PSRAM allocation failuje na urządzeniu:

```text
motion state = Unavailable
motion = false
publish false once if needed
CSI streaming still works
log error once / throttled, not per frame
```

Nie rób automatycznego dużego fallbacku do internal DRAM dla detector storage w MVP. Lepiej wyłączyć CSI alarm niż zjeść internal RAM.

### 2.3. Zero allocation in hot path

W tych miejscach nie wolno dodawać alokacji ani ciężkich obiektów:

```text
CsiService::wifi_csi_rx_cb()
CsiDataQueue::pushFromIsr()
CsiService::processingTask() per-frame path
CsiBandMotionDetector::process()
maybePublishMotion()
```

Zakazane w hot path:

```text
heap_caps_malloc / heap_caps_free
new / delete
malloc / free
std::vector / std::map / std::string / Arduino String
JSON serialization/parsing
filesystem writes
network calls
WebSocket send
Telegram / notification / LED / Shelly side effects
logging per frame
pełne sortowanie carrierów
blokujące xSemaphoreTake z wait > 0
```

Dozwolone:

```text
fixed buffers allocated earlier
small stack locals
float math
atomics
portENTER_CRITICAL only for tiny snapshot copy
xSemaphoreTake(..., 0) for non-blocking pending config apply
```

### 2.4. Non-blocking config and snapshot model

Detektor ma być mutowany tylko przez `CsiProcess`. API/settings nie wywołuje `_motionDetector.configure()` ani `_motionDetector.resetBaseline()` bezpośrednio z taska HTTP/config.

Dodaj do `CsiService` model pending mailbox:

```cpp
CsiBandMotionDetector _motionDetector;
MotionCallback _motionCallback = nullptr;

SemaphoreHandle_t _motionConfigMutex = nullptr;
CsiMotionConfig _pendingMotionConfig;
std::atomic<bool> _motionConfigDirty{false};
std::atomic<bool> _motionCalibrationRequested{false};

portMUX_TYPE _motionSnapshotMux = portMUX_INITIALIZER_UNLOCKED;
CsiMotionSnapshot _lastMotionSnapshot;

uint32_t _lastMotionCallbackMs = 0;
bool _lastPublishedMotion = false;
static constexpr uint32_t MOTION_KEEPALIVE_MS = 3000;
```

`setMotionConfig()`:

```text
- bierze `_motionConfigMutex` krótko, poza CSI hot path
- kopiuje config do `_pendingMotionConfig`
- ustawia `_motionConfigDirty = true`
- aktywuje/dezaktywuje `CsiConsumer::AlarmSystem`
- gdy disabled, publikuje false, jeżeli wcześniej było true
```

`requestMotionCalibration()`:

```cpp
_motionCalibrationRequested.store(true, std::memory_order_release);
```

`processingTask()` między pakietami albo przed obróbką pakietu:

```cpp
void CsiService::applyPendingMotionCommandsNonBlocking() {
    if (_motionConfigDirty.load(std::memory_order_acquire)) {
        if (xSemaphoreTake(_motionConfigMutex, 0) == pdTRUE) {
            const CsiMotionConfig cfg = _pendingMotionConfig;
            _motionConfigDirty.store(false, std::memory_order_release);
            xSemaphoreGive(_motionConfigMutex);
            _motionDetector.configure(cfg);
        }
    }

    if (_motionCalibrationRequested.exchange(false, std::memory_order_acq_rel)) {
        _motionDetector.resetBaseline();
    }
}
```

`getMotionSnapshot()`:

```cpp
CsiMotionSnapshot CsiService::getMotionSnapshot() const {
    portENTER_CRITICAL(&_motionSnapshotMux);
    const auto copy = _lastMotionSnapshot;
    portEXIT_CRITICAL(&_motionSnapshotMux);
    return copy;
}
```

Po `process(packet)`:

```cpp
portENTER_CRITICAL(&_motionSnapshotMux);
_lastMotionSnapshot = snapshot;
portEXIT_CRITICAL(&_motionSnapshotMux);
```

Nigdy nie trzymaj mutexa/critical section podczas wywołania callbacka do alarmów.

### 2.5. CPU budget

Dla obecnego `MAX_CSI_DATA_LEN = 512`, `MAX_CSI_SUBCARRIERS = 256`. Per frame target:

```text
O(width + selectedCarrierCount * topK)
width <= 256
topK default 8, clamp max 32
selectedCarrierCount zwykle 8..64
```

W `process()`:

```text
- używaj float, nie double
- używaj fabsf/isfinite/sqrtf
- sqrtf tylko przy finalize baseline, nie na każdej ramce
- nie używaj std::sort na całym spectrum
- top-K przez fixed buffer max 32
- brak logów per frame
```

Dodaj compile-time guard:

```cpp
static_assert(MAX_CSI_ALARM_BANDS == 4, "CSI alarm UI/config assumes max 4 bands");
static_assert(MAX_CSI_SUBCARRIERS <= 256, "Review CSI motion storage before increasing CSI width");
```

### 2.6. Backpressure and degradation

Jeżeli `queueDropsLastSec` rośnie albo stack budget zaczyna ostrzegać:

```text
- nie zwiększaj od razu priorytetu taska
- nie zwiększaj bezmyślnie kolejki
- ogranicz koszt scoringu
- ewentualnie licz globalHighRatio co N ramek
- keepalive motion max co `MOTION_KEEPALIVE_MS`
```

W overload detektor ma preferować `false/noisy/unavailable`, a nie blokować system.

## 3. Algorytm CSI motion detection

### 3.1. Założenie

Nie używamy średniej z całego wykresu. Interesujące zmiany są lokalne, więc score liczymy tylko z wybranych pasm/subcarrierów.

### 3.2. Sygnał wejściowy

Dla każdego subcarriera używaj energii IQ:

```cpp
const float re = static_cast<float>(packet.buf[2 * i]);
const float im = static_cast<float>(packet.buf[2 * i + 1]);
float energy = re * re + im * im;
```

Gain compensation:

```cpp
const float gain = clamp(packet.compensate_gain, 0.25f, 4.0f);
energy *= gain * gain;
```

Jeżeli testy pokażą nadwrażliwość na gain, zostaw stałą/config pozwalającą wyłączyć compensation, ale domyślnie zachowaj zgodność z aktualnym CSI pipeline.

### 3.3. Baseline

MVP używa Welford online mean/stddev per subcarrier. MAD jest odporniejszy na outliery, ale wymaga historii i większego RAM. W pierwszej wersji:

```text
mean[i]
m2[i]
noise[i] = max(sqrtf(m2[i] / (n - 1)), minNoise)
```

Kalibracja:

```text
baselineFrames default 150
motion = false
state = Calibrating
score = 0
```

Po zebraniu baseline:

```text
baselineReady = true
state = Monitoring
valid[i] = baselineMean[i] >= minEnergy && finite(noise[i])
```

Przy zmianie width (`packet.len / 2`) resetuj baseline.

### 3.4. Scoring

Dla zaznaczonych pasm:

```cpp
z[i] = fabsf(energy[i] - mean[i]) / max(noise[i], minNoise);
```

Ignoruj carrier, gdy:

```text
index poza width
valid[i] == false
energy/mean/noise not finite
```

Score:

```text
score = average(top-K z-score from selected bands)
```

Top-K implementuj przez fixed buffer `topScores[32]`. Nie sortuj całego spectrum.

### 3.5. Noise gate

Ruch lokalny powinien podbijać część wybranych pasm. Globalny skok wielu carrierów zwykle oznacza artifact/gain/noise.

`noisy = true`, gdy:

```text
score >= noisyScoreThreshold                default 80.0
validSelectedCarrierCount < max(4, topK / 2)
globalHighRatio >= 0.60 for >= 500 ms       optional in MVP, preferred
```

W `NoisyEnvironment`:

```text
motion = false
publish false if previously true
return to Monitoring only after score < clearThreshold for clearHoldMs
```

### 3.6. Hysteresis and hold

Domyślne wartości:

| Parametr | Default | Clamp |
|---|---:|---:|
| `baselineFrames` | 150 | 30..1000 |
| `enterThreshold` | 6.0 | 1.0..100.0 |
| `clearThreshold` | 3.0 | 0.5..enterThreshold |
| `holdMs` | 1200 | 100..10000 |
| `clearHoldMs` | 2500 | 100..30000 |
| `topK` | 8 | 1..32 |
| `minNoise` | 4.0 | 0.1..1000.0 |
| `minEnergy` | 4.0 | 0.0..10000.0 |
| `noisyScoreThreshold` | 80.0 | enterThreshold..500.0 |
| `autoRecalibration` | true | bool |
| `sensitivity` | 1 / Medium | 0..2 |

State machine:

```text
Disabled
NeedsConfiguration
Calibrating
Monitoring
MotionCandidate
MotionConfirmed
NoisyEnvironment
Unavailable
```

Pseudokod:

```cpp
CsiMotionSnapshot CsiBandMotionDetector::process(const CsiPacket& packet, uint32_t nowMs) {
    if (!_storage) return unavailable();
    if (!_config.enabled) return disabled();

    const uint16_t width = static_cast<uint16_t>(packet.len / 2);
    if (width < 8 || width > MAX_CSI_SUBCARRIERS) return unavailable();
    if (width != _width) resetBaselineForWidth(width);

    if (_config.bandCount == 0) return needsConfiguration();

    computeEnergy(packet, width); // writes _storage->energy

    if (!_baselineReady) {
        accumulateWelford(width);
        if (_baselineFramesSeen >= _config.baselineFrames) finalizeBaseline(width);
        return calibratingSnapshot();
    }

    const ScoreResult scored = scoreSelectedBands(width);
    const bool noisy = isNoisy(scored, nowMs);

    if (noisy) {
        _state = CsiMotionState::NoisyEnvironment;
        _motion = false;
        return snapshot();
    }

    switch (_state) {
        case CsiMotionState::Monitoring:
            if (scored.score >= _config.enterThreshold) {
                _candidateSinceMs = nowMs;
                _state = CsiMotionState::MotionCandidate;
            }
            break;

        case CsiMotionState::MotionCandidate:
            if (scored.score < _config.clearThreshold) {
                _state = CsiMotionState::Monitoring;
                _candidateSinceMs = 0;
            } else if (elapsed(nowMs, _candidateSinceMs) >= _config.holdMs) {
                _motion = true;
                _state = CsiMotionState::MotionConfirmed;
                _clearSinceMs = 0;
            }
            break;

        case CsiMotionState::MotionConfirmed:
            if (scored.score < _config.clearThreshold) {
                if (_clearSinceMs == 0) _clearSinceMs = nowMs;
                if (elapsed(nowMs, _clearSinceMs) >= _config.clearHoldMs) {
                    _motion = false;
                    _state = CsiMotionState::Monitoring;
                    _clearSinceMs = 0;
                }
            } else {
                _clearSinceMs = 0;
            }
            break;

        case CsiMotionState::NoisyEnvironment:
            if (scoreLowForClearHold(nowMs)) {
                _state = CsiMotionState::Monitoring;
            }
            break;

        default:
            _state = CsiMotionState::Monitoring;
            break;
    }

    if (_config.autoRecalibration && !_motion && !noisy && scored.score < _config.clearThreshold) {
        updateBaselineEwma(width, 0.005f);
    }

    return snapshot();
}
```

## 4. Nowe pliki C++

Dodaj:

```text
src/wifisensing/csi/algo/CsiMotionTypes.h
src/wifisensing/csi/algo/CsiBandMotionDetector.h
src/wifisensing/csi/algo/CsiBandMotionDetector.cpp
```

### 4.1. `CsiMotionTypes.h`

```cpp
namespace WIFISENSING::CSI {

constexpr uint8_t MAX_CSI_ALARM_BANDS = 4;
constexpr uint16_t MAX_CSI_SUBCARRIERS = MAX_CSI_DATA_LEN / 2;

struct CsiBandRange {
    uint16_t start = 0;
    uint16_t end = 0; // inclusive
};

enum class CsiMotionState : uint8_t {
    Disabled = 0,
    NeedsConfiguration = 1,
    Calibrating = 2,
    Monitoring = 3,
    MotionCandidate = 4,
    MotionConfirmed = 5,
    NoisyEnvironment = 6,
    Unavailable = 7,
};

struct CsiMotionConfig {
    bool enabled = false;
    uint8_t bandCount = 0;
    CsiBandRange bands[MAX_CSI_ALARM_BANDS]{};
    uint16_t baselineFrames = 150;
    uint8_t topK = 8;
    float enterThreshold = 6.0f;
    float clearThreshold = 3.0f;
    uint16_t holdMs = 1200;
    uint16_t clearHoldMs = 2500;
    float minNoise = 4.0f;
    float minEnergy = 4.0f;
    float noisyScoreThreshold = 80.0f;
    bool autoRecalibration = true;
    uint8_t sensitivity = 1;
};

struct CsiMotionSnapshot {
    CsiMotionState state = CsiMotionState::Disabled;
    bool motion = false;
    bool baselineReady = false;
    bool noisy = false;
    bool needsCalibration = false;
    float score = 0.0f;
    float confidence = 0.0f;
    uint32_t framesSeen = 0;
    uint16_t width = 0;
    uint16_t selectedCarrierCount = 0;
    uint16_t validCarrierCount = 0;
    uint8_t bandCount = 0;
};

const char* toString(CsiMotionState state);

} // namespace WIFISENSING::CSI
```

### 4.2. `CsiBandMotionDetector`

Public API:

```cpp
class CsiBandMotionDetector {
public:
    bool begin();
    void end();
    void configure(const CsiMotionConfig& config);
    void resetBaseline();
    CsiMotionSnapshot process(const CsiPacket& packet, uint32_t nowMs);
    CsiMotionSnapshot snapshot() const;
    bool isEnabled() const;
    bool storageReady() const;

private:
    CsiMotionStorage* _storage = nullptr;
    CsiMotionConfig _config;
    CsiMotionSnapshot _snapshot;
    // counters/timestamps/state only; no big direct arrays
};
```

`begin()` alokuje `CsiMotionStorage` raz. `end()` zwalnia w destruktorze. `process()` nigdy nie alokuje pamięci.

## 5. Integracja z `CsiService`

Zmodyfikuj:

```text
src/wifisensing/csi/core/CsiService.h
src/wifisensing/csi/core/CsiService.cpp
src/wifisensing/csi/core/CsiServiceTask.cpp
src/wifisensing/csi/core/CsiServiceCallback.cpp
```

### 5.1. Public methods

W `CsiService.h` dodaj:

```cpp
void setMotionCallback(MotionCallback cb);
void setMotionConfig(const CsiMotionConfig& config);
void requestMotionCalibration();
CsiMotionSnapshot getMotionSnapshot() const;
```

### 5.2. Lifecycle

Konstruktor:

```cpp
_motionConfigMutex = xSemaphoreCreateMutex();
_motionSnapshotMux = portMUX_INITIALIZER_UNLOCKED;
```

`begin()`:

```cpp
if (!_motionDetector.begin()) {
    LOGE("Failed to allocate CSI motion detector buffers in PSRAM");
}
```

Destruktor:

```cpp
_motionDetector.end();
if (_motionConfigMutex) vSemaphoreDelete(_motionConfigMutex);
```

### 5.3. Processing task changes

W `CsiService::processingTask()` zastąp obecne:

```cpp
packet.isMotionDetected = false;
packet.motionScore = 0.0f;
```

wywołaniem detektora:

```cpp
self->applyPendingMotionCommandsNonBlocking();
packet.compensate_gain = self->_gainCtrl.update(&(packet.rx_ctrl));
const auto motion = self->processMotionPacket(packet, now);
packet.motionScore = motion.score;
packet.isMotionDetected = motion.motion;
self->publishMotionSnapshot(motion);
self->maybePublishMotion(motion, now);
```

Zastosuj to w obu miejscach: pierwszy `pop()` oraz drain burst loop.

`maybePublishMotion()`:

```cpp
const bool changed = snapshot.motion != _lastPublishedMotion;
const bool keepalive = snapshot.motion && (nowMs - _lastMotionCallbackMs >= MOTION_KEEPALIVE_MS);

if (changed || keepalive) {
    MotionCallback cb = getMotionCallbackSnapshot();
    _lastPublishedMotion = snapshot.motion;
    _lastMotionCallbackMs = nowMs;
    if (cb) cb(snapshot.motion);
}
```

Nie trzymaj locka podczas `cb(...)`.

## 6. Konfiguracja RTC i JSON

Zmodyfikuj:

```text
src/system/rtc/types/RtcWifiSensingTypes.h
src/system/rtc/RtcDefaultValues.h
src/system/rtc/RtcConfig.h
src/config/json/ConfigKeys.h
src/config/json/WifiSensingConfigJson.h
src/config/json/WifiSensingConfigJson.cpp
src/wifisensing/WifiSensingSettings.h
src/wifisensing/WifiSensingSettings.cpp
```

### 6.1. RTC fields

Dodaj do `WifiSensingData` płaskie pola:

```cpp
bool csiAlarmEnabled;
uint8_t csiAlarmBandCount;
uint16_t csiAlarmBandStart[4];
uint16_t csiAlarmBandEnd[4];
uint16_t csiBaselineFrames;
uint8_t csiTopK;
float csiEnterThreshold;
float csiClearThreshold;
uint16_t csiHoldMs;
uint16_t csiClearHoldMs;
float csiMinNoise;
float csiMinEnergy;
float csiNoisyThreshold;
bool csiAutoRecalibration;
uint8_t csiSensitivity;
```

Defaults:

```cpp
constexpr bool CsiAlarmEnabled = false;
constexpr uint16_t CsiBaselineFrames = 150;
constexpr uint8_t CsiTopK = 8;
constexpr float CsiEnterThreshold = 6.0f;
constexpr float CsiClearThreshold = 3.0f;
constexpr uint16_t CsiHoldMs = 1200;
constexpr uint16_t CsiClearHoldMs = 2500;
constexpr float CsiMinNoise = 4.0f;
constexpr float CsiMinEnergy = 4.0f;
constexpr float CsiNoisyThreshold = 80.0f;
constexpr bool CsiAutoRecalibration = true;
constexpr uint8_t CsiSensitivity = 1;
```

### 6.2. JSON shape

`/api/wifisensing/config`:

```json
{
  "enabled": false,
  "sample_interval_ms": 200,
  "variance_threshold": 4.0,
  "csi_alarm": {
    "enabled": false,
    "bands": [
      { "start": 58, "end": 70 }
    ],
    "baseline_frames": 150,
    "top_k": 8,
    "enter_threshold": 6.0,
    "clear_threshold": 3.0,
    "hold_ms": 1200,
    "clear_hold_ms": 2500,
    "min_noise": 4.0,
    "min_energy": 4.0,
    "noisy_threshold": 80.0,
    "auto_recalibration": true,
    "sensitivity": 1
  }
}
```

Walidacja:

```text
bands max 4
start/end clamp 0..255
if start > end swap
top_k clamp 1..32
baseline_frames clamp 30..1000
enter_threshold clamp 1.0..100.0
clear_threshold clamp 0.5..enter_threshold
hold_ms clamp 100..10000
clear_hold_ms clamp 100..30000
min_noise clamp 0.1..1000.0
min_energy clamp 0.0..10000.0
noisy_threshold clamp enter_threshold..500.0
sensitivity clamp 0..2
```

Brak `csi_alarm` w JSON = backward compatibility, zostaw defaults/current values.

### 6.3. Runtime apply

`WifiSensingSettings` ma dostać `CsiService*` i aplikować CSI alarm niezależnie od RSSI `enabled`.

```cpp
WifiSensingSettings(FS* fs,
                    WIFISENSING::WifiSensingService* service,
                    WIFISENSING::CSI::CsiService* csiService);
```

Ważne: toggle RSSI sensing nie może wyłączać CSI alarmu. CSI alarm ma własne `csi_alarm.enabled`.

## 7. API backend

Zmodyfikuj:

```text
src/api/wifisensing/WifiSensingApiService.h
src/api/wifisensing/WifiSensingApiService.cpp
```

### 7.1. Status

Rozszerz `/api/wifisensing/status` o:

```json
"csi": {
  "motion": {
    "enabled": true,
    "state": "monitoring",
    "baseline_ready": true,
    "detected": false,
    "noisy": false,
    "needs_calibration": false,
    "score": 1.25,
    "confidence": 0.0,
    "frames_seen": 1820,
    "width": 192,
    "band_count": 2,
    "selected_carriers": 24,
    "valid_carriers": 90
  }
}
```

Nie usuwaj istniejących pól `csi.*`.

### 7.2. Calibrate endpoint

Dodaj:

```text
POST /api/wifisensing/csi/calibrate
admin only
```

Handler ma tylko ustawić calibration request, nie robić kalibracji synchronicznie.

Response OK:

```json
{ "ok": true, "state": "calibrating" }
```

Gdy CSI service unavailable:

```json
{ "ok": false, "error": "csi_service_unavailable" }
```

## 8. Integracja z alarmami

Zmodyfikuj:

```text
src/alarms/types/AlarmEnums.h
src/alarms/types/AlarmInputData.h
src/alarms/engine/AlarmEvaluator.h
src/alarms/serialization/AlarmEnumConverters.h
src/config/json/AlarmConfigJson.cpp
src/alarms/core/AlarmCoordinator.h
src/alarms/core/AlarmCoordinator.cpp
src/alarms/AlarmService.h
src/alarms/AlarmService.cpp
src/alarms/notifications/AlarmMessageBuilder.h
src/notifications/telegram/commands/AlarmsCommand.cpp
```

### 8.1. Enum/source

```cpp
enum class AlarmSource : uint8_t {
    CO2 = 0,
    Temperature = 1,
    Humidity = 2,
    WifiMotion = 3,
    BleTemperature = 4,
    BleHumidity = 5,
    BleBattery = 6,
    BleRssi = 7,
    WifiCsiMotion = 8,
};
```

String mapping:

```text
WifiCsiMotion <-> "wifi_csi_motion"
```

### 8.2. Input aggregation

`AlarmInputData.h`:

```cpp
float wifiCsiMotion = NAN;
```

`AlarmService::AggregatedAlarmInput`:

```cpp
float wifiCsiMotion = NAN;
```

`AlarmService` private:

```cpp
float _lastWifiCsiMotion = NAN;
```

`submitInput()`:

```cpp
if (!std::isnan(inputData.wifiCsiMotion)) {
    _lastWifiCsiMotion = inputData.wifiCsiMotion;
    hasUpdates = true;
}
```

`submitInput()` jest akceptowalny z CSI callbacka, bo obecny model tylko kopiuje snapshot pod krótkim `portMUX` i ustawia pending evaluation. Nie wolno wołać `processPending()` z CSI.

### 8.3. Evaluator

```cpp
case AlarmSource::WifiCsiMotion:
    return sensors.wifiCsiMotion;
```

Reguła techniczna:

```text
operator = above
threshold = 0.5
```

UI ma to pokazać jako boolean: `CSI motion detected`, bez suwaka float.

## 9. Wiring w ServiceRegistry

Zmodyfikuj:

```text
src/system/services/ServiceRegistry.h
src/system/services/ServiceRegistryCallbacks.cpp
src/system/init/services/CoreServicesInitializer.cpp
```

W `wireRuntimeCallbacks()`:

```cpp
if (_csiService && _alarmService) {
    _csiService->setMotionCallback([this](bool motion) {
        ALARMS::AlarmInputData input;
        input.wifiCsiMotion = motion ? 1.0f : 0.0f;
        _alarmService->submitInput(input);
    });
}
```

W `detachRuntimeCallbacks()`:

```cpp
if (_csiService) {
    _csiService->setMotionCallback(nullptr);
}
```

Użyj faktycznych nazw pól/guardów z repo. Nie dodawaj notyfikacji ani LED tutaj.

## 10. Frontend API/types

Zmodyfikuj:

```text
interface/src/lib/types/connectivity/wifiSensing.ts
interface/src/lib/services/api/connectivity/WifiSensingApiService.ts
interface/src/lib/types/domain/alarms.ts
interface/src/lib/features/alarms/components/alarmThresholdConfig.ts
interface/messages/en.json
interface/messages/pl.json
```

Dodaj typy:

```ts
export interface CsiAlarmBand {
  start: number;
  end: number;
}

export interface CsiAlarmSettings {
  enabled: boolean;
  bands: CsiAlarmBand[];
  baseline_frames: number;
  top_k: number;
  enter_threshold: number;
  clear_threshold: number;
  hold_ms: number;
  clear_hold_ms: number;
  min_noise: number;
  min_energy: number;
  noisy_threshold: number;
  auto_recalibration: boolean;
  sensitivity: 0 | 1 | 2;
}

export interface CsiMotionStatus {
  enabled: boolean;
  state: string;
  baseline_ready: boolean;
  detected: boolean;
  noisy: boolean;
  needs_calibration: boolean;
  score: number;
  confidence: number;
  frames_seen: number;
  width: number;
  band_count: number;
  selected_carriers: number;
  valid_carriers: number;
}
```

API method:

```ts
async calibrateCsiAlarm(): Promise<{ ok: boolean; state?: string; error?: string }> {
  return this.client.post('/api/wifisensing/csi/calibrate', {}, {
    signal: AbortSignal.timeout(WifiSensingApiService.SAVE_TIMEOUT_MS)
  });
}
```

## 11. Frontend CSI page UX

Zmodyfikuj:

```text
interface/src/routes/wifisensing/csi/+page.svelte
interface/src/lib/features/wifisensing/csi/CsiAmplitudeChart.svelte
interface/src/lib/features/wifisensing/csi/useCsiAmplitudeChart.svelte.ts
interface/src/lib/features/wifisensing/csi/CsiWaterfallChart.svelte
interface/src/lib/features/wifisensing/csi/useCsiWaterfallChart.svelte.ts
```

Dodaj:

```text
interface/src/lib/features/wifisensing/csi/useCsiAlarmConfig.svelte.ts
interface/src/lib/features/wifisensing/csi/CsiAlarmControls.svelte
```

Panel:

```text
CSI Alarm
[ ] Enabled
Status: disabled/calibrating/monitoring/motion/noisy/needs calibration
Score: x.xx
Bands: [58-70] [125-132] [+]
[Select band]
[Calibrate empty room]
Sensitivity: Low / Medium / High
Advanced: topK, hold, clear hold, thresholds, min noise
[Save]
```

Konflikt z uPlot drag-to-zoom:

```text
selectionMode = false → obecny drag-to-zoom działa jak teraz
selectionMode = true  → transparent overlay obsługuje pointer events i wybór pasma
```

Nie wyłączaj zoomu permanentnie.

Mapowanie X → subcarrier index:

```text
ratio = clamp((clientX - plotLeft) / plotWidth, 0, 1)
index = round(ratio * (subcarrierCount - 1))
```

Overlay amplitude/waterfall:

```text
left = start / subcarrierCount * 100%
width = (end - start + 1) / subcarrierCount * 100%
```

Sensitivity mapping:

```text
Low    -> enter 8.0, clear 4.0
Medium -> enter 6.0, clear 3.0
High   -> enter 4.5, clear 2.2
```

## 12. Frontend alarm source

Dodaj source:

```ts
'wifi_csi_motion'
```

Metadata:

```text
label: CSI Motion
unit: none
booleanLike: true
operator: above
threshold: 0.5
```

Formularz alarmu dla `wifi_csi_motion`:

```text
- ukryj/zablokuj operator
- ukryj/zablokuj threshold
- tekst: CSI motion detected
```

## 13. Backend tests

Dodaj/zmodyfikuj:

```text
test/test_csi_band_motion_detector/test_csi_band_motion_detector.cpp
test/test_wifi_sensing_config_json/test_wifi_sensing_config_json.cpp
test/test_alarm_enum_converters/test_alarm_enum_converters.cpp
test/test_alarm_config_json/test_alarm_config_json.cpp
test/test_alarm_evaluator/test_alarm_evaluator.cpp
test/test_alarm_message_builder/test_alarm_message_builder.cpp
test/test_service_registry_initialization/test_service_registry_initialization.cpp
```

Detector tests:

```text
disabled_returns_disabled_no_motion
storage_allocation_failure_returns_unavailable
no_bands_returns_needs_configuration
baseline_converges_after_configured_frames
narrow_band_motion_triggers_after_hold
single_frame_spike_does_not_trigger
motion_clears_after_clear_hold
global_noise_enters_noisy_environment
width_change_resets_baseline
dead_carriers_are_ignored
selected_band_only_detects_selected_band_changes
process_does_not_allocate_after_begin, if existing test infra can assert this
```

Config tests:

```text
save includes nested csi_alarm
missing csi_alarm keeps defaults
max 4 bands
reversed start/end normalized
thresholds clamped
top_k clamped
```

Alarm tests:

```text
WifiCsiMotion <-> wifi_csi_motion
evaluator triggers above 0.5 for 1.0
evaluator does not trigger for 0.0
message builder has sane label/unit
```

## 14. Frontend tests

Dodaj/zmodyfikuj:

```text
interface/src/lib/features/wifisensing/csi/useCsiAmplitudeChart.test.svelte.ts
interface/src/lib/features/wifisensing/csi/useCsiConnection.test.svelte.ts
interface/src/lib/features/alarms/components/useAlarmRuleForm.test.svelte.ts
interface/src/lib/features/alarms/components/AlarmRuleList.test.ts
```

Scenariusze:

```text
parseCsiFrame nadal czyta motionScore/isMotionDetected
selected band zapisuje start/end rosnąco
selection mode nie psuje normalnego zoomu
wifi_csi_motion ustawia above/0.5
lista reguł pokazuje boolean text zamiast >0.5
```

## 15. Komendy weryfikacyjne

Uruchamiaj po fazach:

```bash
$HOME/.platformio/penv/bin/pio test -e native -f test_csi_band_motion_detector
$HOME/.platformio/penv/bin/pio test -e native -f test_wifi_sensing_config_json
$HOME/.platformio/penv/bin/pio test -e native -f test_alarm_evaluator
$HOME/.platformio/penv/bin/pio test -e native -f test_alarm_enum_converters
$HOME/.platformio/penv/bin/pio test -e native -f test_alarm_config_json
$HOME/.platformio/penv/bin/pio test -e native -f test_alarm_message_builder
python -m unittest test/test_csi_alarm_harness.py
cd interface && npm run i18n:build && npm run check && npm run test:run
./scripts/build-fast.sh
```

Dodatkowa kontrola hot path:

```bash
grep -R "std::vector\|String\|heap_caps_malloc\|new " -n src/wifisensing/csi/algo src/wifisensing/csi/core
```

Wynik nie może pokazywać alokacji w `process()` ani per-frame path. Alokacje w `begin()/end()/startProcessingTask()` są akceptowalne.

## 16. Manualny test na urządzeniu

1. Flash firmware.
2. Wejdź na `/wifisensing/csi`.
3. Włącz CSI Alarm.
4. Włącz `Select band`.
5. Zaznacz 1–2 stabilne pasma reagujące lokalnie.
6. Zapisz config.
7. Opuść pomieszczenie / nie ruszaj się.
8. Kliknij `Calibrate empty room`.
9. Czekaj na `baseline_ready` / `monitoring`.
10. Porusz się w pomieszczeniu.
11. Sprawdź `motion.detected == true` i alarm `wifi_csi_motion`.
12. Przestań się ruszać i sprawdź powrót do false.
13. Zamknij stronę CSI i powtórz test. Alarm musi działać headless.
14. Reboot: config pasm zostaje, baseline kalibruje się od nowa.
15. Obserwuj `queueDropsLastSec`, `packetsPerSec`, stack budget i heap telemetry.

## 17. Ryzyka i oczekiwana obsługa

| Ryzyko | Obsługa |
|---|---|
| Statyczne piki | Nie triggerują, score jest zmianą względem baseline |
| Globalny gain/noise jump | `NoisyEnvironment`, motion false |
| Martwe/null carriery | `minEnergy` + valid mask |
| Za mało carrierów w paśmie | `NeedsConfiguration` albo motion false |
| Width mismatch | reset baseline |
| Brak strony CSI | `CsiConsumer::AlarmSystem` utrzymuje CSI aktywne |
| Single callback frontend | osobny `MotionCallback`, nie `_csiCallback` |
| Packet loss | hold/hysteresis minimalizuje dropy |
| Za szybka adaptacja baseline | EWMA tylko no motion/no noisy/score low |
| PSRAM allocation fail | `Unavailable`, motion false, CSI streaming działa dalej |
| RSSI regression | nie ruszać `wifi_motion` |

## 18. Acceptance criteria

Gotowe, gdy:

1. `wifi_motion` nadal działa jak wcześniej na RSSI variance.
2. Nowe źródło `wifi_csi_motion` działa end-to-end: backend enum, JSON, frontend, evaluator.
3. CSI detector działa w `CsiProcess` i uzupełnia `packet.motionScore` oraz `packet.isMotionDetected`.
4. CSI wire format nie zmienia rozmiaru ani kolejności pól.
5. CSI alarm działa bez otwartej strony CSI.
6. Wybrane pasma zapisują się pod `csi_alarm.bands`.
7. `POST /api/wifisensing/csi/calibrate` resetuje baseline asynchronicznie/non-blocking.
8. `/api/wifisensing/status` pokazuje `csi.motion`.
9. Alarmy dostają tylko `wifiCsiMotion = 0.0f | 1.0f`.
10. Nie ma bezpośredniej notyfikacji/Telegram/LED z CSI.
11. Duże bufory detektora są alokowane raz w PSRAM.
12. `CsiBandMotionDetector::process()` nie alokuje pamięci.
13. Nie dodano osobnego taska CSI motion w MVP.
14. Config/calibration API nie blokuje taska CSI.
15. Nie zmieniono `lib/framework/**` ani `src/wifisensing/csi/vendor/**`.
16. Native tests, frontend checks i `./scripts/build-fast.sh` przechodzą.

## 19. Kolejność implementacji

### Phase 1 — Detector pure C++

1. Dodaj `CsiMotionTypes.h`.
2. Dodaj `CsiBandMotionDetector.h/.cpp`.
3. Dodaj `CsiMotionStorage` alokowany raz w PSRAM z native fallback.
4. Zaimplementuj Welford baseline, z-score, selected bands, top-K, hysteresis, noise gate.
5. Dodaj testy native detektora.
6. Uruchom tylko test detektora.

Nie dotykaj jeszcze alarmów ani UI.

### Phase 2 — CSI Service integration

1. Dodaj detector member i motion callback do `CsiService`.
2. Dodaj pending config/calibration mailbox.
3. Podepnij detector w `processingTask()`.
4. Dodaj motion fields do `CsiMetricsSnapshot`.
5. Utrzymaj WebSocket CSI bez zmian layoutu.
6. Uruchom test detektora i wire format, jeśli istnieje.

### Phase 3 — Config/API

1. Rozszerz RTC struct + schema version.
2. Dodaj JSON nested `csi_alarm`.
3. Rozszerz `WifiSensingSettings` o `CsiService*` i runtime apply.
4. Dodaj status `csi.motion`.
5. Dodaj calibrate endpoint.
6. Testy config JSON.

### Phase 4 — Alarm source

1. Dodaj `AlarmSource::WifiCsiMotion`.
2. Dodaj `wifiCsiMotion` do input aggregation.
3. Dodaj enum conversions i config parsing.
4. Podepnij `ServiceRegistryCallbacks` motion callback do `AlarmService::submitInput()`.
5. Testy alarm evaluator/config/message.

### Phase 5 — Frontend CSI UX

1. Typy API.
2. API method `calibrateCsiAlarm()`.
3. `useCsiAlarmConfig.svelte.ts`.
4. `CsiAlarmControls.svelte`.
5. Band selection overlay w amplitude chart.
6. Overlay pasm w waterfall.
7. Status badges.
8. i18n build.

### Phase 6 — Frontend alarms

1. Dodaj `wifi_csi_motion` do alarm source list.
2. Boolean-like rule form.
3. Rule list label.
4. Testy frontend.

### Phase 7 — Full verification

1. Native tests.
2. Python harness test.
3. Frontend checks.
4. Fast firmware build.
5. Manual checklist.

## 20. Prompt do Codex Goal Mode

```text
Pracuj w repo MatrixHub. Najpierw przeczytaj AGENTS.md oraz csi_alarm.md w root repo. Zaimplementuj WiFi CSI motion alarm dokładnie według planu z csi_alarm.md.

Cel:
Dodać osobne źródło alarmu `wifi_csi_motion`, oparte o WiFi CSI band motion detection. Nie mieszać z istniejącym `wifi_motion` RSSI variance. CSI ma mieć własny detektor, własną konfigurację `csi_alarm`, UI do zaznaczania pasm/subcarrierów i endpoint kalibracji. System alarmów ma dostawać tylko boolean-like input `wifiCsiMotion = 0.0f/1.0f`.

Twarde ograniczenia:
- Nie ruszaj `lib/framework/**`.
- Nie ruszaj `src/wifisensing/csi/vendor/**`.
- Nie zmieniaj CSI wire format ani `CSI_HEADER_BYTES = 13`.
- Nie zmieniaj semantyki istniejącego `wifi_motion`.
- Nie wysyłaj notyfikacji bezpośrednio z CSI.
- CSI alarm musi działać headless, bez otwartej strony CSI.
- Optymalizacja ESP32-S3/PSRAM jest wymagana.
- Nie dodawaj osobnego taska dla detektora w MVP; użyj istniejącego `CsiProcess`.
- Zero heap allocation/new/vector/String/JSON w CSI hot path.
- Duże bufory detektora alokuj raz w PSRAM; task stack/TCB/FreeRTOS control blocks zostają w internal DRAM.
- Config/calibration API nie może blokować per-frame CSI processing; użyj pending mailbox i apply w `CsiProcess`.

Wymagania techniczne:
- Dodaj `CsiBandMotionDetector` z PSRAM-backed `CsiMotionStorage`, baseline Welford, per-subcarrier z-score, selected bands, top-K scoring, hysteresis, hold/clear hold, noisy gate i recalibration guard.
- `CsiBandMotionDetector::process()` ma mieć zero alokacji i deterministyczny runtime.
- Podepnij detektor w `CsiService::processingTask()` i wypełniaj `packet.motionScore` oraz `packet.isMotionDetected`.
- Dodaj `CsiService::setMotionCallback`, `setMotionConfig`, `requestMotionCalibration`, `getMotionSnapshot`.
- Rozszerz `/api/wifisensing/config` o nested `csi_alarm` i `/api/wifisensing/status` o `csi.motion`.
- Dodaj `POST /api/wifisensing/csi/calibrate` jako non-blocking request.
- Dodaj `AlarmSource::WifiCsiMotion` string `wifi_csi_motion` i pełne mapowanie backend/frontend.
- UI alarmów ma traktować `wifi_csi_motion` jako boolean: operator `above`, threshold `0.5`, bez suwaka float dla użytkownika.
- Strona CSI ma mieć wybór pasm myszką bez psucia obecnego drag-to-zoom: selection mode używa overlay, normalny tryb zostawia zoom.

Po każdym większym etapie uruchamiaj najwęższe testy. Na końcu uruchom:
$HOME/.platformio/penv/bin/pio test -e native -f test_csi_band_motion_detector
$HOME/.platformio/penv/bin/pio test -e native -f test_wifi_sensing_config_json
$HOME/.platformio/penv/bin/pio test -e native -f test_alarm_evaluator
$HOME/.platformio/penv/bin/pio test -e native -f test_alarm_enum_converters
$HOME/.platformio/penv/bin/pio test -e native -f test_alarm_config_json
$HOME/.platformio/penv/bin/pio test -e native -f test_alarm_message_builder
python -m unittest test/test_csi_alarm_harness.py
cd interface && npm run i18n:build && npm run check && npm run test:run
./scripts/build-fast.sh

Jeżeli któryś test/build failuje przez rzecz spoza zakresu, zatrzymaj się i opisz dokładnie: komenda, błąd, plik, podejrzana przyczyna. Nie rób szerokiego refactoru. Trzymaj zmiany scoped i zgodne z istniejącymi wzorcami repo.
```
