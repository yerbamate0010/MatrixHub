# MatrixHub CSI Alarm — plan implementacyjny dla Codex Goal Mode

Ten plik jest kontraktem implementacyjnym. Agent ma prowadzić zmiany krok po kroku, bez mieszania CSI z istniejącym RSSI `wifi_motion` i bez ruszania vendor/framework.

## 0. Zasady nie do negocjacji

1. **Nie zmieniaj semantyki istniejącego `wifi_motion`.** To zostaje RSSI variance.
2. **Dodaj nowe źródło alarmu:** `wifi_csi_motion` / `AlarmSource::WifiCsiMotion`.
3. **Alarmy nie znają CSI.** Do `AlarmService` trafia tylko `wifiCsiMotion = 0.0f | 1.0f`.
4. **Nie zmieniaj CSI wire format.** `CsiWireFormat.cpp` i frontend parser mają już `motionScore` oraz `isMotionDetected`; rozmiar nagłówka `13` zostaje bez zmian.
5. **CSI alarm ma działać headless.** Otwarta strona `/wifisensing/csi` nie może być wymagana do działania alarmów.
6. **Nie dodawaj ciężkiej logiki do ISR callback.** `wifi_csi_rx_cb` ma dalej tylko kopiować dane do kolejki.
7. **Nie ruszaj:** `lib/framework/**`, `src/wifisensing/csi/vendor/**`.
8. **Nie wysyłaj notyfikacji bezpośrednio z CSI.** CSI publikuje stan do `AlarmService::submitInput()`, a alarmy robią resztę.
9. **Kod, komentarze, logi i identyfikatory po angielsku.** Komunikacja z użytkownikiem po polsku.
10. **Po zmianie layoutu RTC increment:** `src/system/rtc/RtcConfig.h`, `RTC::kSchemaVersion` z `42` na `43`.

## 1. Architektura docelowa

```text
CSI RX callback
    ↓
CsiDataQueue
    ↓
CsiService::processingTask()
    ↓
CsiBandMotionDetector
    ↓
CsiMotionSnapshot { state, score, motion, noisy, baselineReady }
    ↓
CsiService::MotionCallback(bool motion)
    ↓
ServiceRegistryCallbacks
    ↓
AlarmService::submitInput({ wifiCsiMotion = 0.0f/1.0f })
    ↓
AlarmEvaluator / cooldown / notifications / LED / Telegram
```

Najważniejszy podział odpowiedzialności:

| Warstwa | Odpowiedzialność | Nie robi |
|---|---|---|
| `CsiBandMotionDetector` | baseline, score, hysteresis, noisy gate | alarmów, WebSocketów, JSON |
| `CsiService` | odpalenie detektora na pakietach CSI, runtime status, callback motion | notyfikacji, reguł alarmowych |
| `WifiSensingSettings` / JSON | trwała konfiguracja `csi_alarm` | obliczeń CSI |
| `WifiSensingApiService` | status, config endpoint, calibrate endpoint | logiki detekcji |
| Alarmy | boolean input i reguły | subcarrierów, progów CSI, baseline |
| Frontend CSI | wybór pasm, kalibracja, diagnostyka | oceny alarmu po stronie przeglądarki |

## 2. Algorytm CSI motion detection

### 2.1. Założenie

Nie używamy średniej z całego wykresu. Ruch ma być wykrywany na wybranych pasmach subcarrierów, bo interesujące zmiany są lokalne, a globalna średnia je rozmywa.

### 2.2. Sygnał wejściowy

Dla każdego subcarriera używaj energii IQ:

```cpp
energy[i] = real * real + imag * imag;
```

Nie używaj `sqrt()` w detektorze. Jest wolniejsze i nie jest potrzebne do detekcji względnej.

Gain compensation:

```cpp
const float gain = clamp(packet.compensate_gain, 0.25f, 4.0f);
energy[i] *= gain * gain;
```

Jeżeli testy pokażą nadwrażliwość na gain, zostaw konfigurację/stałą pozwalającą wyłączyć mnożenie przez `gain * gain`, ale domyślnie trzymaj spójność z aktualnym CSI pipeline.

### 2.3. Baseline

MVP: Welford online mean/stddev per subcarrier.

MAD jest bardziej odporny na outliery, ale wymaga buforowania historii per subcarrier i jest droższy pamięciowo. Dlatego w pierwszej implementacji:

```text
baselineMean[i]
baselineM2[i]
baselineNoise[i] = sqrt(variance)
```

Kalibracja:

```text
baselineFrames = 150
stan = Calibrating
motion = false
score = 0
```

Po zebraniu `baselineFrames`:

```text
baselineReady = true
stan = Monitoring
noise[i] = max(sqrt(m2 / (n - 1)), minNoise)
valid[i] = baselineMean[i] >= minEnergy && noise[i] jest finite
```

W przypadku zmiany liczby subcarrierów (`packet.len / 2`) resetuj baseline.

### 2.4. Scoring per frame

Dla zaznaczonych pasm:

```cpp
z[i] = abs(energy[i] - baselineMean[i]) / max(baselineNoise[i], minNoise);
```

Bierz tylko indeksy z wybranych pasm. Ignoruj carrier, jeżeli:

```text
valid[i] == false
energy[i] nie jest finite
index poza aktualną szerokością CSI
```

Score:

```text
score = średnia z top-K największych z-score w wybranych pasmach
```

Nie sortuj całej tablicy. Utrzymuj mały bufor `top[32]` i insertuj tylko kandydatów.

Domyślne `topK = 8`.

### 2.5. Noise gate

Ruch lokalny powinien dotyczyć wybranych pasm. Bardzo duży globalny skok wielu carrierów częściej oznacza zakłócenie/gain/packet artifact niż osobę.

Wykryj `noisy` gdy spełnione jest jedno z:

```text
score >= noisyScoreThreshold                  // default 80.0
allCarrierHighRatio >= 0.60 przez >= 500 ms   // opcjonalnie w MVP, ale preferowane
validSelectedCarrierCount < max(4, topK / 2)
```

W stanie `NoisyEnvironment`:

```text
motion = false
publikuj false do alarmów, jeżeli wcześniej było true
czekaj aż score < clearThreshold przez clearHoldMs
potem wróć do Monitoring albo NeedsCalibration
```

### 2.6. Hysteresis i hold

Nie triggeruj od pojedynczej ramki.

Domyślne wartości:

| Parametr | Default | Uwagi |
|---|---:|---|
| `baselineFrames` | `150` | około kilkanaście sekund przy obecnym rate |
| `enterThreshold` | `6.0` | medium sensitivity |
| `clearThreshold` | `3.0` | histereza |
| `holdMs` | `1200` | motion candidate musi trwać |
| `clearHoldMs` | `2500` | stabilne wyjście z motion |
| `topK` | `8` | max 32 |
| `minNoise` | `4.0` | w jednostkach energii, stroić manualnie |
| `minEnergy` | `4.0` | ignorowanie martwych/null carrierów |
| `noisyScoreThreshold` | `80.0` | globalny artifact |
| `motionKeepaliveMs` | `3000` | ponowne `true` do alarmów gdy motion trwa |

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
CsiMotionSnapshot process(packet, nowMs) {
    if (!config.enabled) return disabled();

    width = packet.len / 2;
    if (width < 8) return unavailable();
    if (width != _width) resetBaseline(width);

    if (config.bandCount == 0) return needsConfiguration();

    computeEnergy(packet, width);

    if (!_baselineReady) {
        accumulateBaseline(_energy, width);
        if (_baselineFramesSeen >= config.baselineFrames) finalizeBaseline();
        return calibrating();
    }

    score = scoreSelectedBands(_energy, width);
    globalHighRatio = computeGlobalHighRatioIfCheap(_energy, width);

    if (isNoisy(score, globalHighRatio)) {
        _state = CsiMotionState::NoisyEnvironment;
        _motion = false;
        return snapshot();
    }

    switch (_state) {
        case Monitoring:
            if (score >= enterThreshold) {
                _candidateSinceMs = nowMs;
                _state = MotionCandidate;
            }
            break;

        case MotionCandidate:
            if (score < clearThreshold) {
                _state = Monitoring;
                _candidateSinceMs = 0;
            } else if (nowMs - _candidateSinceMs >= holdMs) {
                _motion = true;
                _state = MotionConfirmed;
                _lastMotionChangeMs = nowMs;
            }
            break;

        case MotionConfirmed:
            if (score < clearThreshold) {
                if (_clearSinceMs == 0) _clearSinceMs = nowMs;
                if (nowMs - _clearSinceMs >= clearHoldMs) {
                    _motion = false;
                    _state = Monitoring;
                    _clearSinceMs = 0;
                    _lastMotionChangeMs = nowMs;
                }
            } else {
                _clearSinceMs = 0;
            }
            break;

        case NoisyEnvironment:
            if (score < clearThreshold for clearHoldMs) {
                _state = Monitoring;
            }
            break;
    }

    if (!_motion && !isNoisy && score < clearThreshold && config.autoRecalibration) {
        updateBaselineEwma(_energy, alpha = 0.005f);
    }

    return snapshot();
}
```

### 2.7. Auto recalibration

Po kalibracji aktualizuj baseline bardzo wolno tylko gdy:

```text
motion == false
noisy == false
score < clearThreshold
baselineReady == true
```

EWMA:

```cpp
mean[i] = mean[i] * (1 - alpha) + energy[i] * alpha;
noise[i] = noise[i] * (1 - alpha) + abs(energy[i] - mean[i]) * alpha;
noise[i] = max(noise[i], minNoise);
```

Nie adaptuj baseline podczas motion ani noisy, bo „nauczysz” detektor ruchu jako nowego tła.

## 3. Nowe pliki C++

Dodaj:

```text
src/wifisensing/csi/algo/CsiMotionTypes.h
src/wifisensing/csi/algo/CsiBandMotionDetector.h
src/wifisensing/csi/algo/CsiBandMotionDetector.cpp
```

### 3.1. `CsiMotionTypes.h`

Docelowe typy:

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

### 3.2. `CsiBandMotionDetector`

Public API:

```cpp
class CsiBandMotionDetector {
public:
    void configure(const CsiMotionConfig& config);
    void resetBaseline();
    CsiMotionSnapshot process(const CsiPacket& packet, uint32_t nowMs);
    CsiMotionSnapshot snapshot() const;
    bool isEnabled() const;

private:
    // fixed-size arrays, no heap allocation
};
```

Wewnętrzne tablice jako pola klasy, nie stack:

```cpp
float _energy[MAX_CSI_SUBCARRIERS];
float _mean[MAX_CSI_SUBCARRIERS];
float _m2[MAX_CSI_SUBCARRIERS];
float _noise[MAX_CSI_SUBCARRIERS];
bool _valid[MAX_CSI_SUBCARRIERS];
```

Uwaga: to jest około kilka KB RAM, akceptowalne. Nie dodawaj dynamicznych `std::vector` w hot path.

## 4. Integracja z `CsiService`

Zmodyfikuj:

```text
src/wifisensing/csi/core/CsiService.h
src/wifisensing/csi/core/CsiService.cpp
src/wifisensing/csi/core/CsiServiceTask.cpp
```

### 4.1. `CsiService.h`

Dodaj include:

```cpp
#include "../algo/CsiBandMotionDetector.h"
```

`MotionCallback` już istnieje w `src/wifisensing/csi/data/CsiTypes.h`. Nie duplikuj typedefu.

Dodaj public methods:

```cpp
void setMotionCallback(MotionCallback cb);
void setMotionConfig(const CsiMotionConfig& config);
void requestMotionCalibration();
CsiMotionSnapshot getMotionSnapshot() const;
```

Dodaj private members:

```cpp
CsiBandMotionDetector _motionDetector;
MotionCallback _motionCallback = nullptr;
SemaphoreHandle_t _motionMutex = nullptr;
uint32_t _lastMotionCallbackMs = 0;
bool _lastPublishedMotion = false;
static constexpr uint32_t MOTION_KEEPALIVE_MS = 3000;
```

Jeżeli chcesz oszczędzić mutex, możesz użyć `_callbackMutex` dla callbacków i `_stateMutex` dla config/snapshot, ale czytelniej dodać `_motionMutex`.

### 4.2. Lifecycle

W konstruktorze:

```cpp
_motionMutex = xSemaphoreCreateMutex();
```

W destruktorze:

```cpp
if (_motionMutex) vSemaphoreDelete(_motionMutex);
```

### 4.3. Config i kalibracja

`setMotionConfig()`:

1. lock `_motionMutex`,
2. `_motionDetector.configure(config)`,
3. unlock,
4. `setConsumerActive(CsiConsumer::AlarmSystem, config.enabled)`,
5. jeśli disabled, opublikuj `false` do alarmów, jeżeli wcześniej było true.

`requestMotionCalibration()`:

1. lock `_motionMutex`,
2. `_motionDetector.resetBaseline()`,
3. unlock.

`getMotionSnapshot()`:

1. lock `_motionMutex`,
2. return `_motionDetector.snapshot()`.

### 4.4. Processing task

W `CsiService::processingTask()` zastąp obecne:

```cpp
packet.isMotionDetected = false;
packet.motionScore = 0.0f;
```

wywołaniem pomocniczym.

Dodaj prywatną metodę albo lokalny helper:

```cpp
CsiMotionSnapshot CsiService::processMotionPacket(CsiPacket& packet, uint32_t nowMs);
void CsiService::maybePublishMotion(const CsiMotionSnapshot& snapshot, uint32_t nowMs);
```

Logika:

```cpp
packet.compensate_gain = self->_gainCtrl.update(&(packet.rx_ctrl));
const auto motion = self->processMotionPacket(packet, now);
packet.motionScore = motion.score;
packet.isMotionDetected = motion.motion;
self->maybePublishMotion(motion, now);
```

Zastosuj to w obu miejscach: pierwszy `pop()` i pętla drain burst.

`maybePublishMotion()`:

```cpp
const bool changed = snapshot.motion != _lastPublishedMotion;
const bool keepalive = snapshot.motion && (nowMs - _lastMotionCallbackMs >= MOTION_KEEPALIVE_MS);
const bool forceClear = !snapshot.motion && _lastPublishedMotion;

if (changed || keepalive || forceClear) {
    MotionCallback cb = getMotionCallbackSnapshot();
    if (cb) cb(snapshot.motion);
    _lastPublishedMotion = snapshot.motion;
    _lastMotionCallbackMs = nowMs;
}
```

Callback może wywołać `AlarmService::submitInput()`, ale nie może odpalać pełnej ewaluacji alarmów inline.

## 5. Konfiguracja RTC i JSON

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

### 5.1. RTC struct

W `RtcWifiSensingTypes.h` rozszerz `WifiSensingData` o płaskie pola CSI. JSON ma być zagnieżdżony, ale RTC struct może zostać prosty i packed.

```cpp
static constexpr uint8_t kMaxCsiAlarmBands = 4;

struct __attribute__((packed)) WifiSensingData {
    bool enabled = Defaults::WifiSensing::Enabled;
    uint16_t sampleIntervalMs = Defaults::WifiSensing::SampleIntervalMs;
    float varianceThreshold = Defaults::WifiSensing::VarianceThreshold;

    bool csiAlarmEnabled = Defaults::WifiSensing::CsiAlarmEnabled;
    uint8_t csiAlarmBandCount = 0;
    uint16_t csiAlarmBandStart[kMaxCsiAlarmBands] = {0, 0, 0, 0};
    uint16_t csiAlarmBandEnd[kMaxCsiAlarmBands] = {0, 0, 0, 0};
    uint16_t csiBaselineFrames = Defaults::WifiSensing::CsiBaselineFrames;
    uint8_t csiTopK = Defaults::WifiSensing::CsiTopK;
    float csiEnterThreshold = Defaults::WifiSensing::CsiEnterThreshold;
    float csiClearThreshold = Defaults::WifiSensing::CsiClearThreshold;
    uint16_t csiHoldMs = Defaults::WifiSensing::CsiHoldMs;
    uint16_t csiClearHoldMs = Defaults::WifiSensing::CsiClearHoldMs;
    float csiMinNoise = Defaults::WifiSensing::CsiMinNoise;
    float csiMinEnergy = Defaults::WifiSensing::CsiMinEnergy;
    float csiNoisyThreshold = Defaults::WifiSensing::CsiNoisyThreshold;
    bool csiAutoRecalibration = Defaults::WifiSensing::CsiAutoRecalibration;
    uint8_t csiSensitivity = Defaults::WifiSensing::CsiSensitivity;
};
```

W `RtcDefaultValues.h` dodaj defaults w namespace `RTC::Defaults::WifiSensing`:

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

W `RtcConfig.h`:

```cpp
constexpr uint32_t kSchemaVersion = 43;
```

Dodaj komentarz w historii schema, jeżeli plik ma taką sekcję.

### 5.2. JSON shape

`/api/wifisensing/config` ma zwracać:

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

Dodaj klucze w `ConfigKeys.h`:

```cpp
constexpr const char* kCsiAlarm = "csi_alarm";
constexpr const char* kBands = "bands";
constexpr const char* kStart = "start";
constexpr const char* kEnd = "end";
constexpr const char* kBaselineFrames = "baseline_frames";
constexpr const char* kTopK = "top_k";
constexpr const char* kEnterThreshold = "enter_threshold";
constexpr const char* kClearThreshold = "clear_threshold";
constexpr const char* kHoldMs = "hold_ms";
constexpr const char* kClearHoldMs = "clear_hold_ms";
constexpr const char* kMinNoise = "min_noise";
constexpr const char* kMinEnergy = "min_energy";
constexpr const char* kNoisyThreshold = "noisy_threshold";
constexpr const char* kAutoRecalibration = "auto_recalibration";
constexpr const char* kSensitivity = "sensitivity";
```

Jeżeli istnieją podobne klucze globalne, nie duplikuj nazw — użyj istniejących.

### 5.3. Walidacja JSON

W `deserializeWifiSensing()`:

```text
- brak csi_alarm = backward compatibility, zachowaj defaults/current values
- bands: max 4
- start/end clamp 0..255
- jeżeli start > end, zamień
- pomiń pasma o width 0 tylko jeżeli są poza zakresem po clampie
- top_k clamp 1..32
- baseline_frames clamp 30..1000
- enter_threshold clamp 1.0..100.0
- clear_threshold clamp 0.5..enter_threshold
- hold_ms clamp 100..10000
- clear_hold_ms clamp 100..30000
- min_noise clamp 0.1..1000.0
- min_energy clamp 0.0..10000.0
- noisy_threshold clamp enter_threshold..500.0
- sensitivity clamp 0..2
```

Opcjonalnie sortuj i merge’uj overlapping bands, ale nie jest to wymagane w MVP. Jeżeli robisz merge, zachowaj limit 4 po merge.

### 5.4. Runtime apply

`WifiSensingSettings` obecnie przyjmuje tylko `WifiSensingService*`. Rozszerz konstruktor:

```cpp
WifiSensingSettings(FS* fs,
                    WIFISENSING::WifiSensingService* service,
                    WIFISENSING::CSI::CsiService* csiService);
```

Dodaj forward declaration `namespace CSI { class CsiService; }`.

W `begin()`:

```text
- zastosuj RSSI sensing tylko gdy settings.enabled == true
- zastosuj CSI alarm niezależnie od RSSI settings.enabled
```

To ważne: CSI alarm nie może zależeć od toggle’a RSSI WiFi sensing.

Dodaj helper:

```cpp
static WIFISENSING::CSI::CsiMotionConfig buildCsiMotionConfig(const RTC::WifiSensingData& state);
bool applyCsiRuntimeState(const RTC::WifiSensingData& state);
```

`applyCsiRuntimeState()`:

```cpp
if (!_csiService) return true;
_csiService->setMotionConfig(buildCsiMotionConfig(state));
return true;
```

W `onConfigUpdated()` rollback musi obejmować RSSI i CSI. Jeżeli zapis configu nie przejdzie, przywróć runtime do `previousState`.

## 6. API backend

Zmodyfikuj:

```text
src/api/wifisensing/WifiSensingApiService.h
src/api/wifisensing/WifiSensingApiService.cpp
```

### 6.1. Status

Rozszerz `CsiMetricsSnapshot` w `CsiService.h` o pola motion:

```cpp
bool motionEnabled = false;
bool motionBaselineReady = false;
bool motionDetected = false;
bool motionNoisy = false;
bool motionNeedsCalibration = false;
float motionScore = 0.0f;
float motionConfidence = 0.0f;
uint32_t motionFramesSeen = 0;
uint16_t motionWidth = 0;
uint16_t motionSelectedCarrierCount = 0;
uint16_t motionValidCarrierCount = 0;
uint8_t motionBandCount = 0;
const char* motionState = "disabled";
```

W `getMetricsSnapshot()` pobierz `CsiMotionSnapshot` i uzupełnij pola.

W `/api/wifisensing/status` pod `csi` dodaj:

```json
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
```

Nie usuwaj istniejących pól `csi.*`.

### 6.2. Calibrate endpoint

Dodaj endpoint:

```text
POST /api/wifisensing/csi/calibrate
admin only
```

W `WifiSensingApiService.h`:

```cpp
esp_err_t handleCalibrateCsiAlarm(PsychicRequest* request);
```

W `begin()`:

```cpp
_server->on("/api/wifisensing/csi/calibrate", HTTP_POST,
    wrapAuth([this](PsychicRequest* request) {
        return handleCalibrateCsiAlarm(request);
    }, AuthenticationPredicates::IS_ADMIN));
```

Dopasuj dokładną sygnaturę `wrapAuth` do istniejących wzorców w repo.

Response:

```json
{
  "ok": true,
  "state": "calibrating"
}
```

Jeżeli `_csiService == nullptr`:

```json
{ "ok": false, "error": "csi_service_unavailable" }
```

i status HTTP odpowiedni do istniejących praktyk w API.

## 7. Integracja z alarmami

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

### 7.1. Enum/source

`AlarmEnums.h`:

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

### 7.2. Alarm input

`AlarmInputData.h`:

```cpp
float wifiCsiMotion = NAN;
```

`AlarmService::AggregatedAlarmInput`:

```cpp
float wifiCsiMotion = NAN;
```

`AlarmService` private field:

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

`processPending()` pass-through to coordinator.

### 7.3. Evaluator

`AlarmEvaluator::getSensorValue()`:

```cpp
case AlarmSource::WifiCsiMotion:
    return sensors.wifiCsiMotion;
```

Jeżeli `AlarmEvaluator` dostaje osobne parametry, rozszerz `AlarmInputData`/coordinator tak, żeby `wifiCsiMotion` było w jednym modelu wejściowym. Nie dodawaj CSI-specific branches poza source mappingiem.

### 7.4. Boolean-like alarm UI/backend

Backend może użyć zwykłego operatora:

```text
operator = above
threshold = 0.5
```

UI ma jednak traktować `wifi_csi_motion` jako boolean source:

```text
CSI motion detected
```

Nie pokazuj użytkownikowi suwaka float dla 0/1, jeżeli łatwo to ukryć w istniejących komponentach.

## 8. Wiring w ServiceRegistry

Zmodyfikuj:

```text
src/system/services/ServiceRegistry.h
src/system/services/ServiceRegistryCallbacks.cpp
src/system/init/services/CoreServicesInitializer.cpp
src/system/init/services/*WifiSensing* albo odpowiedni initializer
```

Dokładne nazwy sprawdź w repo przed edycją.

### 8.1. Motion callback

W `ServiceRegistryCallbacks.cpp::wireRuntimeCallbacks()` dodaj:

```cpp
if (_csiService && _alarmService) {
    _csiService->setMotionCallback([this](bool motion) {
        if (_isDying.load(std::memory_order_acquire)) {
            return;
        }
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

Użyj faktycznych nazw pól (`_isDying`, `_alarmService`, `_csiService`) z repo. Jeżeli `_isDying` nie jest dostępne w tej klasie, pomiń guard albo użyj istniejącego shutdown guardu.

### 8.2. Settings constructor

Tam, gdzie tworzony jest `WifiSensingSettings`, przekaż `CsiService*`.

Update testów inicjalizacji service registry, jeżeli kompilacja native pokaże mismatch konstruktora.

## 9. Frontend: typy i API

Zmodyfikuj:

```text
interface/src/lib/types/connectivity/wifiSensing.ts
interface/src/lib/services/api/connectivity/WifiSensingApiService.ts
interface/src/lib/types/domain/alarms.ts
interface/src/lib/features/alarms/components/alarmThresholdConfig.ts
interface/src/lib/features/alarms/components/fields/AlarmConditionFields.svelte
interface/src/lib/features/alarms/components/AlarmRuleModal.svelte
interface/src/lib/features/alarms/components/AlarmRuleList.svelte
interface/src/lib/features/alarms/components/useAlarmRuleForm.svelte.ts
interface/messages/en.json
interface/messages/pl.json
```

### 9.1. WiFi sensing types

Dodaj:

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

`WifiSensingSettings`:

```ts
csi_alarm: CsiAlarmSettings;
```

`CsiRuntimeMetrics`:

```ts
motion: CsiMotionStatus;
```

### 9.2. API service

Dodaj:

```ts
async calibrateCsiAlarm(): Promise<{ ok: boolean; state?: string; error?: string }> {
  return this.client.post('/api/wifisensing/csi/calibrate', {}, {
    signal: AbortSignal.timeout(WifiSensingApiService.SAVE_TIMEOUT_MS)
  });
}
```

## 10. Frontend: CSI page UX

Zmodyfikuj:

```text
interface/src/routes/wifisensing/csi/+page.svelte
interface/src/lib/features/wifisensing/csi/CsiAmplitudeChart.svelte
interface/src/lib/features/wifisensing/csi/useCsiAmplitudeChart.svelte.ts
interface/src/lib/features/wifisensing/csi/CsiWaterfallChart.svelte
interface/src/lib/features/wifisensing/csi/useCsiWaterfallChart.svelte.ts
```

Dodaj nowy helper/component:

```text
interface/src/lib/features/wifisensing/csi/useCsiAlarmConfig.svelte.ts
interface/src/lib/features/wifisensing/csi/CsiAlarmControls.svelte
```

### 10.1. UI

Na stronie CSI dodaj panel:

```text
CSI Alarm
[ ] Enabled
Status: disabled/calibrating/monitoring/motion/noisy/needs calibration
Score: x.xx
Confidence: xx%
Bands: [58-70] [125-132] [+]
[Select band]
[Calibrate empty room]
Sensitivity: Low / Medium / High
Advanced: topK, hold, clear hold, thresholds, min noise
[Save]
```

Przycisk `Calibrate empty room`:

```text
- disabled gdy csi_alarm.enabled == false
- disabled gdy bands.length == 0
- POST /api/wifisensing/csi/calibrate
- po kliknięciu pokaż stan calibrating
```

### 10.2. Konflikt z uPlot drag-to-zoom

`CsiAmplitudeChart` obecnie używa drag do zoomu. Nie przejmuj tego zachowania globalnie.

Dodaj tryb:

```ts
selectionMode: boolean;
selectedBands: CsiAlarmBand[];
onBandSelected?: (band: CsiAlarmBand) => void;
```

Gdy `selectionMode == false`:

```text
uPlot działa jak teraz, drag = zoom
```

Gdy `selectionMode == true`:

```text
nad wykresem pojawia się transparentny overlay div
pointerdown/pointermove/pointerup obsługuje wybór pasma
uPlot drag nie dostaje eventów
```

Nie wyłączaj na stałe zoomu.

### 10.3. Mapowanie X → subcarrier index

W `useCsiAmplitudeChart.svelte.ts` dodaj metodę:

```ts
function clientXToIndex(clientX: number): number | null
```

Implementacja:

```text
- weź bounding rect plot area albo root chart rect
- policz ratio 0..1
- index = round(ratio * (subcarrierCount - 1))
- clamp 0..subcarrierCount - 1
```

Jeżeli uPlot exposes scale/bbox — użyj tego. Jeżeli nie, fallback na rect całego chartu jest akceptowalny w MVP.

### 10.4. Overlay pasm

Amplitude overlay:

```text
left = start / subcarrierCount * 100%
width = (end - start + 1) / subcarrierCount * 100%
```

Waterfall overlay analogicznie. Dodaj props:

```ts
selectedBands?: CsiAlarmBand[];
subcarrierCount?: number;
```

Nie renderuj overlay gdy `subcarrierCount <= 0`.

### 10.5. Sensitivity mapping

UI może dawać prosty wybór:

```text
Low    -> enter 8.0, clear 4.0
Medium -> enter 6.0, clear 3.0
High   -> enter 4.5, clear 2.2
```

Przy zmianie sensitivity zaktualizuj thresholds, chyba że użytkownik ręcznie edytował Advanced — wtedy zachowaj ręczne wartości.

## 11. Frontend: alarm source

### 11.1. Domain type

W `interface/src/lib/types/domain/alarms.ts` dodaj source:

```ts
'wifi_csi_motion'
```

Metadata:

```text
label: CSI Motion
unit: none
booleanLike: true
```

Dopasuj do istniejącego modelu metadata.

### 11.2. Alarm form

Dla `wifi_csi_motion`:

```ts
operator = 'above'
threshold = 0.5
```

Ukryj albo zablokuj operator/threshold. Tekst warunku:

```text
CSI motion detected
```

Nie pokazuj użytkownikowi `> 0.5` poza debugiem.

### 11.3. i18n

Dodaj klucze EN/PL, minimum:

```json
"source_wifi_csi_motion": "CSI Motion"
"alarm_condition_wifi_csi_motion": "CSI motion detected"
"csi_alarm_title": "CSI Alarm"
"csi_alarm_select_band": "Select band"
"csi_alarm_calibrate": "Calibrate empty room"
"csi_alarm_baseline_ready": "Baseline ready"
"csi_alarm_noisy": "Noisy environment"
"csi_alarm_needs_calibration": "Needs calibration"
```

Po zmianach uruchom:

```bash
cd interface && npm run i18n:build
```

## 12. Testy backend

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

### 12.1. Detector tests

Scenariusze:

1. `disabled_returns_disabled_no_motion`
2. `no_bands_returns_needs_configuration`
3. `baseline_converges_after_configured_frames`
4. `narrow_band_motion_triggers_after_hold`
5. `single_frame_spike_does_not_trigger`
6. `motion_clears_after_clear_hold`
7. `global_noise_enters_noisy_environment`
8. `width_change_resets_baseline`
9. `dead_carriers_are_ignored`
10. `selected_band_only_detects_selected_band_changes`

Synthetic packet helper:

```cpp
static CsiPacket makePacket(uint16_t width,
                            int8_t real = 10,
                            int8_t imag = 0,
                            std::initializer_list<uint16_t> boosted = {});
```

Boost local band by changing IQ values in selected indexes.

### 12.2. Config JSON tests

Testuj:

```text
- save includes nested csi_alarm
- deserialize missing csi_alarm keeps defaults
- max 4 bands
- reversed start/end normalized
- thresholds clamped
- top_k clamped
```

### 12.3. Alarm tests

Testuj:

```text
- enum string WifiCsiMotion -> wifi_csi_motion
- JSON parser accepts wifi_csi_motion
- evaluator triggers above 0.5 when input = 1.0
- evaluator does not trigger when input = 0.0
- message builder has sane label/unit
```

### 12.4. Existing Python harness

Nie usuwaj:

```text
scripts/sensing_analysis/csi_alarm_harness.py
test/test_csi_alarm_harness.py
```

Możesz dopisać komentarz w nowym detektorze, że production C++ jest portem idei harnessu, ale z band selection.

## 13. Testy frontend

Dodaj/zmodyfikuj:

```text
interface/src/lib/features/wifisensing/csi/useCsiAmplitudeChart.test.svelte.ts
interface/src/lib/features/wifisensing/csi/useCsiConnection.test.svelte.ts
interface/src/lib/features/alarms/components/useAlarmRuleForm.test.svelte.ts
interface/src/lib/features/alarms/components/AlarmRuleList.test.ts
```

Scenariusze:

```text
- parseCsiFrame nadal czyta motionScore/isMotionDetected
- selected band state zapisuje start/end w kolejności rosnącej
- selection mode nie zmienia zachowania poza overlay
- wifi_csi_motion form ustawia above/0.5
- lista reguł pokazuje boolean text zamiast >0.5
```

## 14. Komendy weryfikacyjne

Uruchamiaj w tej kolejności, nie rób pełnych clean buildów bez potrzeby.

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

Jeżeli `pio` nie jest w PATH, używaj `$HOME/.platformio/penv/bin/pio`.

## 15. Manualny test na urządzeniu

1. Flash firmware.
2. Wejdź na `/wifisensing/csi`.
3. Włącz CSI Alarm.
4. Włącz `Select band`.
5. Zaznacz 1–2 pasma widoczne jako stabilne i reagujące lokalnie, np. okolice pików z wykresu, ale nie całe spektrum.
6. Zapisz config.
7. Opuść pomieszczenie / nie ruszaj się.
8. Kliknij `Calibrate empty room`.
9. Czekaj aż status będzie `baseline_ready` / `monitoring`.
10. Porusz się w pomieszczeniu.
11. Sprawdź:
    - `motion.detected == true`,
    - CSI frame ma `isMotionDetected == true`,
    - alarm `wifi_csi_motion` odpala się przez normalny pipeline,
    - po braku ruchu stan wraca do false.
12. Zamknij stronę CSI i powtórz test alarmu. Alarm musi nadal działać headless.
13. Reboot urządzenia: config pasm ma zostać, baseline ma się skalibrować od nowa.

## 16. Ryzyka i edge-case’y

| Ryzyko | Oczekiwana obsługa |
|---|---|
| Statyczne piki na wykresie | Nie triggerują same z siebie; score jest zmianą względem baseline |
| Globalny skok gain/noise | `NoisyEnvironment`, motion false |
| Martwe/null carriery | `minEnergy` + valid mask |
| Za mało carrierów w paśmie | `NeedsConfiguration` albo `validSelectedCarrierCount` niski |
| Zmiana szerokości CSI | reset baseline |
| Brak otwartej strony CSI | `CsiConsumer::AlarmSystem` utrzymuje CSI aktywne |
| Single callback frontend | nie używaj `_csiCallback` do alarmów; dodaj osobny `_motionCallback` |
| Packet loss | hold/hysteresis minimalizują pojedyncze dropy |
| Za szybka adaptacja baseline | EWMA tylko gdy no motion/no noisy/score low |
| False positives po starcie | alarm false aż baseline ready |
| RSSI wifi_motion regression | nie ruszać istniejącego source ani jego configu |

## 17. Acceptance criteria

Implementacja jest gotowa, gdy:

1. `wifi_motion` nadal działa jak dotychczas na RSSI variance.
2. Jest nowe źródło `wifi_csi_motion` end-to-end: enum, JSON, frontend, evaluator.
3. CSI detector działa w `CsiService::processingTask()` i uzupełnia `packet.motionScore` oraz `packet.isMotionDetected`.
4. Wire format CSI nie zmienia rozmiaru ani kolejności pól.
5. CSI alarm działa bez otwartej strony CSI.
6. Wybrane pasma zapisują się w `/api/wifisensing/config` pod `csi_alarm.bands`.
7. Kalibracja jest dostępna przez `POST /api/wifisensing/csi/calibrate`.
8. `/api/wifisensing/status` pokazuje `csi.motion`.
9. Alarmy dostają tylko `wifiCsiMotion = 0.0f | 1.0f`.
10. Nie ma bezpośredniej notyfikacji/Telegram/LED z CSI.
11. Native tests przechodzą.
12. `cd interface && npm run check && npm run test:run` przechodzi.
13. `./scripts/build-fast.sh` przechodzi.
14. Nie zmieniono `lib/framework/**` ani `src/wifisensing/csi/vendor/**`.

## 18. Kolejność implementacji

### Phase 1 — Detector pure C++

1. Dodaj `CsiMotionTypes.h`.
2. Dodaj `CsiBandMotionDetector.h/.cpp`.
3. Dodaj testy native detektora.
4. Uruchom tylko test detektora.

Nie dotykaj jeszcze alarmów ani UI.

### Phase 2 — CSI Service integration

1. Dodaj detector member i motion callback do `CsiService`.
2. Podepnij detector w `processingTask()`.
3. Dodaj motion fields do `CsiMetricsSnapshot`.
4. Utrzymaj WebSocket CSI bez zmian layoutu.
5. Uruchom `test_csi_wire_format` jeśli istnieje i nowy detector test.

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

## 19. Źródła techniczne do kontekstu

Te źródła są tylko kontekstem, nie wymagają implementacji ML:

- Espressif ESP-IDF Wi-Fi API Reference: `esp_wifi_set_csi_rx_cb`, `esp_wifi_set_csi_config`, `esp_wifi_set_csi`. Ważne dla zasady, że callback CSI jest hot path i nie może robić ciężkiej pracy.
- Yousefi et al., “A Survey of Human Activity Recognition Using WiFi CSI”, arXiv:1708.07129. Kontekst: ruch człowieka zmienia multipath i sekwencje CSI.
- Li et al., “Wi-Motion: A Robust Human Activity Recognition Using WiFi Signals”, arXiv:1810.11705. Kontekst: CSI amplitude/phase jako feature dla ruchu.
- Rousseeuw & Croux, “Alternatives to the Median Absolute Deviation”, Journal of the American Statistical Association, 1993. Kontekst robust scale.
- Hampel/MAD robust statistics. Kontekst: odporność na outliery, ale w MVP używamy Welford stddev z guardami pamięciowymi.

## 20. Prompt do Codex Goal Mode

Skopiuj poniższy prompt do Goal Mode:

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
- Pracuj etapami z planu: detector → CsiService → config/API → alarm source → frontend CSI UX → frontend alarms → testy.

Wymagania techniczne:
- Dodaj `CsiBandMotionDetector` z baseline Welford, per-subcarrier z-score, selected bands, top-K scoring, hysteresis, hold/clear hold, noisy gate i recalibration guard.
- Podepnij detektor w `CsiService::processingTask()` i wypełniaj `packet.motionScore` oraz `packet.isMotionDetected`.
- Dodaj `CsiService::setMotionCallback`, `setMotionConfig`, `requestMotionCalibration`, `getMotionSnapshot`.
- Rozszerz `/api/wifisensing/config` o nested `csi_alarm` i `/api/wifisensing/status` o `csi.motion`.
- Dodaj `POST /api/wifisensing/csi/calibrate`.
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
